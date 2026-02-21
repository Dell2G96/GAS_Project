// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS_Project/Components/ExecutionComponent.h"

#include "AbilitySystemInterface.h"
#include "BrainComponent.h"
#include "MotionWarpingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AI/CAIController.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "Net/UnrealNetwork.h"

UExecutionComponent::UExecutionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    
    ExecutionWarpTargetName = FName(TEXT("ExecutionTarget"));
    ExecutionRange = 200.f;
    ExecutionStartDistance = 100.f;
    ExecutableHealthPercent = 0.3f;
    bExecutableWhenGroggy = true;
    BloodEffectSocketName = FName("spine_03");
}

void UExecutionComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 오너 캐릭터 캐싱
    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] Owner is not a Character!"));
        return;
    }
    
    // 모션 워핑 컴포넌트 찾기
    MotionWarpingComp = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();
    if (!MotionWarpingComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] MotionWarpingComponent not found on %s"), 
            *OwnerCharacter->GetName());
    }
    
    // ❌ AnimInstance는 BeginPlay에서 캐싱하지 말고, 사용할 때마다 가져오기
    // OwnerAnimInstance = Mesh->GetAnimInstance(); // 제거
}

void UExecutionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UExecutionComponent, CurrentRole);
    DOREPLIFETIME(UExecutionComponent, CurrentTarget);
}

void UExecutionComponent::SetVictim(AActor* InVictim)
{
    CurrentTarget = InVictim;
    CurrentRole = EExecutionRole::Victim;
}

void UExecutionComponent::PlayVictimMontage(int32 AttackIndex, AActor* Attacker)
{
    if (!VictimMontages.IsValidIndex(AttackIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Invalid victim montage index: %d"), AttackIndex);
        return;
    }
    
    UAnimMontage* VictimMontage = VictimMontages[AttackIndex];
    if (!VictimMontage)
    {
        return;
    }
    
    // ✅ 매번 새로 AnimInstance 가져오기
    if (!OwnerCharacter) return;
    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;
    
    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] AnimInstance is NULL for %s"), 
            *OwnerCharacter->GetName());
        return;
    }
    
    // 애니메이션 재생
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Playing Victim Montage on %s (Role: %d)"), 
        *OwnerCharacter->GetName(), (int32)OwnerCharacter->GetLocalRole());
    
    AnimInstance->Montage_Play(VictimMontage);
    
    // 몽타주 종료 델리게이트
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UExecutionComponent::OnVictimMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, VictimMontage);
    
    // 움직임 및 입력 비활성화
    DisableCharacterMovement();
    DisableCharacterInput();
    DisableAI();
    
    // 공격자 방향으로 회전
    if (Attacker && OwnerCharacter)
    {
        FVector Direction = (Attacker->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
        OwnerCharacter->SetActorRotation(Direction.Rotation());
    }
}

void UExecutionComponent::ActivateBloodTrail()
{
    if (!BloodEffect || !OwnerCharacter)
    {
        return;
    }
    
    // 피 이펙트 위치 결정
    FVector EffectLocation;
    if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
    {
        if (Mesh->DoesSocketExist(BloodEffectSocketName))
        {
            EffectLocation = Mesh->GetSocketLocation(BloodEffectSocketName);
        }
        else
        {
            EffectLocation = OwnerCharacter->GetActorLocation();
        }
    }
    else
    {
        EffectLocation = OwnerCharacter->GetActorLocation();
    }
    
    // 나이아가라 이펙트 스폰
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        BloodEffect,
        EffectLocation,
        OwnerCharacter->GetActorRotation()
    );
}

bool UExecutionComponent::CanBeExecuted() const
{
    if (!OwnerCharacter || CurrentRole != EExecutionRole::None)
    {
        return false;
    }
    
    // AbilitySystemComponent 확인
    IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerCharacter);
    if (!ASI)
    {
        return false;
    }
    
    UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
    if (!ASC)
    {
        return false;
    }
    
    // Groggy 상태 체크
    if (bExecutableWhenGroggy && ASC->HasMatchingGameplayTag(MyTags::Status::Groggy))
    {
        return true;
    }
    
    // 체력 체크
    const UCAttributeSet* AttributeSet = ASC->GetSet<UCAttributeSet>();
    if (AttributeSet)
    {
        float CurrentHealth = AttributeSet->GetHealth();
        float MaxHealth = AttributeSet->GetMaxHealth();
        
        if (MaxHealth > 0.f)
        {
            float HealthPercent = CurrentHealth / MaxHealth;
            return HealthPercent <= ExecutableHealthPercent;
        }
    }
    
    return false;
}

void UExecutionComponent::StartExecution(AActor* Target)
{
    if (!Target || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] StartExecution: Invalid Target or Owner"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] StartExecution called on %s (HasAuthority: %d)"), 
        *OwnerCharacter->GetName(), OwnerCharacter->HasAuthority());
    
    if (AttackerMontages.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] No attacker montages configured"));
        return;
    }
    // ✅ 디버그: 몽타주 개수 확인
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] AttackerMontages count: %d"), 
        AttackerMontages.Num());
    
    // ✅ 랜덤 인덱스 생성 (개선)
    int32 MontageIndex = 0;
    if (AttackerMontages.Num() > 1)
    {
        MontageIndex = FMath::RandRange(0, AttackerMontages.Num() - 1);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Selected MontageIndex: %d"), MontageIndex);
    
    // ✅ 서버 RPC 호출
    Server_RequestExecution(Target, MontageIndex);
}

void UExecutionComponent::TryExecuteNearestTarget()
{
    // ✅ OwnerCharacter null 체크 추가
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] TryExecuteNearestTarget: OwnerCharacter is NULL!"));
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] TryExecuteNearestTarget called on %s (Authority: %d)"), 
        *OwnerCharacter->GetName(), OwnerCharacter->HasAuthority());
    
    AActor* Target = FindNearestExecutableTarget();
    if (Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Found target: %s"), *Target->GetName());
        StartExecution(Target);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] No executable target found in range"));
    }
}

void UExecutionComponent::OnRep_CurrentRole()
{
    UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Role replicated to: %d"), (int32)CurrentRole);
}

AActor* UExecutionComponent::FindNearestExecutableTarget() const
{
     if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] FindNearestExecutableTarget: OwnerCharacter is NULL"));
        return nullptr;
    }
    
    // ✅ 방법 1: Sphere Overlap 사용 (더 정확함)
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);
    
    FVector StartLocation = OwnerCharacter->GetActorLocation();
    
    // Sphere 충돌 검사
    bool bHasOverlap = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        StartLocation,
        FQuat::Identity,
        ECC_Pawn,  // Pawn 채널로 검색
        FCollisionShape::MakeSphere(ExecutionRange),
        QueryParams
    );
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Sphere overlap found %d actors in range %.1f"), 
        OverlapResults.Num(), ExecutionRange);
    
    if (!bHasOverlap)
    {
        return nullptr;
    }
    
    AActor* BestTarget = nullptr;
    float BestDistance = ExecutionRange;
    
    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* Actor = Result.GetActor();
        if (!Actor || Actor == OwnerCharacter)
        {
            continue;
        }
        
        UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Checking actor: %s"), *Actor->GetName());
        
        // ExecutionComponent가 있는지 확인
        UExecutionComponent* TargetExecComp = Actor->FindComponentByClass<UExecutionComponent>();
        if (!TargetExecComp)
        {
            UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Actor %s has no ExecutionComponent"), *Actor->GetName());
            continue;
        }
        
        // 처형 가능한지 확인
        bool bCanExecute = TargetExecComp->CanBeExecuted();
        UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Actor %s CanBeExecuted: %d"), 
            *Actor->GetName(), bCanExecute);
        
        if (!bCanExecute)
        {
            continue;
        }
        
        // 거리 체크
        float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Actor->GetActorLocation());
        UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Actor %s distance: %.1f (Best: %.1f)"), 
            *Actor->GetName(), Distance, BestDistance);
        
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            BestTarget = Actor;
        }
    }
    
    if (BestTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Selected target: %s at distance %.1f"), 
            *BestTarget->GetName(), BestDistance);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] No valid executable target found"));
    }
    
    return BestTarget;
}

void UExecutionComponent::SetupMotionWarping(AActor* Target)
{
    if (!MotionWarpingComp || !Target || !OwnerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] SetupMotionWarping: Missing components"));
        return;
    }
    
    FVector VictimLocation = Target->GetActorLocation();
    FVector AttackerLocation = OwnerCharacter->GetActorLocation();
    FVector Direction = (VictimLocation - AttackerLocation).GetSafeNormal();
    
    // ✅ Yaw만 사용 (지면과 평행하게)
    FRotator TargetRotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f);
    
    // ✅ 피해자 앞으로 워프 위치 계산
    FVector WarpLocation = VictimLocation - (Direction * ExecutionStartDistance);
    
    // ✅ Z축은 공격자의 현재 높이 유지 (지형 차이 보정)
    WarpLocation.Z = AttackerLocation.Z;
    
    MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(
        ExecutionWarpTargetName,
        WarpLocation,
        TargetRotation
    );
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Motion Warping set: Loc=(%.1f,%.1f,%.1f), Rot=(%.1f)"), 
        WarpLocation.X, WarpLocation.Y, WarpLocation.Z, TargetRotation.Yaw);
}

void UExecutionComponent::DisableCharacterMovement()
{
    if (!OwnerCharacter) return;
    
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->DisableMovement();
    }
}

void UExecutionComponent::EnableCharacterMovement()
{
    if (!OwnerCharacter) return;
    
    if (UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement())
    {
        MovementComp->SetMovementMode(MOVE_Walking);
    }
}

void UExecutionComponent::DisableCharacterInput()
{
    if (!OwnerCharacter) return;
    
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        OwnerCharacter->DisableInput(PC);
    }
}

void UExecutionComponent::EnableCharacterInput()
{
    if (!OwnerCharacter) return;
    
    if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
    {
        OwnerCharacter->EnableInput(PC);
    }
}

void UExecutionComponent::DisableAI()
{
    if (!OwnerCharacter) return;
    
    if (ACAIController* AIC = Cast<ACAIController>(OwnerCharacter->GetController()))
    {
        AIC->StopMovement();
        
        if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
        {
            BrainComp->StopLogic(TEXT("Execution"));
        }
    }
}

void UExecutionComponent::EnableAI()
{
    if (!OwnerCharacter) return;
    
    if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
    {
        if (UBrainComponent* BrainComp = AIC->GetBrainComponent())
        {
            BrainComp->RestartLogic();
        }
    }
}

void UExecutionComponent::Server_RequestExecution_Implementation(AActor* Target, int32 MontageIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Server_RequestExecution received"));
    
    if (!Target || !OwnerCharacter || !AttackerMontages.IsValidIndex(MontageIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] Server validation failed"));
        return;
    }
    
    // 타겟의 ExecutionComponent 확인
    UExecutionComponent* TargetExecComp = Target->FindComponentByClass<UExecutionComponent>();
    if (!TargetExecComp || !TargetExecComp->CanBeExecuted())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Target cannot be executed"));
        return;
    }

    // ✅ 디버그: 피해자 몽타주 개수 확인
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] VictimMontages count: %d"), 
        TargetExecComp->VictimMontages.Num());
    
    
    // 거리 체크
    float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
    if (Distance > ExecutionRange)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Target out of range: %.1f > %.1f"), 
            Distance, ExecutionRange);
        return;
    }
    
    // 처형 상태 설정
    CurrentRole = EExecutionRole::Attacker;
    CurrentTarget = Target;
    TargetExecComp->SetVictim(OwnerCharacter);
    
    // ✅ 피해자 몽타주도 동일한 인덱스 사용 (페어링)
    // 만약 피해자 몽타주가 공격자보다 적으면 마지막 것 사용
    int32 VictimMontageIndex = FMath::Min(MontageIndex, TargetExecComp->VictimMontages.Num() - 1);
    
    UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Selected VictimMontageIndex: %d"), 
        VictimMontageIndex);
    
    // 모든 클라이언트에 브로드캐스트
    Multicast_PlayExecution(OwnerCharacter, Target, MontageIndex, VictimMontageIndex);
}

void UExecutionComponent::Multicast_PlayExecution_Implementation(AActor* Attacker, AActor* Victim,
    int32 AttackerMontageIndex, int32 VictimMontageIndex)
{
      UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Multicast: AttackerIndex=%d, VictimIndex=%d"), 
        AttackerMontageIndex, VictimMontageIndex);
    
    if (!Attacker || !Victim)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] Multicast: Attacker or Victim is NULL"));
        return;
    }
    
    // 공격자 컴포넌트
    UExecutionComponent* AttackerComp = Attacker->FindComponentByClass<UExecutionComponent>();
    if (!AttackerComp || !AttackerComp->AttackerMontages.IsValidIndex(AttackerMontageIndex))
    {
        return;
    }
    
    // 피해자 컴포넌트
    UExecutionComponent* VictimComp = Victim->FindComponentByClass<UExecutionComponent>();
    if (!VictimComp)
    {
        return;
    }
    
    // ✅ 1단계: 공격자를 피해자 방향으로 즉시 회전
    ACharacter* AttackerChar = Cast<ACharacter>(Attacker);
    if (AttackerChar)
    {
        FVector ToVictim = (Victim->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
        FRotator LookAtRotation = ToVictim.Rotation();
        
        // Z축(Yaw)만 회전 (Pitch, Roll은 유지)
        FRotator NewRotation = FRotator(0.f, LookAtRotation.Yaw, 0.f);
        AttackerChar->SetActorRotation(NewRotation);
        
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Attacker rotated to face victim: Yaw=%.1f"), 
            LookAtRotation.Yaw);
    }
    
    // ✅ 2단계: 모션 워핑 설정 (애니메이션 재생 전에!)
    AttackerComp->SetupMotionWarping(Victim);
    
    // ✅ 3단계: 공격자 AnimInstance 가져오기
    USkeletalMeshComponent* AttackerMesh = AttackerChar->GetMesh();
    if (!AttackerMesh) return;
    
    UAnimInstance* AttackerAnimInstance = AttackerMesh->GetAnimInstance();
    if (!AttackerAnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExecutionComponent] Multicast: Attacker AnimInstance is NULL"));
        return;
    }
    
    // ✅ 4단계: 공격자 애니메이션 재생
    UAnimMontage* AttackerMontage = AttackerComp->AttackerMontages[AttackerMontageIndex];
    if (AttackerMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExecutionComponent] Playing Attacker Montage [%d]: %s"), 
            AttackerMontageIndex, *AttackerMontage->GetName());
        
        AttackerAnimInstance->Montage_Play(AttackerMontage);
        
        // 몽타주 종료 델리게이트
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(AttackerComp, &UExecutionComponent::OnAttackerMontageEnded);
        AttackerAnimInstance->Montage_SetEndDelegate(EndDelegate, AttackerMontage);
    }
    
    // ✅ 5단계: 공격자 움직임 비활성화
    AttackerComp->DisableCharacterMovement();
    AttackerComp->DisableCharacterInput();
    
    // ✅ 6단계: 피해자 몽타주 재생
    VictimComp->PlayVictimMontage(VictimMontageIndex, Attacker);
}

void UExecutionComponent::OnAttackerMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Attacker montage ended (Interrupted: %d)"), bInterrupted);
    
    // 상태 리셋
    CurrentRole = EExecutionRole::None;
    AActor* VictimActor = CurrentTarget;
    CurrentTarget = nullptr;
    
    // 움직임 재활성화
    EnableCharacterMovement();
    EnableCharacterInput();
    
    // 서버에서만 데미지 처리
    if (OwnerCharacter && OwnerCharacter->HasAuthority() && VictimActor)
    {
        // 피해자에게 치명타 데미지
        IAbilitySystemInterface* VictimASI = Cast<IAbilitySystemInterface>(VictimActor);
        if (VictimASI)
        {
            if (UCAbilitySystemComponent* VictimASC = Cast<UCAbilitySystemComponent>(VictimASI->GetAbilitySystemComponent()))
            {
                // 즉사 처리
                VictimASC->ApplyDeath();
                
                UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Applied death to victim: %s"), 
                    *VictimActor->GetName());
            }
        }
    }
}

void UExecutionComponent::OnVictimMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    UE_LOG(LogTemp, Log, TEXT("[ExecutionComponent] Victim montage ended (Interrupted: %d)"), bInterrupted);
    
    // 상태 리셋
    CurrentRole = EExecutionRole::None;
    CurrentTarget = nullptr;
    
    // 움직임 재활성화 (죽었다면 의미없지만 안전하게)
    EnableCharacterMovement();
    EnableCharacterInput();
    EnableAI();
}
