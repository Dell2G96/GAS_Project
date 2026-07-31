 
 #include "LeeDefenseComponent.h"
 
 #include "AbilitySystemBlueprintLibrary.h"
 #include "AbilitySystemComponent.h"
 #include "TimerManager.h"
 #include "DrawDebugHelpers.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
 #include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
 #include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"


// // 생성자 — 판정 로직은 델리게이트 기반이라 틱 불필요. 가드 원뿔 디버그 시각화용으로만 틱 사용 가능
ULeeDefenseComponent::ULeeDefenseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// BeginPlay — 서버에서만 StatSet 델리게이트 바인딩 예약 + 디버그 켜져 있으면 틱 활성화
void ULeeDefenseComponent::BeginPlay()
{
	Super::BeginPlay();

	// ASC/AttributeSet 초기화가 오너 BeginPlay 이후일 수 있으므로 다음 틱에 바인딩 (FinisherTargetComponent와 동일 패턴)
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::BindToStatSetDelegates);
	}

	// [디버그] 가드 원뿔 시각화 — 켜져 있을 때만 틱을 돌린다 (연출은 로컬 뷰포트 기준으로 그려짐)
	SetComponentTickEnabled(bDrawGuardArcDebug);
}

// [디버그] 매 틱 — 가드 상태(Status.Guard.Active)일 때만 전방 삼각형을 그린다
void ULeeDefenseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDrawGuardArcDebug)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetOwnerASC();
	if (ASC && ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active))
	{
		DrawGuardArcDebug();
	}
}

// [디버그] Player(방어자)를 꼭짓점으로, 전방(ActorForward) 기준 ±GuardValidAngleDeg(half-angle) 방향의
//  두 변을 뻗어 삼각형(꼭짓점-왼쪽점-오른쪽점)으로 가드 유효범위를 그린다
void ULeeDefenseComponent::DrawGuardArcDebug() const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	// 삼각형 꼭짓점 = Player 위치, 기준 방향 = Player 전방(2D)
	const FVector Apex = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	// GuardValidAngleDeg 자체가 half-angle(좌우 각각 허용 각도)이므로 그대로 사용 (ExecCalc의 판정식과 동일 해석)
	const float HalfAngle = GuardValidAngleDeg;
	const FColor TriangleColor = FColor::Green;

	// 왼쪽/오른쪽 변의 끝점
	const FVector LeftPoint = Apex + Forward.RotateAngleAxis(-HalfAngle, FVector::UpVector) * GuardArcDebugRadius;
	const FVector RightPoint = Apex + Forward.RotateAngleAxis(HalfAngle, FVector::UpVector) * GuardArcDebugRadius;

	// 삼각형 3변: 꼭짓점→왼쪽점, 꼭짓점→오른쪽점, 왼쪽점→오른쪽점(밑변)
	DrawDebugLine(World, Apex, LeftPoint, TriangleColor, false, -1.0f, 0, 2.0f);
	DrawDebugLine(World, Apex, RightPoint, TriangleColor, false, -1.0f, 0, 2.0f);
	DrawDebugLine(World, LeftPoint, RightPoint, TriangleColor, false, -1.0f, 0, 2.0f);

	// 전방(파란색) 기준선
	DrawDebugLine(World, Apex, Apex + Forward * GuardArcDebugRadius, FColor::Blue, false, -1.0f, 0, 1.5f);
}

// 오너 ASC의 LeeSoulsStatSet을 찾아 판정/고갈 델리게이트에 바인딩
void ULeeDefenseComponent::BindToStatSetDelegates()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	const ULeeSoulsStatSet* SoulsSet = ASC ? ASC->GetSet<ULeeSoulsStatSet>() : nullptr;
	if (!SoulsSet)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeDefenseComponent] %s에서 LeeSoulsStatSet을 찾지 못했습니다. 방어 판정 후처리가 동작하지 않습니다."),
			*GetNameSafe(GetOwner()));
		return;
	}

	SoulsSet->OnDamageResolved.AddUObject(this, &ThisClass::HandleDamageResolved);
	SoulsSet->OnOutOfStamina.AddUObject(this, &ThisClass::HandleOutOfStamina);
}

// [서버] 데미지 판정 확정 — ExecCalc가 기록한 결과 태그별로 이벤트 발송 + 반대편 GE 적용
void ULeeDefenseComponent::HandleDamageResolved(AActor* EffectInstigator, AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* EffectSpec, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/)
{
	// 서버 아니라면 리턴
	if (GetOwnerRole() != ROLE_Authority || !EffectSpec)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!Owner || !OwnerASC)
		return;

	const FGameplayTagContainer& ResultTags = EffectSpec->GetDynamicAssetTags();

	// 공격자 액터 — 히트리액션 방향/모션워핑 계산에는 실제 월드 위치가 있는 폰(아바타)이 필요하다.
	// Player는 ASC가 PlayerState에 있어 EffectInstigator가 PlayerState(월드 위치 없음)이므로,
	// 인스티게이터 ASC의 아바타(폰)를 최우선으로 사용한다. (Enemy는 Pawn ASC라 그대로 폰)
	AActor* Attacker = EffectInstigator;
	const FGameplayEffectContextHandle& Ctx = EffectSpec->GetContext();
	if (UAbilitySystemComponent* InstigatorASC = Ctx.GetInstigatorAbilitySystemComponent())
	{
		if (AActor* Avatar = InstigatorASC->GetAvatarActor())
		{
			Attacker = Avatar;
		}
	}
	if (Attacker == EffectInstigator)
	{
		if (AActor* Causer = Ctx.GetEffectCauser())
		{
			Attacker = Causer;
		}
	}

	// ── 퍼펙트 회피: 방어자(Dodge 어빌리티)에게 알림 → 카운터윈도우 + 잔상 Cue ──
	if (ResultTags.HasTagExact(MyTags::Souls::DamageResult_PerfectDodge))
	{
		// UE_LOG(LogLee, Warning, TEXT("[임시디버그][PerfectDodge] 판정 성공 → Owner=%s 에 Event 발송"), *GetNameSafe(Owner));

		SendGameplayEventTo(Owner, MyTags::Souls::Event_Defense_PerfectDodge, Attacker);
		return;
	}

	// ── 퍼펙트 가드 (패리 연쇄): 방어자 패리 몽타주 + 공격자 스태미나 감소/패리당함 리액션 ──
	if (ResultTags.HasTagExact(MyTags::Souls::DamageResult_PerfectGuard))
	{
		SendGameplayEventTo(Owner, MyTags::Souls::Event_Defense_PerfectGuard, Attacker);

		UAbilitySystemComponent* AttackerASC =
			Attacker ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Attacker) : nullptr;
		if (AttackerASC)
		{
			// 스태미나 감소 GE 적용은 공격자의 OnOutOfStamina를 "동기적으로" 발동시킨다.
			// 먼저 연출 시작을 선언해두지 않으면 그로기 몽타주가 아래 Parried 이벤트보다 앞서 재생됐다가
			// 곧바로 패리당함 몽타주에 덮여 사라진다. 그래서 GE 적용 전에 보류 플래그를 세운다.
			ULeeDefenseComponent* AttackerDefense =
				Attacker ? Attacker->FindComponentByClass<ULeeDefenseComponent>() : nullptr;
			if (AttackerDefense)
			{
				AttackerDefense->BeginReaction();
			}

			// 공격자 스태미나 감소 GE — 원인 태그(ParryCounter)를 실어 고갈 시 PostureBreak로 분기되게 한다
			if (StaminaDamageEffect)
			{
				FGameplayEffectSpecHandle SpecHandle =
					AttackerASC->MakeOutgoingSpec(StaminaDamageEffect, /*Level*/1.0f, AttackerASC->MakeEffectContext());
				if (SpecHandle.IsValid())
				{
					// 노출 변수는 양수 비용, 적용 시점에만 음수 변환
					SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_StaminaDamage, -StaminaDamageOnParry);
					SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::DamageType_ParryCounter);
					AttackerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}

			// 공격자의 공격(Exclusive_Blocking)을 먼저 취소해야 HitReaction(Exclusive_Replaceable)이 활성화된다
			CancelExclusiveAbilities(AttackerASC);
			SendGameplayEventTo(Attacker, MyTags::Souls::Event_Combat_Parried, Owner);

			// 스태미나가 고갈되지 않았다면 보류할 그로기도 없으므로 연출 플래그를 즉시 되돌린다
			// (패리당함 몽타주만 재생하고 끝나는 정상 케이스)
			if (AttackerDefense && !AttackerDefense->bGroggyPending)
			{
				AttackerDefense->bReactionInProgress = false;
			}
		}
		return;
	}

	// ── 일반 가드 피격: 플린치 + 잠시 회복 차단 (다크소울3식) ──
	if (ResultTags.HasTagExact(MyTags::Souls::DamageResult_GuardHit))
	{
		if (RegenBlockOnHitEffect)
		{
			OwnerASC->ApplyGameplayEffectToSelf(
				RegenBlockOnHitEffect->GetDefaultObject<UGameplayEffect>(), /*Level*/1.0f, OwnerASC->MakeEffectContext());
		}

		// 이 시점에는 스태미나 감소가 이미 반영돼 있다(ExecCalc가 스태미나→Health 순으로 출력).
		// 스태미나가 0 이하이면 가드 브레이크로 처리한다. 이렇게 하면 "이미 0인 채 가드 피격"처럼
		// OnOutOfStamina가 재발동하지 않는(엣지 트리거 누락) 상황에서도 반드시 가드가 풀린다.
		const float CurrentStamina = OwnerASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
		if (CurrentStamina <= 0.0f)
		{
			BreakGuard(Attacker);
			return;
		}

		// 같은 피격으로 이미 가드 브레이크됐다면 플린치는 생략 (브레이크 몽타주가 우선).
		// 그로기 GE는 브레이크 몽타주가 끝난 뒤에 붙으므로, 태그 대신 연출 진행 플래그로 판정한다.
		if (!bReactionInProgress && !OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
		{
			SendGameplayEventTo(Owner, MyTags::Souls::Event_Defense_GuardHit, Attacker);
		}
		return;
	}

	// ── 일반 피격: 현재 행동을 끊고 경직 리액션 (강공격이면 루트모션 넉백 리액션으로 분기) ──
	if (ResultTags.HasTagExact(MyTags::Souls::DamageResult_HitReact))
	{
		// UE_LOG(LogLee, Warning, TEXT("[임시디버그][HitReact] Attacker(EffectInstigator)=%s Owner=%s"),
		// 	*GetNameSafe(Attacker), *GetNameSafe(Owner));

		CancelExclusiveAbilities(OwnerASC);

		// 가드불가 공격을 가드 중에 맞으면 → 상태 변화 없이 연출만 가드브레이크 몽타주로 대체
		// (그로기 GE·가드 어빌리티 강제종료는 하지 않음. 실제 가드브레이크는 스태미나 고갈 시 HandleOutOfStamina가 그대로 담당)
		if (ResultTags.HasTagExact(MyTags::Souls::DamageType_Attack_Unblockable)
			&& OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active))
		{
			SendGameplayEventTo(Owner, MyTags::Souls::Event_Combat_GuardBreak, Attacker);
			return;
		}

		const FGameplayTag& HitReactEventTag = ResultTags.HasTagExact(MyTags::Souls::DamageType_Attack_Heavy)
			? MyTags::Souls::Event_Combat_HitReactHeavy
			: MyTags::Souls::Event_Combat_HitReact;
		SendGameplayEventTo(Owner, HitReactEventTag, Attacker);
	}
}

// [서버] 스태미나 0 도달 — 원인 태그로 GuardBreak/PostureBreak 분기 
void ULeeDefenseComponent::HandleOutOfStamina(AActor* EffectInstigator, AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* EffectSpec, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/)
{
	if (GetOwnerRole() != ROLE_Authority || !EffectSpec)
	{
		return;
	}

	AActor* Owner = GetOwner();
	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!Owner || !OwnerASC)
	{
		return;
	}

	const FGameplayTagContainer& CauseTags = EffectSpec->GetDynamicAssetTags();

	// ── 가드 중 피격으로 고갈 → 가드 브레이크 ──
	if (CauseTags.HasTagExact(MyTags::Souls::DamageResult_GuardHit))
	{
		// "0으로 떨어지는 순간" 경로 — HandleDamageResolved(이미 0 경로)와 동일한 헬퍼로 수렴
		BreakGuard(EffectInstigator);
		return;
	}

	// ── 그 외 모든 고갈 경로(패리 반격, 일반 피격 누적, 자체 소모 등) → 체간 붕괴(그로기) ──
	// 예전에는 ParryCounter만 처리하고 나머지는 그대로 return해서, 일반 전투로 스태미나가 마르면
	// Status.Groggy는 붙는데 PostureBreak 이벤트가 발송되지 않아 그로기 몽타주가 재생되지 않았다.
	EnterGroggy(EffectInstigator);
}

// [서버] 그로기 진입 단일 입구 — Exclusive 어빌리티 취소 + GE_Groggy 적용 + PostureBreak 이벤트 발송
void ULeeDefenseComponent::EnterGroggy(AActor* Instigator)
{
	AActor* Owner = GetOwner();
	UAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (GetOwnerRole() != ROLE_Authority || !Owner || !OwnerASC)
	{
		return;
	}

	// 이미 그로기면 재진입하지 않는다 (몽타주 재시작 방지)
	if (OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
	{
		return;
	}

	// 가드브레이크/패리당함 연출 중이면 지금 진입하지 않고 예약만 한다.
	// 지금 PostureBreak를 발송하면 그로기 몽타주가 먼저 재생됐다가 선행 연출 몽타주에 덮여 사라진다.
	if (bReactionInProgress)
	{
		bGroggyPending = true;
		PendingGroggyInstigator = Instigator;

		// 연출 어빌리티가 어떤 이유로든(몽타주 미설정 등) 종료 콜백을 주지 못해도 그로기가 누락되지 않도록
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				PendingGroggyTimeoutHandle, this, &ThisClass::ForceEnterPendingGroggy, PendingGroggyTimeout, /*bLoop*/false);
		}
		return;
	}

	CancelExclusiveAbilities(OwnerASC);
	ApplyGroggy();

	// GE 적용 이후에 발송해야 한다 — GA_HitReaction이 Status.Groggy 제거를 기다리는데,
	// 태그가 없는 상태로 태스크가 시작되면 즉시 종료 콜백이 날아온다.
	SendGameplayEventTo(Owner, MyTags::Souls::Event_Combat_PostureBreak, Instigator);
}

// [서버] 선행 리액션 연출 시작 — 이 구간의 그로기 진입은 연출이 끝날 때까지 보류된다
void ULeeDefenseComponent::BeginReaction()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	bReactionInProgress = true;
}

// [서버] 선행 리액션 몽타주 종료 — 보류된 그로기가 있으면 지금 진입시킨다
void ULeeDefenseComponent::NotifyReactionFinished(AActor* Instigator)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingGroggyTimeoutHandle);
	}

	bReactionInProgress = false;

	if (!bGroggyPending)
	{
		return;
	}
	bGroggyPending = false;

	// 예약 시점의 Instigator를 우선 사용한다 (몽타주 종료 콜백에는 공격자 정보가 없다)
	AActor* GroggyInstigator = PendingGroggyInstigator.IsValid() ? PendingGroggyInstigator.Get() : Instigator;
	PendingGroggyInstigator = nullptr;

	EnterGroggy(GroggyInstigator);
}

// [서버] 안전장치 — 연출 어빌리티가 종료 콜백을 주지 못한 경우 보류된 그로기를 강제 진입시킨다
void ULeeDefenseComponent::ForceEnterPendingGroggy()
{
	UE_LOG(LogLee, Warning,
		TEXT("[LeeDefenseComponent] %s: 리액션 몽타주 종료 통지가 %.1f초 안에 오지 않아 그로기를 강제 진입시킵니다. "
			 "GA_HitReaction의 가드브레이크/패리당함 몽타주 설정을 확인하세요."),
		*GetNameSafe(GetOwner()), PendingGroggyTimeout);

	NotifyReactionFinished(nullptr);
}

// 그로기 GE 적용 — 이미 그로기 상태면 중복 적용하지 않는다
void ULeeDefenseComponent::ApplyGroggy()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
	{
		return;
	}

	if (!GroggyEffect)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeDefenseComponent] GroggyEffect가 설정되지 않음. BP에서 GE_Groggy를 지정해주세요."));
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GroggyEffect, /*Level*/1.0f, ASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

// 가드 브레이크 — 가드 어빌리티 강제 종료 + GuardBreak 이벤트. 그로기는 몽타주가 끝난 뒤 EnterGroggy()가 건다.
void ULeeDefenseComponent::BreakGuard(AActor* Attacker)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 이미 리액션 연출 중이거나 그로기면, 두 경로(OnOutOfStamina/OnDamageResolved)가 겹쳐도 한 번만 처리
	if (bReactionInProgress || ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
	{
		return;
	}
	BeginReaction();

	// 가드 어빌리티 강제 종료 (AbilityTags = Souls.Abilities.Guard 매칭)
	const FGameplayTagContainer GuardAbilityTags(MyTags::Souls::Ability_Guard);
	ASC->CancelAbilities(&GuardAbilityTags);

	SendGameplayEventTo(GetOwner(), MyTags::Souls::Event_Combat_GuardBreak, Attacker);
}

// Exclusive 그룹(공격 등) 어빌리티만 취소 — Independent(락온 등)는 유지한다
void ULeeDefenseComponent::CancelExclusiveAbilities(UAbilitySystemComponent* ASC)
{
	if (ULeeAbilitySystemComponent* LeeASC = Cast<ULeeAbilitySystemComponent>(ASC))
	{
		LeeASC->CancelActivationGroupAbilities(ELeeAbilityActivationGroup::Exclusive_Blocking, nullptr, /*bReplicateCancel*/true);
		LeeASC->CancelActivationGroupAbilities(ELeeAbilityActivationGroup::Exclusive_Replaceable, nullptr, /*bReplicateCancel*/true);
	}
}

// GameplayEvent 발송 헬퍼 — Instigator에 상대편 액터를 실어 리액션 방향(모션워핑) 계산에 쓴다
void ULeeDefenseComponent::SendGameplayEventTo(AActor* TargetActor, const FGameplayTag& EventTag, AActor* InstigatorActor)
{
	if (!TargetActor || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = InstigatorActor;
	Payload.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, Payload);
}

UAbilitySystemComponent* ULeeDefenseComponent::GetOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
