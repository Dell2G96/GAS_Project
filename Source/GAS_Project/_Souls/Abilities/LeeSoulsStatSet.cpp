// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSoulsStatSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"

ULeeSoulsStatSet::ULeeSoulsStatSet()
	:Health(100.f)
	, MaxHealth(100.f)
	, Stamina(100.f)
	, MaxStamina(100.f)
{
	bOutOfHealth = false;
	bOutOfStamina = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
	MaxStaminaBeforeAttributeChange = 0.0f;
	StaminaBeforeAttributeChange = 0.0f;
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
		// 무적 태그가 있으면 넘어감
		if (Data.Target.HasMatchingGameplayTag(MyTags::Souls::Status_Invincible))
		{
			return false;
		}
	}

	// 현재 체력을 저장
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	StaminaBeforeAttributeChange = GetStamina();
	MaxStaminaBeforeAttributeChange = GetMaxStamina();
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
                MyTags::Souls::DamageResult_PerfectDodge });
        
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

        // [신규] 스태미나 0 도달 → 그로기 진입 알림 (OnOutOfHealth와 동일 패턴)
        if ((GetStamina() <= 0.0f) && !bOutOfStamina)
        {
            OnOutOfStamina.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
        }

        bOutOfStamina = (GetStamina() <= 0.0f);
    }
}
