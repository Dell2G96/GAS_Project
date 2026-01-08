// CPlayerController.cpp
#include "CPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CPlayerCharacter.h"
#include "CPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "GAS_Project/Widgets/GameplayWidget.h"
#include "Net/UnrealNetwork.h"


void ACPlayerController::OnPossess(APawn* NewPawn)
{
    Super::OnPossess(NewPawn);

    OwnerCharacter = Cast<ACPlayerCharacter>(NewPawn);
    if (OwnerCharacter)
    {
        OwnerCharacter->ServerSideInit();
        // 팀아이디는 빙의 되기전에 실행되어야한다
        OwnerCharacter->GetGenericTeamId();
    }
}

void ACPlayerController::AcknowledgePossession(class APawn* NewPawn)
{
    Super::AcknowledgePossession(NewPawn);
    OwnerCharacter = Cast<ACPlayerCharacter>(NewPawn);
    if (OwnerCharacter)
    {
        OwnerCharacter->ClientSideInit();
        SpawnGameplayWidget();
    }
}

void ACPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    TeamID = NewTeamID;
}

FGenericTeamId ACPlayerController::GetGenericTeamId() const
{
    return TeamID;
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
    if (!EnhancedInputComp)  return;
    
    // ✅ 2. LocalPlayer 체크 (클라이언트에서만 유효)
    if (!IsLocalPlayerController()) return;
    
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer)  return;
    
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
    for (const TPair<ECAbilityInputID, UInputAction*>& InputActionPair : GameplayAbilityInputActions)
    {
        if (InputActionPair.Value)
        {
            if (InputActionPair.Key == ECAbilityInputID::TargetSwitch)
            {
                EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Triggered,
                this, &ACPlayerController::Input_SwitchTargetTriggered);

                EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Triggered,
               this, &ACPlayerController::Input_SwitchTargetCompleted);
                
                continue;
            }
            EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Triggered,
                this, &ACPlayerController::HandleAbilityInput, InputActionPair.Key);
            
            
            UE_LOG(LogTemp, Warning, TEXT("Bound Ability InputAction: InputID=%d"), (int32)InputActionPair.Key);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("=== SetupInputComponent COMPLETED ==="));
}

void ACPlayerController::Jump()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive() || IsKnockdown()) return;
    
    GetCharacter()->Jump();
}

void ACPlayerController::Run()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive() || IsKnockdown()) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 600.f;
}

void ACPlayerController::StopRuning()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive() || IsKnockdown()) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

void ACPlayerController::StopJumping()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive() || IsKnockdown()) return;
    GetCharacter()->StopJumping();
}

void ACPlayerController::Move(const FInputActionValue& Value)
{
    if (!IsValid(GetPawn())) return;
    if (!IsAlive() || IsKnockdown()) return;

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
void ACPlayerController::HandleAbilityInputPressed(ECAbilityInputID InputId)
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

    if (InputId == ECAbilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = UCAbilitySystemStatics::GetBasicAttackInputPressedTag();
        UE_LOG(LogTemp, Warning, TEXT(">>> Sending GameplayEvent: %s"), *BasicAttackTag.ToString());

        SendEventToPawn(BasicAttackTag);

        // UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
  
}

void ACPlayerController::HandleAbilityInput(const FInputActionValue& InputActionValue, ECAbilityInputID InputID)
{
    OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!OwnerCharacter) return;
    
    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!ASC) return;

    if (InputID == ECAbilityInputID::TargetLock)
    {
        UCAbilitySystemComponent* OwnerASC = Cast<UCAbilitySystemComponent>(ASC);
        OwnerASC->OnAbilityInputPressed(InputID);
        return;
    }

    bool bPressed = InputActionValue.Get<bool>();
    
    if (bPressed)
    {
        ASC->AbilityLocalInputPressed((int32)InputID);
    }
    else
    {
        ASC->AbilityLocalInputReleased((int32)InputID);
    }

    if (InputID == ECAbilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = bPressed ? UCAbilitySystemStatics::GetBasicAttackInputPressedTag() : UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter , BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
    if (InputID == ECAbilityInputID::Guard)
    {
        FGameplayTag GuardTag = bPressed ? UCAbilitySystemStatics::GetGuardInputPressedTag() : UCAbilitySystemStatics::GetGuardInputReleasedTag();
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter , GuardTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(GuardTag, FGameplayEventData());
    }
   
}

// ✅ 함수 분리: Released
void ACPlayerController::HandleAbilityInputReleased(ECAbilityInputID InputId)
{
    OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(OwnerCharacter)) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!IsValid(ASC)) return;

    UE_LOG(LogTemp, Warning, TEXT(">>> HandleAbilityInput RELEASED: InputID=%d"), (int32)InputId);

    ASC->AbilityLocalInputReleased((int32)InputId);

    if (InputId == ECAbilityInputID::BasicAttack)
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

bool ACPlayerController::IsKnockdown()
{
    ACPlayerCharacter* PlayerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(PlayerCharacter)) return false;
    return PlayerCharacter->IsKnockedDown();
}

bool ACPlayerController::IsAlive()
{
    ACCharacter* Playercharacter = Cast<ACCharacter>(GetPawn());
    if (!IsValid(Playercharacter)) return false;
    return Playercharacter->IsAlive();
}

void ACPlayerController::SpawnGameplayWidget()
{
    if (!IsLocalPlayerController())
        return;

    GameplayWidget = CreateWidget<UGameplayWidget>(this, GameplayWidgetClass);
    if (GameplayWidget)
    {
        GameplayWidget->AddToViewport();
       // GameplayWidget->ConfigureAbilities(OwnerCharacter->GetAbilities());
    }
}

void ACPlayerController::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
    SwitchDirection = InputActionValue.Get<FVector2D>();
}

void ACPlayerController::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{	

    // 입력은 로컬 컨트롤러에서만 처리
    if (!IsLocalController())
        return;

    const FGameplayTag Tag =
        (SwitchDirection.X > 0.f) ? MyTags::Events::SwitchTarget_Right
                                  : MyTags::Events::SwitchTarget_Left;

    // 이벤트를 받을 대상(보통 ASC가 붙어있는 Pawn/Character)
    ACPlayerState* PS = (GetPlayerState<ACPlayerState>());
    if (!PS)
        return;

    // ACCharacter* OC = Cast<ACCharacter>(GetPawn());
    // 리슨서버 호스트(Authority)는 바로 실행
    if (HasAuthority())
    {
        FGameplayEventData Data;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PS, Tag, Data);
        return;
    }

    // 데디서버/클라 환경: 서버에게 요청해서 서버가 이벤트를 쏴주게 함
    Server_Input_SwitchTargetCompleted(Tag);
}

void ACPlayerController::Server_Input_SwitchTargetCompleted_Implementation(const FGameplayTag& Tag)
{

    ACPlayerState* PS = (GetPlayerState<ACPlayerState>());
    if (!PS)
        return;

    FGameplayEventData Data;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PS, Tag, Data);
}


