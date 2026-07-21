 
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
		// [임시 디버그] ExecCalc가 퍼펙트닷지로 판정 → Dodge 어빌리티에 이벤트 발송 (#2: 이 로그가 뜨면 판정 성공)
		UE_LOG(LogLee, Warning, TEXT("[임시디버그][PerfectDodge] 판정 성공 → Owner=%s 에 Event 발송"), *GetNameSafe(Owner));

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

		// 같은 피격으로 이미 가드 브레이크(그로기)됐다면 플린치는 생략 (브레이크 몽타주가 우선)
		if (!OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
		{
			SendGameplayEventTo(Owner, MyTags::Souls::Event_Defense_GuardHit, Attacker);
		}
		return;
	}

	// ── 일반 피격: 현재 행동을 끊고 경직 리액션 ──
	if (ResultTags.HasTagExact(MyTags::Souls::DamageResult_HitReact))
	{
		// [임시 디버그] HitReact로 넘기는 Attacker(=EffectInstigator) 확인. None이면 HitReaction에서 방향 판정 불가(#1)
		UE_LOG(LogLee, Warning, TEXT("[임시디버그][HitReact] Attacker(EffectInstigator)=%s Owner=%s"),
			*GetNameSafe(Attacker), *GetNameSafe(Owner));

		CancelExclusiveAbilities(OwnerASC);
		SendGameplayEventTo(Owner, MyTags::Souls::Event_Combat_HitReact, Attacker);
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
		// 가드 어빌리티 강제 종료 (AbilityTags = Souls.Abilities.Guard 매칭)
		const FGameplayTagContainer GuardAbilityTags(MyTags::Souls::Ability_Guard);
		OwnerASC->CancelAbilities(&GuardAbilityTags);

		ApplyGroggy();
		SendGameplayEventTo(Owner, MyTags::Souls::Event_Combat_GuardBreak, EffectInstigator);
		return;
	}

	// ── 패리 반격으로 고갈 → 체간 붕괴 ──
	if (CauseTags.HasTagExact(MyTags::Souls::DamageType_ParryCounter))
	{
		CancelExclusiveAbilities(OwnerASC);
		ApplyGroggy();
		SendGameplayEventTo(Owner, MyTags::Souls::Event_Combat_PostureBreak, EffectInstigator);
		return;
	}
	
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
