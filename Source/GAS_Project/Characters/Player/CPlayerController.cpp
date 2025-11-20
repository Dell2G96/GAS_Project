// CPlayerController.cpp
#include "CPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Utils/CStructTypes.h"

void ACPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // ✅ InputMappingContext 등록 (필수!)
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = 
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    
    if (InputSubsystem)
    {
        for (UInputMappingContext* Context : InputMappingContexts)
        {
            if (Context)
            {
                InputSubsystem->AddMappingContext(Context, 0);
            }
        }
    }

    UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInputComp)
    {
        UE_LOG(LogTemp, Error, TEXT("SetupInputComponent: Failed to cast to UEnhancedInputComponent"));
        return;
    }

    // 기본 입력
    EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACPlayerController::Jump);
    EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACPlayerController::StopJumping);
    EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACPlayerController::Move);
    EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACPlayerController::Look);
    EnhancedInputComp->BindAction(RunAction, ETriggerEvent::Started, this, &ACPlayerController::Run);
    EnhancedInputComp->BindAction(RunAction, ETriggerEvent::Completed, this, &ACPlayerController::StopRuning);

    // ✅ 어빌리티 입력: Started/Completed로 변경 (중요!)
    for (const TPair<ECabilityInputID, UInputAction*>& InputActionPair : GameplayAbilityInputActions)
    {
        if (InputActionPair.Value)
        {
            // Started (누를 때)
            EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Started, 
                this, &ACPlayerController::HandleAbilityInputPressed, InputActionPair.Key);
            
            // Completed (뗄 때)
            EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Completed, 
                this, &ACPlayerController::HandleAbilityInputReleased, InputActionPair.Key);
        }
    }
}

void ACPlayerController::Jump()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive()) return;
    GetCharacter()->Jump();
}

void ACPlayerController::Run()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive()) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 600.f;
}

void ACPlayerController::StopRuning()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive()) return;
    GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

void ACPlayerController::StopJumping()
{
    if (!IsValid(GetCharacter())) return;
    if (!IsAlive()) return;
    GetCharacter()->StopJumping();
}

void ACPlayerController::Move(const FInputActionValue& Value)
{
    if (!IsValid(GetPawn())) return;
    if (!IsAlive()) return;

    const FVector2D MovementVector = Value.Get<FVector2D>();

    const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
    GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void ACPlayerController::Look(const FInputActionValue& Value)
{
    if (!IsAlive()) return;
    
    const FVector2D LookAxisVector = Value.Get<FVector2D>();

    AddYawInput(LookAxisVector.X);
    AddPitchInput(LookAxisVector.Y);
}

// ✅ 함수 분리: Pressed
void ACPlayerController::HandleAbilityInputPressed(ECabilityInputID InputId)
{
    ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
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

        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
}

// ✅ 함수 분리: Released
void ACPlayerController::HandleAbilityInputReleased(ECabilityInputID InputId)
{
    ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(OwnerCharacter)) return;

    UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
    if (!IsValid(ASC)) return;

    UE_LOG(LogTemp, Warning, TEXT(">>> HandleAbilityInput RELEASED: InputID=%d"), (int32)InputId);

    ASC->AbilityLocalInputReleased((int32)InputId);

    if (InputId == ECabilityInputID::BasicAttack)
    {
        FGameplayTag BasicAttackTag = UCAbilitySystemStatics::GetBasicAttackInputReleasedTag();
        UE_LOG(LogTemp, Warning, TEXT(">>> Sending GameplayEvent: %s"), *BasicAttackTag.ToString());

        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), BasicAttackTag, FGameplayEventData());
        OwnerCharacter->Server_SendGameplayEventToSelf(BasicAttackTag, FGameplayEventData());
    }
}

void ACPlayerController::BasicAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("Is Activate Primary Ability"));
    ActivateAbility(MyTags::Abilities::BasicAttack);
}

void ACPlayerController::HeavyAttack()
{
    ActivateAbility(MyTags::Abilities::HeavyAttack);
}

void ACPlayerController::Equip()
{
    ACPlayerCharacter* PlayerCharacter = Cast<ACPlayerCharacter>(GetPawn());
    if (!IsValid(PlayerCharacter)) return;

    UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
    if (!IsValid(ASC)) return;

    if (ASC->HasMatchingGameplayTag(MyTags::Abilities::Equip::EquipKnife))
    {
        ActivateAbility(MyTags::Abilities::UnEquip::UnEquipKnife);
        return;
    }
    ActivateAbility(MyTags::Abilities::Equip::EquipKnife);
}

void ACPlayerController::UnEquipTest()
{
    ActivateAbility(MyTags::Abilities::UnEquip::UnEquipKnife);
}

void ACPlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
    if (!IsAlive()) return;

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
    if (!IsValid(ASC)) return;

    ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

bool ACPlayerController::IsAlive() const
{
    ACCharacter* BaseCharacter = Cast<ACCharacter>(GetPawn());
    if (!IsValid(BaseCharacter)) return false;

    return BaseCharacter->IsAlive();
}
