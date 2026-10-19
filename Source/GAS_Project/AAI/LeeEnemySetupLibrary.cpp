// Fill out your copyright notice in the Description page of Project Settings.
// 26.09.23 신규 작성 — Enemy 전투 데이터 배선 헬퍼 구현

#include "LeeEnemySetupLibrary.h"

#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"

namespace
{
	/** 이름을 이미 등록된 태그로 변환한다. 실패하면 이유를 로그로 남긴다 */
	bool ResolveRegisteredTag(FName TagName, FGameplayTag& OutTag)
	{
		if (TagName.IsNone())
		{
			UE_LOG(LogTemp, Error, TEXT("[LeeSetup] 태그 이름이 비어 있습니다."));
			return false;
		}

		OutTag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
		if (!OutTag.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[LeeSetup] 등록되지 않은 태그입니다: %s"), *TagName.ToString());
			return false;
		}
		return true;
	}
}

// 어택 토큰 컴포넌트의 SlotConfigs에 Quota 태그 하나의 상한을 기록한다.
bool ULeeEnemySetupLibrary::SetAttackTokenSlotConfig(ULeeAttackTokenComponent* TokenComponent,
                                                     FName QuotaTagName, int32 MaxAttackers, int32 MaxTotalCost)
{
	if (!TokenComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeSetup] TokenComponent가 null입니다."));
		return false;
	}

	FGameplayTag QuotaTag;
	if (!ResolveRegisteredTag(QuotaTagName, QuotaTag))
	{
		return false;
	}

	FLeeAttackSlotConfig Config;
	Config.MaxAttackers = MaxAttackers;
	Config.MaxTotalCost = MaxTotalCost;
	TokenComponent->SlotConfigs.Add(QuotaTag, Config);

	// 컴포넌트 템플릿을 고친 것이므로 패키지를 dirty로 표시해야 저장 대상이 된다.
	TokenComponent->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[LeeSetup] SlotConfig 설정: %s = 동시 %d명, Cost 상한 %d"),
		*QuotaTagName.ToString(), MaxAttackers, MaxTotalCost);
	return true;
}

// 공격 프로필의 약/강/원거리 슬롯에 Quota 태그와 Cost를 기록한다.
bool ULeeEnemySetupLibrary::SetAttackerProfileReservation(ULeeAttackerProfileComponent* ProfileComponent,
                                                          ELeeAttackProfileSlot Slot, FName QuotaTagName, int32 Cost)
{
	if (!ProfileComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeSetup] ProfileComponent가 null입니다."));
		return false;
	}

	FGameplayTag QuotaTag;
	if (!ResolveRegisteredTag(QuotaTagName, QuotaTag))
	{
		return false;
	}

	FLeeAttackReservationConfig* Target = nullptr;
	switch (Slot)
	{
	case ELeeAttackProfileSlot::Light:  Target = &ProfileComponent->LightAttack;  break;
	case ELeeAttackProfileSlot::Heavy:  Target = &ProfileComponent->HeavyAttack;  break;
	case ELeeAttackProfileSlot::Ranged: Target = &ProfileComponent->RangedAttack; break;
	default: break;
	}

	if (!Target)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeSetup] 알 수 없는 슬롯입니다."));
		return false;
	}

	Target->QuotaTag = QuotaTag;
	Target->Cost = FMath::Max(1, Cost);
	ProfileComponent->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[LeeSetup] 공격 프로필 설정: 슬롯 %d = %s (Cost %d)"),
		static_cast<int32>(Slot), *QuotaTagName.ToString(), Target->Cost);
	return true;
}

// 어빌리티 셋에 어빌리티를 등록한다. 구조체 필드가 EditDefaultsOnly라 파이썬이 직접 못 채우기 때문에 필요하다.
bool ULeeEnemySetupLibrary::GrantAbilityToSet(ULeeAbilitySet* AbilitySet, TSubclassOf<ULeeGameplayAbility> Ability,
                                              int32 AbilityLevel, bool bReplaceAll)
{
	if (!AbilitySet || !Ability)
	{
		UE_LOG(LogTemp, Error, TEXT("[LeeSetup] AbilitySet 또는 Ability가 null입니다."));
		return false;
	}

	if (bReplaceAll)
	{
		AbilitySet->GrantedGameplayAbilities.Empty();
	}

	FLeeAbilitySet_GameplayAbility Entry;
	Entry.Ability = Ability;
	Entry.AbilityLevel = FMath::Max(1, AbilityLevel);
	AbilitySet->GrantedGameplayAbilities.Add(Entry);
	AbilitySet->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[LeeSetup] 어빌리티 등록: %s (총 %d개)"),
		*Ability->GetName(), AbilitySet->GrantedGameplayAbilities.Num());
	return true;
}
