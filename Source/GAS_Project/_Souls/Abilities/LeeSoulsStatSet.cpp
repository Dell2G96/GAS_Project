// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSoulsStatSet.h"
#include "GAS_Project/AAI/Boss/LeeBossBrainComponent.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/Executions/LeeExecCalc_Damage.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "Net/UnrealNetwork.h"

ULeeSoulsStatSet::ULeeSoulsStatSet()
	:Health(100.f)
	, MaxHealth(100.f)
	, Stamina(100.f)
	, MaxStamina(100.f)
	, EstusCharges(3.f)
	, MaxEstusCharges(3.f)
	, EstusHealAmount(40.f)
{
	bOutOfHealth = false;
	bOutOfStamina = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
	MaxStaminaBeforeAttributeChange = 0.0f;
	StaminaBeforeAttributeChange = 0.0f;
	EstusChargesBeforeAttributeChange = 0.0f;
}

// 26.11.02 추가 - 클라이언트로 복제할 어트리뷰트 등록 (값이 같아도 항상 OnRep 호출 → 예측 값 보정)
void ULeeSoulsStatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, EstusCharges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, MaxEstusCharges, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULeeSoulsStatSet, EstusHealAmount, COND_None, REPNOTIFY_Always);
}

// 클라이언트: Health 복제 수신 → HUD 갱신 이벤트 (사망 판정은 서버의 PostGameplayEffectExecute가 담당)
void ULeeSoulsStatSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, Health, OldValue);

	const float OldHealth = OldValue.GetCurrentValue();
	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetHealth() - OldHealth, OldHealth, GetHealth());
}

// 클라이언트: MaxHealth 복제 수신 → HUD 갱신 이벤트
void ULeeSoulsStatSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, MaxHealth, OldValue);

	const float OldMaxHealth = OldValue.GetCurrentValue();
	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldMaxHealth, OldMaxHealth, GetMaxHealth());
}

// 클라이언트: Stamina 복제 수신 → HUD 갱신 이벤트 (그로기 판정은 서버 담당)
void ULeeSoulsStatSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, Stamina, OldValue);

	const float OldStamina = OldValue.GetCurrentValue();
	OnStaminaChanged.Broadcast(nullptr, nullptr, nullptr, GetStamina() - OldStamina, OldStamina, GetStamina());
}

// 클라이언트: MaxStamina 복제 수신 → HUD 갱신 이벤트
void ULeeSoulsStatSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, MaxStamina, OldValue);

	const float OldMaxStamina = OldValue.GetCurrentValue();
	OnMaxStaminaChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxStamina() - OldMaxStamina, OldMaxStamina, GetMaxStamina());
}

// 클라이언트: 에스트 횟수 복제 수신 → 포션 개수 HUD 갱신 이벤트
void ULeeSoulsStatSet::OnRep_EstusCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, EstusCharges, OldValue);

	const float OldCharges = OldValue.GetCurrentValue();
	OnEstusChargesChanged.Broadcast(nullptr, nullptr, nullptr, GetEstusCharges() - OldCharges, OldCharges, GetEstusCharges());
}

// 클라이언트: 에스트 최대 횟수 복제 수신
void ULeeSoulsStatSet::OnRep_MaxEstusCharges(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, MaxEstusCharges, OldValue);
}

// 클라이언트: 에스트 회복량 복제 수신
void ULeeSoulsStatSet::OnRep_EstusHealAmount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULeeSoulsStatSet, EstusHealAmount, OldValue);
}

bool ULeeSoulsStatSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// 무적(Status.Invincible) 상태에서는 Health 감소를 이 단일 지점에서 차단한다.
	// 처형/암살 시퀀스 중 GE_FinisherInvincible이 부여하는 태그 — 모든 데미지 경로가 여기를 거친다.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && Data.EvaluatedData.Magnitude < 0.0f)
	{
		// 무적 태그가 있으면 넘어감 — 단, 가드불가 공격의 일반 회피 무적 관통은 허용 (퍼펙트 회피로만 회피 가능)
		if (Data.Target.HasMatchingGameplayTag(MyTags::Souls::Status_Invincible)
			&& !ULeeExecCalc_Damage::ShouldPierceDodgeInvincibility(Data.EffectSpec.GetDynamicAssetTags(), Data.Target))
		{
			return false;
		}
	}

	// 현재 체력을 저장
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	StaminaBeforeAttributeChange = GetStamina();
	MaxStaminaBeforeAttributeChange = GetMaxStamina();

	EstusChargesBeforeAttributeChange = GetEstusCharges();
	return true;}

void ULeeSoulsStatSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float CurrentHealth = GetHealth();

		// [방어 시스템 —  Clamp 상한을 하드코딩(100) 대신 MaxHealth 기준으로 수정 (버그 수정)
		float ClampedHealth = FMath::Clamp(CurrentHealth, 0.0f, GetMaxHealth());

		if (CurrentHealth != ClampedHealth)
		{
			SetHealth(ClampedHealth);
		}
	}
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		float CurrentStamina = GetStamina();

		// [방어 시스템 —] 
		float ClampedStamina = FMath::Clamp(CurrentStamina, 0.0f, GetMaxStamina());
		
		if (CurrentStamina != ClampedStamina)
		{
			SetStamina(ClampedStamina);
		}
	}
	// 26.10.30 추가 - 에스트 횟수를 0 ~ MaxEstusCharges로 제한 (휴식 GE가 Max보다 크게 넣어도 안전)
	if (Data.EvaluatedData.Attribute == GetEstusChargesAttribute())
	{
		SetEstusCharges(FMath::Clamp(GetEstusCharges(), 0.0f, GetMaxEstusCharges()));

		if (GetEstusCharges() != EstusChargesBeforeAttributeChange)
		{
			OnEstusChargesChanged.Broadcast(Data.EffectSpec.GetEffectContext().GetOriginalInstigator(), Data.EffectSpec.GetEffectContext().GetEffectCauser(),
				&Data.EffectSpec, Data.EvaluatedData.Magnitude, EstusChargesBeforeAttributeChange, GetEstusCharges());
		}
	}
	const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MyTags::Souls::Gameplay_DamageSelfDestruct);
    float MinimumHealth = 0.0f;

// #if !UE_BUILD_SHIPPING
//     // Godmode and unlimited health stop death unless it's a self destruct
//     if (!bIsDamageFromSelfDestruct &&
//         (Data.Target.HasMatchingGameplayTag(LeeGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(LeeGameplayTags::Cheat_UnlimitedHealth)))
//     {
//         MinimumHealth = 1.0f;
//     }
// #endif // #if !UE_BUILD_SHIPPING

    const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
    AActor* Instigator = EffectContext.GetOriginalInstigator();
    AActor* Causer = EffectContext.GetEffectCauser();

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        if (Data.EvaluatedData.Magnitude > 0.0f)
        {
            FLeeVerbMessage Message;
            Message.Verb =  MyTags::Souls::Gameplay_Damage_Message;
            Message.Instigator = Data.EffectSpec.GetEffectContext().GetEffectCauser();
            Message.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
            Message.Target = GetOwningActor();
            Message.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
        	//@TODO: 컨텍스트 태그와 어빌리티 시스템 소속이 아닌 소스(출처)/인스티게이터(유발자) 태그 채워 넣기
        	//@TODO: 상대팀 처치인지, 자해(자살)인지, 팀킬(아군 처치)인지 등 판별하기...
            Message.Magnitude = Data.EvaluatedData.Magnitude;

            UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
            MessageSystem.BroadcastMessage(Message.Verb, Message);
        }
        if (GetHealth() != HealthBeforeAttributeChange)
        {
            OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
        }

        if ((GetHealth() <= 0.0f) && !bOutOfHealth)
        {
            OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
        }

        bOutOfHealth = (GetHealth() <= 0.0f);

        // [방어 시스템 - 26.07.16]
        // ExecCalc가 Spec에 기록한 판정 결과(Souls.DamageResult.*)가 있으면 브로드캐스트.
        // ExecCalc는 Health Modifier를 항상 마지막에 출력하므로(스태미나 감소가 먼저 실행됨),
        // 이 시점에는 이번 데미지 GE의 모든 어트리뷰트 변경이 확정된 상태다
        static const FGameplayTagContainer DamageResultTags = FGameplayTagContainer::CreateFromArray(
            TArray<FGameplayTag>{
                MyTags::Souls::DamageResult_HitReact,
                MyTags::Souls::DamageResult_GuardHit,
                MyTags::Souls::DamageResult_PerfectGuard,
                MyTags::Souls::DamageResult_PerfectDodge,
                MyTags::Souls::DamageResult_ShieldHit }); // 26.11.08 추가 - 마법 쉴드 피격도 DefenseComponent로 전달
        
        if (Data.EffectSpec.GetDynamicAssetTags().HasAny(DamageResultTags))
        {
            OnDamageResolved.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
        }
    }
    if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
        if (GetStamina() != StaminaBeforeAttributeChange)
        {
            OnStaminaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
        }

        if (GetStamina() > 0.0f)
        {
            bOutOfStamina = false;
        }
        else if (!bOutOfStamina)
        {
            ULeeBossBrainComponent* BossBrain = ULeeBossBrainComponent::FindForPawn(Data.Target.GetAvatarActor());
            const float StaminaDelta = Data.EvaluatedData.Magnitude;
            const bool bIsAttackCost = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MyTags::Souls::StaminaCause_AttackCost);

            if (BossBrain && bIsAttackCost && StaminaDelta < 0.0f)
            {
                // 비용 고갈로는 그로기 이벤트를 소비하지 않는다. 0인 동안 받은 외부 피해도 처리할 수 있다.
                BossBrain->BeginAttackStaminaRecovery(-StaminaDelta);
            }
            else if (!BossBrain || StaminaDelta < 0.0f || !BossBrain->IsRecoveringAttackStamina())
            {
                bOutOfStamina = true;
                OnOutOfStamina.Broadcast(Instigator, Causer, &Data.EffectSpec, StaminaDelta, StaminaBeforeAttributeChange, GetStamina());
            }
        }
    }
}
