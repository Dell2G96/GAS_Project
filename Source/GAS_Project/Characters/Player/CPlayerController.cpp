// CPlayerController.cpp
#include "CPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "Net/UnrealNetwork.h"

void ACPlayerController::BeginPlay()
{
    Super::BeginPlay();
    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
    
}

// void ACPlayerController::SendAbilityInputEvent(const FGameplayTag& EventTag, bool bPressed)
// {
//     return;
// }

void ACPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    OwnerCharacter = Cast<ACPlayerCharacter>(InPawn);
    if (OwnerCharacter)
    {
        // 팀아이디는 빙의 되기전에 실행되어야한다
        OwnerCharacter->SetGenericTeamId(TeamID);
        OwnerCharacter->ServerSideInit();
    }
    
}

void ACPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACPlayerController, TeamID);
}

void ACPlayerController::Server_SendGameplayEventToSelf_Implementation(const FGameplayTag& EventTag,
    const FGameplayEventData& EventData)
{
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, EventData);
}

bool ACPlayerController::Server_SendGameplayEventToSelf_Validate(const FGameplayTag& EventTag,
    const FGameplayEventData& EventData)
{
    return true;
}

void ACPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent(); 
    // ✅ 1. Enhanced Input Component 캐스팅 확인
    UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInputComp)
    {
        return;
    }
    
    // ✅ 2. LocalPlayer 체크 (클라이언트에서만 유효)
    if (!IsLocalPlayerController())
    {
        return;
    }
    
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer)
    {
        return;
    }
    
    // ✅ 3. InputMappingContext 등록 (안전하게)
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
    
    if (InputSubsystem)
    {
        // ✅ 기존 컨텍스트 모두 제거 (중복 방지)
        InputSubsystem->ClearAllMappings();
        
        // ✅ 새 컨텍스트 등록
        for (UInputMappingContext* Context : InputMappingContexts)
        {
            if (Context)
            {
                InputSubsystem->AddMappingContext(Context, 0);
                UE_LOG(LogTemp, Warning, TEXT("Added InputMappingContext: %s"), *Context->GetName());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SetupInputComponent: InputSubsystem is NULL!"));
        return;
    }

    // ✅ 4. 기본 입력 바인딩
    {
        EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACPlayerController::Jump);
        EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACPlayerController::StopJumping);
        EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACPlayerController::Move);
        EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACPlayerController::Look);
        EnhancedInputComp->BindAction(RunAction, ETriggerEvent::Started, this, &ACPlayerController::Run);
        EnhancedInputComp->BindAction(RunAction, ETriggerEvent::Completed, this, &ACPlayerController::StopRuning);
     
    }
    // ✅ 5. Ability 입력 바인딩
    for (const TPair<ECabilityInputID, UInputAction*>& InputActionPair : GameplayAbilityInputActions)
    {
        if (InputActionPair.Value)
        {
            EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Started,
                this, &ACPlayerController::HandleAbilityInput, InputActionPair.Key);
            
            
            UE_LOG(LogTemp, Warning, TEXT("Bound Ability InputAction: InputID=%d"), (int32)InputActionPair.Key);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== SetupInputComponent COMPLETED ==="));
}

void ACPlayerController::Jump()
{
    if (!IsValid(GetCharacter())) return;
    
    GetCharacter()->Jump();
}

void ACPlayerController::Run()
{
    if (!IsValid(GetCharacter())) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 600.f;
}

void ACPlayerController::StopRuning()
{
    if (!IsValid(GetCharacter())) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 400.f;
}



void ACPlayerController::StopJumping()
{
    if (!IsValid(GetCharacter())) return;
    GetCharacter()->StopJumping();
}

void ACPlayerController::Move(const FInputActionValue& Value)
{
    if (!IsValid(GetPawn())) return;

    const FVector2D MovementVector = Value.Get<FVector2D>();

    const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
    GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void ACPlayerController::Look(const FInputActionValue& Value)
{
    
    const FVector2D LookAxisVector = Value.Get<FVector2D>();

    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
}


// ✅ 함수 분리: Pressed
void ACPlayerController::HandleAbilityInputPressed(ECabilityInputID InputId)
{
    
    OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleAbilityInputPressed: OwnerCharacter is NULL!"));
        return;
    }

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!IsValid(ASC))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleAbilityInputPressed: ASC is NULL!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> HandleAbilityInput PRESSED: InputID=%d"), (int32)InputId);

    ASC->AbilityLocalInputPressed((int32)InputId);

    if (InputId == ECabilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = UCAbilitySystemStatics::GetBasicAttackInputPressedTag();
        UE_LOG(LogTemp, Warning, TEXT(">>> Sending GameplayEvent: %s"), *BasicAttackTag.ToString());

        SendEventToPawn(BasicAttackTag);

        // UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
}

void ACPlayerController::HandleAbilityInput(const FInputActionValue& InputActionValue, ECabilityInputID InputID)
{
    OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!OwnerCharacter) return;
    
    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!ASC) return;


    bool bPressed = InputActionValue.Get<bool>();
    
    if (bPressed)
    {
        ASC->AbilityLocalInputPressed((int32)InputID);
    }
    else
    {
        ASC->AbilityLocalInputReleased((int32)InputID);
    }

    if (InputID == ECabilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = bPressed ? UCAbilitySystemStatics::GetBasicAttackInputPressedTag() : UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter , BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
}




// ✅ 함수 분리: Released
void ACPlayerController::HandleAbilityInputReleased(ECabilityInputID InputId)
{
    OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(OwnerCharacter)) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!IsValid(ASC)) return;

    UE_LOG(LogTemp, Warning, TEXT(">>> HandleAbilityInput RELEASED: InputID=%d"), (int32)InputId);

    ASC->AbilityLocalInputReleased((int32)InputId);

    if (InputId == ECabilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();
        UE_LOG(LogTemp, Warning, TEXT(">>> Sending GameplayEvent: %s"), *BasicAttackTag.ToString());

        SendEventToPawn(BasicAttackTag);
        // UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
}

void ACPlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
    if (!IsValid(ASC)) return;

    ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

void ACPlayerController::SendEventToPawn(const struct FGameplayTag& Tag)
{
    Owner = GetPawn();
    if (!Owner || !Tag.IsValid()) return;

    FGameplayEventData Data;   // 필요 시 Payload 채워넣기
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, Tag, Data);
}

