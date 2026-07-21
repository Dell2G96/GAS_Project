#include "LeeExecCalc_Damage.h"

#include "AbilitySystemComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeDefenseComponent.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

// 방어자에게 ULeeDefenseComponent가 없을 때 사용하는 안전 기본값 (정상 구성이라면 사용되지 않음)
namespace LeeExecCalcDefaults
{
	constexpr float GuardStaminaCost = 20.0f;
	// half-angle 기본값 — 정면 기준 좌우 45도(총 90도)까지만 가드 유효
	constexpr float GuardValidAngleDeg = 45.0f;
}

ULeeExecCalc_Damage::ULeeExecCalc_Damage()
{

}

// 공격자가 방어자의 전방 가드 유효범위(정면 기준 ±GuardValidAngleDeg) 안에 있는지 판정
bool ULeeExecCalc_Damage::IsAttackInsideGuardArc(const AActor* Defender, const AActor* Attacker, float GuardValidAngleDeg)
{
	if (!Defender || !Attacker)
		return true;

	// 방어자 전방 벡터
	const FVector DefenderForward = Defender->GetActorForwardVector().GetSafeNormal2D();
	// 방어자가 공격자를 바라보는 방향 벡터
	const FVector ToAttacker = (Attacker->GetActorLocation() - Defender->GetActorLocation()).GetSafeNormal2D();
	// GuardValidAngleDeg는 half-angle(좌우 각각 허용 각도)이므로 그대로 임계값 계산에 사용 (곱하기 0.5 안 함)
	const float FrontDotThreshold = FMath::Cos(FMath::DegreesToRadians(GuardValidAngleDeg));
	const float ArcDot = FVector::DotProduct(DefenderForward, ToAttacker);

	// [임시 디버그] 가드 유효범위 판정 — Dot이 Threshold 이상이어야 가드 성립.
	//  랜덤 실패면 Defender(방어자)의 GetActorForwardVector가 적을 향하지 않는 것 = 가드 중 캐릭터 회전 문제일 확률 높음
	UE_LOG(LogTemp, Warning, TEXT("[임시디버그][GuardArc] Dot=%.2f Threshold=%.2f HalfAngle=%.0f → %s"),
		ArcDot, FrontDotThreshold, GuardValidAngleDeg, (ArcDot >= FrontDotThreshold) ? TEXT("유효범위안(가드성립)") : TEXT("유효범위밖(일반피격)"));

	return ArcDot >= FrontDotThreshold;
}

// 데미지 판정 본체 — 방어자 태그 우선순위 검사 후 Modifier 출력 + 판정 결과 태그 기록 (서버 전용 실행)
void ULeeExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 판정 결과 태그를 기록할 수 있는 mutable Spec (PostGameplayEffectExecute에서 읽는다)
	FGameplayEffectSpec* MutableSpec = ExecutionParams.GetOwningSpecForPreExecuteMod();

	// 기존 컨벤션 유지: SetByCaller(Souls.SetByCaller.Damage)는 음수 = Health 감소량
	const float IncomingHealthDelta = Spec.GetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, /*WarnIfNotFound*/false, 0.0f);

	AActor* DefenderActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	// 공격자 액터 — 전방 원뿔 판정용. 방향 계산에는 실제 월드 위치가 있는 폰(아바타)이 필요하다.
	// Player는 ASC가 PlayerState에 있어 GetOriginalInstigator()가 PlayerState(월드 위치 없음)를 반환하므로,
	// 인스티게이터 ASC의 아바타(폰)를 최우선으로 사용한다. (Enemy는 Pawn ASC라 그대로 폰이 나옴)
	const FGameplayEffectContextHandle& Context = Spec.GetEffectContext();
	AActor* AttackerActor = nullptr;
	if (UAbilitySystemComponent* InstigatorASC = Context.GetInstigatorAbilitySystemComponent())
	{
		AttackerActor = InstigatorASC->GetAvatarActor();
	}
	if (!AttackerActor)
	{
		AttackerActor = Context.GetEffectCauser();
	}
	if (!AttackerActor)
	{
		AttackerActor = Context.GetOriginalInstigator();
	}

	// 가드 수치는 방어자의 DefenseComponent에서 읽는다 (없으면 안전 기본값)
	const ULeeDefenseComponent* DefenseComp =
		DefenderActor ? DefenderActor->FindComponentByClass<ULeeDefenseComponent>() : nullptr;
	const float GuardStaminaCost = DefenseComp ? DefenseComp->GetGuardStaminaCost() : LeeExecCalcDefaults::GuardStaminaCost;
	const float GuardValidAngleDeg = DefenseComp ? DefenseComp->GetGuardValidAngleDeg() : LeeExecCalcDefaults::GuardValidAngleDeg;

	if (!TargetASC)
	{
		return;
	}

	// ── 우선순위 1: 무적 ─────────────────────────────────────────────
	if (TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Invincible))
	{
		// 퍼펙트 회피 윈도우 안이면 결과 기록 (연출/카운터윈도우는 DefenseComponent → Dodge 어빌리티)
		if (TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Dodge_Perfect) && MutableSpec)
		{
			MutableSpec->AddDynamicAssetTag(MyTags::Souls::DamageResult_PerfectDodge);

			// PostGameplayEffectExecute(Health 분기)가 호출되도록 0 크기 Health Modifier 출력
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
				ULeeSoulsStatSet::GetHealthAttribute(), EGameplayModOp::Additive, 0.0f));
		}
		// 단순 무적: 데미지 0, 이벤트 없음 — Modifier를 출력하지 않고 종료
		return;
	}

	const bool bInsideGuardArc = IsAttackInsideGuardArc(DefenderActor, AttackerActor, GuardValidAngleDeg);

	// [임시 디버그] 공격자/가드태그/원뿔 상태 한 줄 요약
	//  - Attacker=None 이면 GetOriginalInstigator/GetEffectCauser 둘 다 null (히트리액션 방향 #1과 연관)
	//  - GuardActive=1 인데 InsideArc=0 이면 → 방어자가 적을 안 보고 있어서 원뿔 밖 판정 = 가드 랜덤 실패 원인
	UE_LOG(LogTemp, Warning, TEXT("[임시디버그][Damage] Attacker=%s GuardActive=%d InsideArc=%d"),
		*GetNameSafe(AttackerActor),
		TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active) ? 1 : 0,
		bInsideGuardArc ? 1 : 0);

	// ── 우선순위 2: 퍼펙트 가드 (전방 원뿔 안) ──────────────────────
	if (bInsideGuardArc && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Perfect))
	{
		if (MutableSpec)
		{
			MutableSpec->AddDynamicAssetTag(MyTags::Souls::DamageResult_PerfectGuard);
		}

		// 데미지 0 / 스태미나 소모 0 — 판정 결과 브로드캐스트용 0 크기 Health Modifier만 출력
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			ULeeSoulsStatSet::GetHealthAttribute(), EGameplayModOp::Additive, 0.0f));
		return;
	}

	// ── 우선순위 3: 일반 가드 (전방 원뿔 안) ────────────────────────
	if (bInsideGuardArc && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active))
	{
		if (MutableSpec)
		{
			MutableSpec->AddDynamicAssetTag(MyTags::Souls::DamageResult_GuardHit);
		}

		// 스태미나 Modifier를 먼저 출력한다 — 고갈(OnOutOfStamina → GuardBreak) 판정이
		// Health Modifier의 OnDamageResolved(플린치)보다 먼저 확정되도록 순서를 보장 
		
		// 스태미나 감소
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			ULeeSoulsStatSet::GetStaminaAttribute(), EGameplayModOp::Additive, -GuardStaminaCost));
		
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			ULeeSoulsStatSet::GetHealthAttribute(), EGameplayModOp::Additive, 0.0f));
		return;
	}

	// ── 우선순위 4: 일반 피격 (풀 데미지) ───────────────────────────
	if (MutableSpec)
	{
		MutableSpec->AddDynamicAssetTag(MyTags::Souls::DamageResult_HitReact);
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		ULeeSoulsStatSet::GetHealthAttribute(), EGameplayModOp::Additive, IncomingHealthDelta));
}
