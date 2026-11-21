// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_AttackMelee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "MotionWarpingComponent.h"
#include "AnimNotifyState_MotionWarping.h"
#include "RootMotionModifier.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/LeeTargetSelectionComponent.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "LeeHitOverride.h"

ULeeGameplayAbility_AttackMelee::ULeeGameplayAbility_AttackMelee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 26.11.02 수정 - ServerInitiated → LocalPredicted
	// 클라이언트가 입력 즉시 몽타주(루트모션·워프)를 재생해야 서버 위치 보정으로 인한 끊김이 없다.
	// 데미지(Trace)·스태미나 차감은 아래 각 함수에서 서버 전용으로 유지한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationGroup   = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// 어빌리티가 활성 동안 ASC에 자동 부여될 태그 (EndAbility 시 GAS가 자동 제거)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);

	// 공격 활성 내내(다단히트 콤보 전체 동안) 스태미나 회복 정지.
	// EndAbility 시 이 태그는 자동 제거되지만, GE_StaminaRegenDelay가 이어받아 종료 후에도 지연을 유지한다.
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Stamina_RegenBlocked);

	// TryActivateAbilitiesByTag 호출 시 이 태그로 매칭됨
	AbilityTags.AddTag(MyTags::Souls::Status_Attack_Melee);

	// 26.11.13 추가 - 그로기 중에는 공격을 시작하지 않는다.
	// (적 StateTree에는 그로기 검사가 없어, 그로기 중 스태미나가 차오르면 공격이 그로기 몽타주를 끊었다.
	//  플레이어는 입력 단계에서 이미 막고 있어 동작 변화 없음)
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);

	// 26.11.06 추가 - 부활 중 공격 입력 → 부활을 끊고 공격 (적은 부활 어빌리티가 없어 영향 없음)
	CancelAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Revive);

	// ANS_ToggleTrace와의 약속 — 기존 코드 그대로 재사용
	TraceEventTag = MyTags::Abilities::Enemy::Trace;

	// 공격 단계 비용 이벤트 태그 기본값 설정
	AttackStepEventTag = MyTags::Souls::Event_Attack_CommitStep;

	// [모션워핑] Warp 타깃 재계산 이벤트 태그 기본값
	RefreshWarpTargetEventTag = MyTags::Souls::Event_Attack_RefreshWarpTarget;
}

void ULeeGameplayAbility_AttackMelee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 이번 활성화가 실제 공격 시작(쿨다운 커밋 성공)까지 도달했는지 여부 — EndAbility의 회복지연 적용 조건으로 쓰인다
	bAttackActuallyStarted = false;

	// 아바타 유효성 검사
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 공격 데이터 검증 — Commit(비용 소모) 이전에 끝내야 데이터 오류로 비용이 낭비되지 않는다
	if (!ValidateAttackDataList())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] AttackDataList 검증 실패. BP에서 공격 데이터를 확인해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FLeeMeleeAttackData* SelectedAttackData = SelectAttackData();
	if (!SelectedAttackData || !SelectedAttackData->Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] 선택된 공격 데이터가 유효하지 않거나 몽타주가 null입니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// [쿨타임] 파생 클래스(Player 콤보 등)가 쿨타임 중인 데이터를 골랐을 수 있으므로 한 번 더 확인
	if (IsAttackDataOnCooldown(*SelectedAttackData, GetAbilitySystemComponentFromActorInfo()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CurrentAttackData = *SelectedAttackData;

	// 2. 공격 단계 상태 초기화
	CurrentAttackStepIndex = 0;
	CommittedAttackSteps.Empty();

	// 2-A. 몽타주 재생 전 스태미나 사전 검사.
	//      첫 단계 비용조차 지불 못하면(스태미나 0 포함) 몽타주를 아예 재생하지 않는다.
	//      (기존엔 재생 후 CommitStep 이벤트에서 부족을 감지해 몽타주가 중간에 취소되며 헛스윙처럼 보이던 것을 원천 차단)
	//      쿨다운 Commit 이전에 검사하여, 부족 시 쿨다운도 낭비하지 않는다.
	// 26.11.17 수정 - 첫 타 비용 대신 GetStaminaRequiredToStart(Enemy = 모든 타 합계, Player = 첫 타)로 검사
	{
		UAbilitySystemComponent* CostASC = GetAbilitySystemComponentFromActorInfo();
		const float CurrentStamina = CostASC ? CostASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) : 0.0f;
		const float RequiredStamina = GetStaminaRequiredToStart(CurrentAttackData);
		if (RequiredStamina > 0.0f && CurrentStamina < RequiredStamina)
		{
			UE_LOG(LogTemp, Log, TEXT("[LeeGA_AttackMelee] 스태미나 부족 (현재: %.1f / 필요: %.1f) — 공격 몽타주 미재생"),
				CurrentStamina, RequiredStamina);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 3. Cooldown만 Commit — Cost는 공격 단계 이벤트에서 단계마다 별도 적용
	//    (기존 CommitAbility()는 Cost+Cooldown을 동시에 처리하므로 여기서는 사용하지 않음)
	//    ForceCooldown=true이므로 현재 구현상 항상 true를 반환하지만, 향후 false로 바뀌는 경우에 대비해 방어적으로 검사한다
	if (!CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] 쿨다운 커밋 실패 — 공격 취소"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 여기까지 왔다면 실제 공격이 시작된 것으로 간주 — EndAbility에서 회복지연 GE를 적용할 근거가 된다
	bAttackActuallyStarted = true;

	// 4-A. Attack.CommitStep 이벤트 대기 — 공격 단계별 스태미나 비용 처리용
	//      반드시 Trace 대기보다 먼저, PlayMontageAndWait보다도 먼저 등록해야 한다
	if (AttackStepEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* StepTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				AttackStepEventTag,
				/*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false,
				/*OnlyMatchExact*/true);

		StepTask->EventReceived.AddDynamic(this, &ThisClass::OnAttackStepEventReceived);
		StepTask->ReadyForActivation();
	}

	// 4-B. ANS_ToggleTrace로부터 HitResult 이벤트 대기 — 몽타주보다 먼저 등록해야 시작 프레임 트레이스 유실을 막는다
	//      OnlyTriggerOnce=false: 한 공격 모션 내에서 여러 타겟/여러 구간 히트 허용 (콤보 몽타주의 다단히트 포함)
	if (TraceEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				TraceEventTag,
				/*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false,
				/*OnlyMatchExact*/true);

		EventTask->EventReceived.AddDynamic(this, &ThisClass::OnTraceEventReceived);
		EventTask->ReadyForActivation();
	}

	// 4-C. [모션워핑] RefreshWarpTarget 이벤트 대기 — 다단 공격에서 2번째+ Warp 구간 직전에
	//      몽타주 Notify가 이 이벤트를 발사하면, 그 시점의 대상 위치로 워프 타깃을 다시 계산한다.
	//      OnlyTriggerOnce=false: 한 몽타주에서 여러 Warp 구간을 위해 반복 수신.
	if (RefreshWarpTargetEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* RefreshTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				RefreshWarpTargetEventTag,
				/*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false,
				/*OnlyMatchExact*/true);

		RefreshTask->EventReceived.AddDynamic(this, &ThisClass::OnRefreshWarpTargetEventReceived);
		RefreshTask->ReadyForActivation();
	}

	// 5. 몽타주 재생 (Player 파생 클래스는 이 함수를 override하여 콤보 Task를 먼저 등록한다)
	PlayAttackMontage(CurrentAttackData.Montage, NAME_None);
}

// 몽타주 재생 태스크 생성/델리게이트 연결 — Enemy는 이 기본 구현 그대로 사용
void ULeeGameplayAbility_AttackMelee::PlayAttackMontage(UAnimMontage* Montage, FName StartSection)
{
	// 몽타주 재생 직전, 공격 대상 기준으로 워프 타깃을 갱신 (하이브리드: CurrentAttackData.bWarpTranslation 참조)
	UpdateAttackWarpTarget();

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, /*PlayRate*/1.0f, StartSection);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// 공격 데이터 목록 검증 — 기본은 "비어있지 않은가"만 확인 (Enemy는 여러 개 중 랜덤 선택)
bool ULeeGameplayAbility_AttackMelee::ValidateAttackDataList() const
{
	return !AttackDataList.IsEmpty();
}

// 공격 데이터 선택 — 기본(Enemy)은 쿨타임이 아닌 데이터 중 랜덤 1개. Player는 콤보 첫 타로 override.
const FLeeMeleeAttackData* ULeeGameplayAbility_AttackMelee::SelectAttackData()
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	// 26.11.17 수정 - 쿨타임이 아니고 + 끝까지 휘두를 스태미나가 있는 공격만 후보로 둔다
	// (여러 공격 중 랜덤으로 비싼 것이 골라져 실패하는 대신, 지금 할 수 있는 공격을 고른다)
	const float CurrentStamina = ASC ? ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) : 0.0f;
	TArray<int32> AvailableIndices;
	for (int32 Index = 0; Index < AttackDataList.Num(); ++Index)
	{
		if (!IsAttackDataOnCooldown(AttackDataList[Index], ASC)
			&& CurrentStamina >= GetStaminaRequiredToStart(AttackDataList[Index]))
		{
			AvailableIndices.Add(Index);
		}
	}

	if (AvailableIndices.IsEmpty())
	{
		return nullptr;
	}
	return &AttackDataList[AvailableIndices[FMath::RandRange(0, AvailableIndices.Num() - 1)]];
}

// [쿨타임] 공격 데이터 중 하나라도 쿨타임이 아니면 활성화 가능 — 모두 쿨타임이면 TryActivateAbility 단계에서 실패시킨다
bool ULeeGameplayAbility_AttackMelee::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 기본 GE 기반 쿨타임 검사도 유지 (공용 쿨다운 GE는 태그를 부여하지 않으므로 보통 통과)
	if (!Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || AttackDataList.IsEmpty())
	{
		return true;
	}

	for (const FLeeMeleeAttackData& AttackData : AttackDataList)
	{
		if (!IsAttackDataOnCooldown(AttackData, ASC))
		{
			return true;
		}
	}

	// 전부 쿨타임 — 실패 사유로 각 데이터의 쿨다운 태그를 알려준다
	if (OptionalRelevantTags)
	{
		for (const FLeeMeleeAttackData& AttackData : AttackDataList)
		{
			OptionalRelevantTags->AddTag(AttackData.CooldownTag);
		}
	}
	return false;
}

// [쿨타임] 이번에 선택된 몽타주(CurrentAttackData)의 시간/태그로 공용 쿨다운 GE를 적용
void ULeeGameplayAbility_AttackMelee::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || CurrentAttackData.CooldownDuration <= 0.0f || !CurrentAttackData.CooldownTag.IsValid())
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle =
		MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 몽타주 전용 태그를 GE가 부여하게 하고, 지속시간은 SetByCaller로 넘긴다 (GE 에셋 1개를 모든 근접 GA가 공유)
	SpecHandle.Data->DynamicGrantedTags.AddTag(CurrentAttackData.CooldownTag);
	SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, CurrentAttackData.CooldownDuration);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

// [쿨타임] 공격 데이터의 쿨다운 태그가 ASC에 남아 있는지 확인
bool ULeeGameplayAbility_AttackMelee::IsAttackDataOnCooldown(const FLeeMeleeAttackData& AttackData, const UAbilitySystemComponent* ASC)
{
	return ASC && AttackData.CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(AttackData.CooldownTag);
}

// [모션워핑] 워프가 바라볼 대상 — 기본(Enemy): AIController의 네이티브 Focus를 그대로 읽는다.
// Enemy AIController(BP, StateTree 기반)가 Perception으로 얻은 타깃을 자체 변수에 저장했다가
// STT_SetFocus 태스크가 AIController->SetFocus()로 반영하므로, GetFocusActor()가 그 값을 그대로 돌려준다.
AActor* ULeeGameplayAbility_AttackMelee::GetWarpFacingTarget() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIC = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	if (!AIC)
	{
		return nullptr;
	}

	if (AActor* FocusActor = AIC->GetFocusActor())
	{
		return FocusActor;
	}

	// 26.11.22 수정 - Focus가 비어 있으면 AI가 고른 전투 타겟을 쓴다.
	// 공격 상태에 FaceTarget 태스크가 없는 StateTree는 직전 상태가 나가며 ClearFocus()해 Focus가 비고,
	// 그러면 워프 대상이 없어 공격 중 타겟 쪽으로 회전하지 않았다 (신오비 콤보에서 확인).
	const ULeeTargetSelectionComponent* Selection = ULeeTargetSelectionComponent::FindTargetSelectionComponent(AIC);
	if (!Selection)
	{
		Selection = ULeeTargetSelectionComponent::FindTargetSelectionComponent(Pawn);
	}
	return Selection ? Selection->GetCurrentTarget() : nullptr;
}

// [모션워핑] GetWarpFacingTarget() 대상 기준으로 CurrentAttackData.bWarpTranslation에 따라 워프 타깃을 갱신한다 (하이브리드 방식 A/B 분기)
void ULeeGameplayAbility_AttackMelee::UpdateAttackWarpTarget()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UMotionWarpingComponent* MW = Avatar ? Avatar->FindComponentByClass<UMotionWarpingComponent>() : nullptr;

	// 26.11.22 추가 - [AttackWarp] 진단 로그: 워프 대상 계산 결과 (적 공격 중 회전이 안 되는 문제 추적용)
	AActor* Target = MW ? GetWarpFacingTarget() : nullptr;
	UE_LOG(LogLee, Log, TEXT("[AttackWarp] %s Montage=%s MW=%d Target=%s WarpTranslation=%d Dist=%.0f ActorYaw=%.0f ToTargetYaw=%.0f"),
		*GetNameSafe(Avatar), *GetNameSafe(CurrentAttackData.Montage), MW != nullptr, *GetNameSafe(Target),
		CurrentAttackData.bWarpTranslation,
		(Avatar && Target) ? FVector::Dist2D(Avatar->GetActorLocation(), Target->GetActorLocation()) : -1.0f,
		Avatar ? Avatar->GetActorRotation().Yaw : 0.0f,
		(Avatar && Target) ? (Target->GetActorLocation() - Avatar->GetActorLocation()).Rotation().Yaw : 0.0f);

	if (!MW)
	{
		return;
	}

	if (!Target)
	{
		// 대상은 없지만 대체 방향(Player = 이동 입력 방향)이 있으면 그 방향으로 회전 워프.
		// 위치는 "원본 루트모션이 도착할 지점을 그 방향으로 돌린 곳" — 현재 위치를 넣으면 루트모션 이동이 상쇄되어 제자리/공중에서 멈춘다
		FVector FallbackDir;
		if (GetWarpFallbackDirection(FallbackDir))
		{
			const FRotator FacingRot = FallbackDir.Rotation();
			const FVector WarpLoc = Avatar->GetActorLocation() + GetRootMotionOffsetToWarpEnd(FacingRot);
			MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLoc, FacingRot);
			return;
		}

		// 워프 대상 없음 — 이전 타깃 제거하여 제자리 공격(워프 없음)으로 안전 폴백
		MW->RemoveWarpTarget(WarpTargetName);
		return;
	}

	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector ToTargetDir = (TargetLoc - AvatarLoc).GetSafeNormal2D();

	if (!CurrentAttackData.bWarpTranslation)
	{
		// 방식 B: 회전만 — 현재 위치 그대로, 방향만 대상 쪽으로
		MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, AvatarLoc, ToTargetDir.Rotation());

		const FMotionWarpingTarget* Registered = MW->FindWarpTarget(WarpTargetName);
		
		return;
	}

	// 방식 A: 위치 + 회전
	const float DistToTarget = FVector::Dist2D(AvatarLoc, TargetLoc);

	// 이미 접근거리 안이면(너무 가까우면) 뒤로 끌려가지 않도록 현재 위치 유지
	const FVector ApproachLoc = TargetLoc - ToTargetDir * CurrentAttackData.ApproachDistance;
	const bool bAlreadyClose = (DistToTarget <= CurrentAttackData.ApproachDistance);
	FVector FinalLoc = bAlreadyClose ? AvatarLoc : ApproachLoc;

	// 26.11.22 수정 - 최대 이동 거리 제한: 목표 지점이 MaxWarpDistance보다 멀면 타겟 방향으로 그만큼만 다가간다.
	// 이전에는 워프 타깃을 통째로 지워서 회전까지 꺼졌다 (신오비 콤보가 타겟 쪽으로 돌지 않던 원인).
	if (CurrentAttackData.MaxWarpDistance > 0.0f
		&& FVector::Dist2D(AvatarLoc, FinalLoc) > CurrentAttackData.MaxWarpDistance)
	{
		FinalLoc = AvatarLoc + ToTargetDir * CurrentAttackData.MaxWarpDistance;
	}

	MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, FinalLoc, ToTargetDir.Rotation());

	// (몽타주 노티파이의 Warp Target Name과 여기 WarpTargetName 문자열이 다르면 FindWarpTarget이 못찾음 -> 노티파이가 조용히 무시됨)
	const FMotionWarpingTarget* Registered = MW->FindWarpTarget(WarpTargetName);
	UE_LOG(LogLee, Log, TEXT("[AttackWarp] %s 워프 타깃 등록=%d Name=%s Loc=%s Yaw=%.0f AlreadyClose=%d"),
		*GetNameSafe(Avatar), Registered != nullptr, *WarpTargetName.ToString(), *FinalLoc.ToCompactString(),
		ToTargetDir.Rotation().Yaw, bAlreadyClose);
}

// [모션워핑] 현재 몽타주 위치 ~ 다음 이동 워프 구간 끝까지의 원본 루트모션 이동량을 FacingRotation 기준 월드 오프셋으로 변환
FVector ULeeGameplayAbility_AttackMelee::GetRootMotionOffsetToWarpEnd(const FRotator& FacingRotation) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UAnimMontage* Montage = CurrentAttackData.Montage;
	if (!Character || !Montage || !Character->GetMesh())
	{
		return FVector::ZeroVector;
	}

	// 재생 직전(아직 이 몽타주가 안 돌고 있음)이면 0초, 재생 중(Refresh 이벤트)이면 현재 위치부터 계산
	float FromTime = 0.0f;
	if (const UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
	{
		if (AnimInstance->Montage_IsPlaying(Montage))
		{
			FromTime = AnimInstance->Montage_GetPosition(Montage);
		}
	}

	// 아직 끝나지 않은 워프 구간 중 가장 먼저 시작하는, 같은 타깃 이름 + 이동 워프인 구간을 찾는다
	float WindowStart = TNumericLimits<float>::Max();
	float WindowEnd = -1.0f;
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		const UAnimNotifyState_MotionWarping* WarpNotify = Cast<UAnimNotifyState_MotionWarping>(NotifyEvent.NotifyStateClass);
		const URootMotionModifier_Warp* Modifier = WarpNotify ? Cast<URootMotionModifier_Warp>(WarpNotify->RootMotionModifier) : nullptr;
		if (!Modifier || Modifier->WarpTargetName != WarpTargetName || !Modifier->bWarpTranslation)
		{
			continue;
		}

		const float Start = NotifyEvent.GetTriggerTime();
		const float End = NotifyEvent.GetEndTriggerTime();
		if (End > FromTime && Start < WindowStart)
		{
			WindowStart = Start;
			WindowEnd = End;
		}
	}

	if (WindowEnd < 0.0f)
	{
		return FVector::ZeroVector;
	}

	// 루트 본 공간 이동량 → 액터 공간(메시 회전 오프셋 반영) → 바라볼 방향으로 회전
	const FTransform RootMotion = UMotionWarpingUtilities::ExtractRootMotionFromAnimation(Montage, FromTime, WindowEnd);
	const FVector ActorSpaceDelta = Character->GetBaseRotationOffset().RotateVector(RootMotion.GetTranslation());
	return FacingRotation.RotateVector(ActorSpaceDelta);
}

// [모션워핑] RefreshWarpTarget 이벤트 수신 — 그 시점의 대상 위치로 워프 타깃을 다시 계산한다.
// 다단 공격에서 두 번째+ Warp 구간이 몽타주 시작 때의 낡은 위치가 아니라 최신 위치를 향하게 한다.
void ULeeGameplayAbility_AttackMelee::OnRefreshWarpTargetEventReceived(FGameplayEventData Payload)
{
	// 26.11.02 수정 - 서버 + 로컬 클라이언트 양쪽에서 갱신 (LocalPredicted: 양쪽 몽타주의 워프 이동량을 맞춘다)
	const bool bServerOrLocal = GetActorInfo().IsNetAuthority() || GetActorInfo().IsLocallyControlled();
	UE_LOG(LogLee, Log, TEXT("[AttackWarp] %s RefreshWarpTarget 이벤트 수신 ServerOrLocal=%d Active=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), bServerOrLocal, IsActive());
	if (!bServerOrLocal || !IsActive())
	{
		return;
	}

	UpdateAttackWarpTarget();
}

void ULeeGameplayAbility_AttackMelee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 중복 EndAbility 호출 방지 — 이미 종료 처리된 경우 회복지연 GE를 두 번 적용하지 않는다
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	// [모션워핑] 워프 타깃 잔류 방지 — Enemy·Player 공통 정리
	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UMotionWarpingComponent* MW = Avatar->FindComponentByClass<UMotionWarpingComponent>())
		{
			MW->RemoveWarpTarget(WarpTargetName);
		}
	}

	// 다음 활성화에서 이전 공격 데이터/단계 기록이 남아있지 않도록 초기화
	CurrentAttackData = FLeeMeleeAttackData();
	CurrentAttackStepIndex = 0;
	CommittedAttackSteps.Empty();

	// [타격별 공격 속성] 몽타주가 비정상 종료돼 Telegraph NotifyEnd가 불리지 않아도 다음 공격에 속성 태그가 남지 않도록 초기화
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->SetLooseGameplayTagCount(MyTags::Souls::DamageType_Attack_Unblockable, 0);
		ASC->SetLooseGameplayTagCount(MyTags::Souls::DamageType_Attack_PerfectGuardOnly, 0);
	}

	// 공격 종료 후 일정 시간 동안 스태미나 회복을 지연 (RegenBlocked 태그를 Duration GE로 유지)
	// 실제로 공격이 시작된 경우(쿨다운 커밋 성공)에만 적용한다 — 데이터 검증 실패 등으로
	// 몽타주가 재생되지도 못하고 끝난 경우까지 회복을 지연시키는 것은 의도한 동작이 아니다
	if (bAttackActuallyStarted)
	{
		ApplyDurationEffect(StaminaRegenDelayEffect, StaminaRegenDelayDuration);
	}
	bAttackActuallyStarted = false;

	// ActivationOwnedTags의 Status_Attack_Attacking은 GAS가 여기서 자동 제거
	// StateTree Task가 이 태그 제거를 감지하여 FinishTask를 호출함
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 26.11.17 추가 - 공격 시작에 필요한 스태미나 — 기본(Enemy)은 이 몽타주 모든 타 비용의 합
float ULeeGameplayAbility_AttackMelee::GetStaminaRequiredToStart(const FLeeMeleeAttackData& AttackData) const
{
	float Total = 0.0f;
	for (const float StepCost : AttackData.StaminaCostPerStep)
	{
		Total += StepCost;
	}
	return Total;
}

// 스태미나 비용 검사/차감 — Enemy(CostIndex==DedupKey)/Player(CostIndex=0, DedupKey=콤보 인덱스) 공용
bool ULeeGameplayAbility_AttackMelee::TryCommitAttackStepCost(int32 CostIndex, int32 DedupKey)
{
	// 비용 배열 범위를 벗어나면 무시 (배열 개수 < Notify 개수인 경우)
	if (!CurrentAttackData.StaminaCostPerStep.IsValidIndex(CostIndex))
	{
		return false;
	}

	// 중복 방지 — Section Loop/네트워크 보정 등으로 같은 Notify가 재실행될 수 있음
	if (CommittedAttackSteps.Contains(DedupKey))
	{
		return false;
	}

	float StepCost = CurrentAttackData.StaminaCostPerStep[CostIndex];

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	// 현재 스태미나 값을 읽어 비용 지불 가능 여부 검사
	const float CurrentStamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
	if (CurrentStamina < StepCost)
	{
		if (ShouldEndOnStaminaShortage())
		{
			// [Player] 스태미나 부족 — 다음 Trace 구간 시작 전에 즉시 어빌리티(+몽타주) 종료
			UE_LOG(LogTemp, Log, TEXT("[LeeGA_AttackMelee] 스태미나 부족 (현재: %.1f / 필요: %.1f) — 공격 단계 %d 취소"),
				CurrentStamina, StepCost, DedupKey);
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
			return false;
		}

		// 26.11.17 추가 - [Enemy] 시작할 땐 충분했지만 도중에(피격·패링 등) 줄었다 → 끊지 않고 남은 만큼만 뺀다
		UE_LOG(LogTemp, Log, TEXT("[LeeGA_AttackMelee] 스태미나 부족 (현재: %.1f / 필요: %.1f) — 공격 단계 %d 계속, 남은 만큼만 차감"),
			CurrentStamina, StepCost, DedupKey);
		StepCost = FMath::Max(CurrentStamina, 0.0f);
		if (StepCost <= 0.0f)
		{
			// 이미 0 — 뺄 것이 없으므로 이 단계는 처리한 것으로만 기록
			CommittedAttackSteps.Add(DedupKey);
			return true;
		}
	}

	// 스태미나 차감 GE 적용 (공격자 자신에게)
	if (AttackStepStaminaCostEffect)
	{
		FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(AttackStepStaminaCostEffect, /*Level*/1.0f, ASC->MakeEffectContext());

		if (SpecHandle.IsValid())
		{
			// 음수로 설정하여 스태미나 감소
			SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_StaminaDamage, -StepCost);
			SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::StaminaCause_AttackCost);
			// 26.11.17 추가 - [Enemy] 끝까지 휘두르는 공격이므로 자기 공격 비용으로 0이 되어도 그로기에 빠지지 않는다
			if (!ShouldEndOnStaminaShortage())
			{
				SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::StaminaCause_NoGroggy);
			}
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] AttackStepStaminaCostEffect가 설정되지 않음. BP에서 GE_AttackStepStaminaCost를 지정해주세요."));
	}

	// 이번 단계 처리 완료 기록
	CommittedAttackSteps.Add(DedupKey);
	return true;
}

void ULeeGameplayAbility_AttackMelee::OnAttackStepEventReceived(FGameplayEventData Payload)
{
	// 서버 전용 — 클라이언트 AnimNotify 중복 실행 방지
	if (!GetActorInfo().IsNetAuthority())
	{
		return;
	}

	// 어빌리티가 이미 종료된 상태면 무시
	if (!IsActive())
	{
		return;
	}

	// Enemy는 단일 몽타주라 비용 인덱스와 중복키가 동일한 자동 증가 인덱스를 사용한다
	// (기존 동작과 완전히 동일: 범위 밖/중복/스태미나 부족 시에는 증가하지 않음)
	if (TryCommitAttackStepCost(CurrentAttackStepIndex, CurrentAttackStepIndex))
	{
		CurrentAttackStepIndex++;
	}
}

void ULeeGameplayAbility_AttackMelee::OnTraceEventReceived(FGameplayEventData Payload)
{
	// 26.11.02 추가 - 데미지는 서버에서만 적용 (LocalPredicted로 클라이언트에서도 어빌리티가 돌기 때문)
	if (!GetActorInfo().IsNetAuthority())
	{
		return;
	}

	// Payload.Instigator: ANS_ToggleTrace가 설정하는 공격자 (GetOwner() 아님)
	AActor* AttackerActor = const_cast<AActor*>(Payload.Instigator.Get());
	if (!AttackerActor)
	{
		return;
	}

	// TargetData에 HitResult가 없으면 처리하지 않음
	if (!UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(Payload.TargetData, 0))
	{
		return;
	}

	const FHitResult HitResult =
		UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Payload.TargetData, 0);

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || HitActor == AttackerActor)
	{
		return;
	}

	// 데미지 GE가 설정되지 않았으면 처리 생략
	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] DamageEffect가 설정되지 않음. BP에서 GE_MeleeDamage를 지정해주세요."));
		return;
	}

	UAbilitySystemComponent* AttackerASC = GetActorInfo().AbilitySystemComponent.Get();
	UAbilitySystemComponent* HitASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

	if (!AttackerASC || !HitASC)
	{
		return;
	}

	// [타격별 Override] 판정 노티파이가 자신을 OptionalObject로 실어 보냈다면, 체크된 항목만 그 값으로 바꾼다
	const ILeeHitOverrideProvider* HitOverrideProvider = Cast<ILeeHitOverrideProvider>(Payload.OptionalObject.Get());
	const FLeeHitOverride* HitOverride = HitOverrideProvider ? &HitOverrideProvider->GetHitOverride() : nullptr;
	const float HitDamage = (HitOverride && HitOverride->bOverrideDamage) ? HitOverride->BaseDamage : CurrentAttackData.BaseDamage;
	const FLeeKnockbackParams& HitKnockback = (HitOverride && HitOverride->bOverrideKnockback) ? HitOverride->Knockback : CurrentAttackData.Knockback;
	const FLeeHitStopParams& HitStop = (HitOverride && HitOverride->bOverrideHitStop) ? HitOverride->HitStop : CurrentAttackData.HitStop;	// 26.11.12 추가

	// SetByCaller 방식으로 데미지 GE 적용
	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle =
		AttackerASC->MakeOutgoingSpec(DamageEffect, /*Level*/1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 음수로 설정하여 Health 감소 (AttributeSet의 PostGameplayEffectExecute에서 처리)
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -HitDamage);

		// 26.11.12 추가 - 히트스톱 값을 Spec에 싣는다. 실제 적용 여부(판정 결과·싱글 여부)는 피격자의 ULeeDefenseComponent가 정한다
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_HitStopDuration, HitStop.Duration);
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_HitStopTimeScale, HitStop.TimeScale);

		// 공격 속성 태그 전달 (DamageType_ParryCounter와 동일한 DynamicAssetTag 패턴)
		for (const FGameplayTag& DamageTypeTag : CurrentAttackData.DamageTypeTags)
		{
			SpecHandle.Data->AddDynamicAssetTag(DamageTypeTag);
		}

		// [타격별 공격 속성] Telegraph 노티파이가 공격자에게 건 Loose Tag를 이번 히트에만 추가
		const FGameplayTag PerHitTags[] = { MyTags::Souls::DamageType_Attack_Unblockable, MyTags::Souls::DamageType_Attack_PerfectGuardOnly };
		for (const FGameplayTag& PerHitTag : PerHitTags)
		{
			if (AttackerASC->GetTagCount(PerHitTag) > 0)
			{
				SpecHandle.Data->AddDynamicAssetTag(PerHitTag);
			}
		}

		HitASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	// [넉백] 서버에서만 보낸다. 데미지 적용 중 패리를 당하면 이 공격 어빌리티가 취소되므로(IsActive=false) 넉백도 생략된다
	if (GetActorInfo().IsNetAuthority() && IsActive())
	{
		ULeeGameplayAbility_Knockback::SendKnockbackEvent(AttackerActor, HitActor, HitKnockback, AttackerActor->GetActorRotation().Yaw);
	}
}

// SetByCaller Duration 방식 GE 적용 헬퍼 — BP 애셋의 Duration이 SetByCaller(Souls.SetByCaller.Duration)여야 한다
FActiveGameplayEffectHandle ULeeGameplayAbility_AttackMelee::ApplyDurationEffect(
	TSubclassOf<UGameplayEffect> EffectClass, float Duration)
{
	if (!EffectClass || Duration <= 0.0f)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, Duration);
	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}

void ULeeGameplayAbility_AttackMelee::OnMontageCompleted()
{
	// 정상 완료: 다음 공격 가능 상태로 복귀
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_AttackMelee::OnMontageInterrupted()
{
	// 외부 취소 (피격 그로기, 처형 등으로 인한 강제 종료)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
