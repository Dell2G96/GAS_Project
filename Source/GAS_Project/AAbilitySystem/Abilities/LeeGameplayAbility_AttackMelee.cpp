// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_AttackMelee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "MotionWarpingComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

ULeeGameplayAbility_AttackMelee::ULeeGameplayAbility_AttackMelee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup   = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// 어빌리티가 활성 동안 ASC에 자동 부여될 태그 (EndAbility 시 GAS가 자동 제거)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);

	// 공격 활성 내내(다단히트 콤보 전체 동안) 스태미나 회복 정지.
	// EndAbility 시 이 태그는 자동 제거되지만, GE_StaminaRegenDelay가 이어받아 종료 후에도 지연을 유지한다.
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Stamina_RegenBlocked);

	// TryActivateAbilitiesByTag 호출 시 이 태그로 매칭됨
	AbilityTags.AddTag(MyTags::Souls::Status_Attack_Melee);

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
	CurrentAttackData = *SelectedAttackData;

	// 2. 공격 단계 상태 초기화
	CurrentAttackStepIndex = 0;
	CommittedAttackSteps.Empty();

	// 2-A. 몽타주 재생 전 스태미나 사전 검사.
	//      첫 단계 비용조차 지불 못하면(스태미나 0 포함) 몽타주를 아예 재생하지 않는다.
	//      (기존엔 재생 후 CommitStep 이벤트에서 부족을 감지해 몽타주가 중간에 취소되며 헛스윙처럼 보이던 것을 원천 차단)
	//      쿨다운 Commit 이전에 검사하여, 부족 시 쿨다운도 낭비하지 않는다.
	if (CurrentAttackData.StaminaCostPerStep.IsValidIndex(0))
	{
		UAbilitySystemComponent* CostASC = GetAbilitySystemComponentFromActorInfo();
		const float CurrentStamina = CostASC ? CostASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) : 0.0f;
		const float FirstStepCost = CurrentAttackData.StaminaCostPerStep[0];
		if (CurrentStamina < FirstStepCost)
		{
			UE_LOG(LogTemp, Log, TEXT("[LeeGA_AttackMelee] 스태미나 부족 (현재: %.1f / 필요: %.1f) — 공격 몽타주 미재생"),
				CurrentStamina, FirstStepCost);
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

// 공격 데이터 선택 — 기본(Enemy)은 목록에서 랜덤 1개. Player는 콤보 첫 타로 override.
const FLeeMeleeAttackData* ULeeGameplayAbility_AttackMelee::SelectAttackData()
{
	if (AttackDataList.IsEmpty())
	{
		return nullptr;
	}
	return &AttackDataList[FMath::RandRange(0, AttackDataList.Num() - 1)];
}

// [모션워핑] 워프가 바라볼 대상 — 기본(Enemy): AIController의 네이티브 Focus를 그대로 읽는다.
// Enemy AIController(BP, StateTree 기반)가 Perception으로 얻은 타깃을 자체 변수에 저장했다가
// STT_SetFocus 태스크가 AIController->SetFocus()로 반영하므로, GetFocusActor()가 그 값을 그대로 돌려준다.
AActor* ULeeGameplayAbility_AttackMelee::GetWarpFacingTarget() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIC = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	return AIC ? AIC->GetFocusActor() : nullptr;
}

// [모션워핑] GetWarpFacingTarget() 대상 기준으로 CurrentAttackData.bWarpTranslation에 따라 워프 타깃을 갱신한다 (하이브리드 방식 A/B 분기)
void ULeeGameplayAbility_AttackMelee::UpdateAttackWarpTarget()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UMotionWarpingComponent* MW = Avatar ? Avatar->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!MW)
	{
		return;
	}

	AActor* Target = GetWarpFacingTarget();
	if (!Target)
	{
		// [디버그 로그] 임시 - 타깃 없음으로 워프 제거되는 경로인지 확인용
		UE_LOG(LogTemp, Log, TEXT("[Warp] %s: Target 없음 -> RemoveWarpTarget"), *GetNameSafe(Avatar));
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

		// [디버그 로그] 임시 - 등록에 쓴 이름(WarpTargetName)과 실제로 컴포넌트에 등록된 타깃 이름이 일치하는지 확인용
		const FMotionWarpingTarget* Registered = MW->FindWarpTarget(WarpTargetName);
		UE_LOG(LogTemp, Log,
			TEXT("[Warp] %s: bWarpTranslation=false -> 회전만 (Target=%s) | 등록에 쓴 이름=\"%s\" | FindWarpTarget 결과=%s"),
			*GetNameSafe(Avatar), *GetNameSafe(Target), *WarpTargetName.ToString(),
			Registered ? TEXT("찾음(등록 성공)") : TEXT("못찾음(이름 불일치 의심)"));
		return;
	}

	// 방식 A: 위치 + 회전 — 접근거리 밖이면 순간이동 방지를 위해 워프하지 않음
	const float DistToTarget = FVector::Dist2D(AvatarLoc, TargetLoc);
	if (CurrentAttackData.MaxWarpDistance > 0.0f && DistToTarget > CurrentAttackData.MaxWarpDistance)
	{
		// [디버그 로그] 임시 - MaxWarpDistance 초과로 워프가 취소되는 경로인지 확인용
		UE_LOG(LogTemp, Log, TEXT("[Warp] %s: DistToTarget(%.1f) > MaxWarpDistance(%.1f) -> RemoveWarpTarget"),
			*GetNameSafe(Avatar), DistToTarget, CurrentAttackData.MaxWarpDistance);
		MW->RemoveWarpTarget(WarpTargetName);
		return;
	}

	// 이미 접근거리 안이면(너무 가까우면) 뒤로 끌려가지 않도록 현재 위치 유지
	const FVector ApproachLoc = TargetLoc - ToTargetDir * CurrentAttackData.ApproachDistance;
	const bool bAlreadyClose = (DistToTarget <= CurrentAttackData.ApproachDistance);
	const FVector FinalLoc = bAlreadyClose ? AvatarLoc : ApproachLoc;

	MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, FinalLoc, ToTargetDir.Rotation());

	// [디버그 로그] 임시 - 방식 A 최종 분기 + "등록에 쓴 이름"과 "실제 컴포넌트에 등록된 타깃"이 일치하는지 확인용
	// (몽타주 노티파이의 Warp Target Name과 여기 WarpTargetName 문자열이 다르면 FindWarpTarget이 못찾음 -> 노티파이가 조용히 무시됨)
	const FMotionWarpingTarget* Registered = MW->FindWarpTarget(WarpTargetName);
	UE_LOG(LogTemp, Log,
		TEXT("[Warp] %s: bWarpTranslation=true, DistToTarget=%.1f, ApproachDistance=%.1f, bAlreadyClose=%s, FinalLoc=%s | 등록에 쓴 이름=\"%s\" | FindWarpTarget 결과=%s"),
		*GetNameSafe(Avatar), DistToTarget, CurrentAttackData.ApproachDistance,
		bAlreadyClose ? TEXT("true(제자리 유지)") : TEXT("false(접근 이동)"), *FinalLoc.ToString(),
		*WarpTargetName.ToString(),
		Registered ? TEXT("찾음(등록 성공)") : TEXT("못찾음(이름 불일치 의심)"));
}

// [모션워핑] RefreshWarpTarget 이벤트 수신 — 그 시점의 대상 위치로 워프 타깃을 다시 계산한다.
// 다단 공격에서 두 번째+ Warp 구간이 몽타주 시작 때의 낡은 위치가 아니라 최신 위치를 향하게 한다.
void ULeeGameplayAbility_AttackMelee::OnRefreshWarpTargetEventReceived(FGameplayEventData Payload)
{
	// 서버 전용 — Warp 등록은 어빌리티가 도는 서버 권위 인스턴스에서 처리 (초기 등록과 동일 규약)
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
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

	const float StepCost = CurrentAttackData.StaminaCostPerStep[CostIndex];

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	// 현재 스태미나 값을 읽어 비용 지불 가능 여부 검사
	const float CurrentStamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
	if (CurrentStamina < StepCost)
	{
		// 스태미나 부족 — 다음 Trace 구간 시작 전에 즉시 어빌리티(+몽타주) 종료
		UE_LOG(LogTemp, Log, TEXT("[LeeGA_AttackMelee] 스태미나 부족 (현재: %.1f / 필요: %.1f) — 공격 단계 %d 취소"),
			CurrentStamina, StepCost, DedupKey);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
		return false;
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

	// SetByCaller 방식으로 데미지 GE 적용
	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle =
		AttackerASC->MakeOutgoingSpec(DamageEffect, /*Level*/1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 음수로 설정하여 Health 감소 (AttributeSet의 PostGameplayEffectExecute에서 처리)
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -CurrentAttackData.BaseDamage);

		// 공격 속성 태그 전달 (DamageType_ParryCounter와 동일한 DynamicAssetTag 패턴)
		for (const FGameplayTag& DamageTypeTag : CurrentAttackData.DamageTypeTags)
		{
			SpecHandle.Data->AddDynamicAssetTag(DamageTypeTag);
		}

		HitASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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
