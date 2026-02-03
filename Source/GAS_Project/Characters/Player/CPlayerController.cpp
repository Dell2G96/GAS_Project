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
#include "GAS_Project/Characters/Enemy/CEnemyBase.h"
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
        
        // [ADDED] Execution UI는 “로컬 플레이어”가 만들고 관리
        SpawnExecutionWidget();    }
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


   //      // Temp
   //      if (ExecutionAction)
   //      {
   //          
			// EnhancedInputComp->BindAction(ExecutionAction, ETriggerEvent::Started, this, &ThisClass::Input_ExecutionPressed);
   //      }
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

                EnhancedInputComp->BindAction(InputActionPair.Value, ETriggerEvent::Completed,
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

void ACPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // [ADDED] 현재 타겟이 있으면 매 프레임 위치 갱신
    UpdateExecutionWidgetPosition();
}

void ACPlayerController::SpawnExecutionWidget()
{
    // [ADDED] 로컬 컨트롤러에서만 UI 생성
    if (!IsLocalController()) return;

    if (!ExecutionWidgetClass || ExecutionWidget)  return;

    ExecutionWidget = CreateWidget<UUserWidget>(this, ExecutionWidgetClass);
    if (!ExecutionWidget)  return;

    ExecutionWidget->AddToViewport();
    ExecutionWidget->SetVisibility(ESlateVisibility::Hidden);

    // [ADDED] (0,0) 왼쪽상단 박힘 방지: 중앙 Pivot으로 두고 매 프레임 스크린 좌표로 이동
    ExecutionWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void ACPlayerController::RefreshExecutionTarget()
{
    if (!ExecutionWidget)  return;

    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        ExecutionWidget->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    // [ADDED] 후보가 여러 개면 “가장 가까운 Enemy”를 선택
    ACEnemyBase* Best = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (ACEnemyBase* Candidate : ExecutionCandidates)
    {
        if (!IsValid(Candidate))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }

    CurrentExecutionTarget = Best;

    if (CurrentExecutionTarget)
    {
        ExecutionWidget->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        ExecutionWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ACPlayerController::UpdateExecutionWidgetPosition()
{
    if (!ExecutionWidget)
    {
        return;
    }

    if (!CurrentExecutionTarget || !IsValid(CurrentExecutionTarget))
    {
        ExecutionWidget->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    // [ADDED] Enemy 몸 중앙(소켓/오프셋) 월드좌표를 스크린좌표로 투영
    const FVector WorldLoc = CurrentExecutionTarget->GetExecutionUIWorldLocation();

    FVector2D ScreenPos;
    const bool bProjected = ProjectWorldLocationToScreen(WorldLoc, ScreenPos, true);

    if (!bProjected)
    {
        ExecutionWidget->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    ExecutionWidget->SetVisibility(ESlateVisibility::Visible);

    // [CHANGED] 위치 안 잡으면 (0,0)으로 남아서 왼쪽상단에 뜸
    ExecutionWidget->SetPositionInViewport(ScreenPos, true);
    ExecutionWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

void ACPlayerController::Client_SetExecutionCandidate_Implementation(class ACEnemyBase* Enemy, bool bShow)
{
    // [ADDED] 서버가 판정한 결과를 로컬 UI 후보 집합에 반영
    if (!IsLocalController())  return;

    if (!Enemy) return;

    if (bShow)
        ExecutionCandidates.Add(Enemy);
    else
    {
        ExecutionCandidates.Remove(Enemy);

        // [ADDED] 현재 타겟이 이 Enemy였으면 초기화
        if (CurrentExecutionTarget == Enemy)
        {
            CurrentExecutionTarget = nullptr;
        }
    }

    RefreshExecutionTarget();
}
//
// void ACPlayerController::EnsureExecutionUI()
// {
//     if (!IsLocalPlayerController()) return;
//     if (ExecutionUI) return;
//     if (!ExecutionUIClass) return;
//
//     ExecutionUI = CreateWidget<UUserWidget>(this, ExecutionUIClass);
//     if (!ExecutionUI)
//     {
//         UE_LOG(LogTemp, Warning, TEXT(">>> EnsureExecutionUI"));
//         return ;
//     }
//
//     ExecutionUI->AddToViewport();
//     ExecutionUI->SetVisibility(ESlateVisibility::Hidden);
// }
//
// void ACPlayerController::Input_ExecutionPressed(const struct FInputActionValue& InputActionValue)
// {
//     if (!IsLocalController()) return;
//     if (!CurrentExecutionTarget.IsValid()) return;
//
//     ServerRequestExecution(CurrentExecutionTarget.Get());
//         
//         
// }
//
// void ACPlayerController::ClientSetExecutionPrompt_Implementation(ACEnemyBase* TargetEnemy, bool bShow)
// {
//     if (!IsLocalPlayerController()) return;
//
//     EnsureExecutionUI();
//     if (!ExecutionUI) return;
//
//     if (bShow)
//     {
//         CurrentExecutionTarget = TargetEnemy;
//         ExecutionUI->SetVisibility(ESlateVisibility::Visible);
//
//         if (!GetWorldTimerManager().IsTimerActive(ExecutionWidgetUpdateTimer))
//         {
//             GetWorldTimerManager().SetTimer(ExecutionWidgetUpdateTimer, this, 
//                 &ACPlayerController::UpdateExecutionWidgetPosition, 0.016f, true);
//         }
//     }
//     else
//     {
//         if (!CurrentExecutionTarget.IsValid() || CurrentExecutionTarget.Get() == TargetEnemy)
//         {
//             CurrentExecutionTarget = nullptr;
//             ExecutionUI->SetVisibility(ESlateVisibility::Hidden);
//         }
//     }
// }

// void ACPlayerController::ServerRequestExecution_Implementation(ACEnemyBase* TargetEnemy)
// {
//     if (!TargetEnemy) return;
//     APawn* MyPawn = GetPawn();
//     if (!MyPawn) return;
//
//     const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), TargetEnemy->GetActorLocation());
//     if (DistSq > FMath::Square(250.f))
//     {
//         return;
//     }
//     UE_LOG(LogTemp, Warning, TEXT("[Execution] ServerRequestExecution accepted for %s"), *TargetEnemy->GetName());
// }

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

    // [ADDED] 클라(로컬)에서도 먼저 이벤트를 쏴서, 로컬 GA 인스턴스가 즉시 SwitchTarget 실행하게 함
    {
        FGameplayEventData Data;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PS, Tag, Data);
    }

    // 리슨서버 호스트(Authority)는 위에서 이미 처리 끝
    if (HasAuthority())
        return;

    // [CHANGED] 클라는 서버에도 알려서(권한/판정용), 서버 GA도 동일하게 처리하게 함
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


