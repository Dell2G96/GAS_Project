// Fill out your copyright notice in the Description page of Project Settings.
// 26.09.23 신규 작성 — Enemy 전투 데이터 배선을 스크립트로 처리하기 위한 헬퍼

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// TSubclassOf<ULeeGameplayAbility>를 UFUNCTION 인자로 쓰려면 전방 선언만으로는 부족하다.
// 생성 코드가 StaticClass()를 호출하므로 완전한 정의가 필요하다.
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeEnemySetupLibrary.generated.h"

class ULeeAbilitySet;
class ULeeAttackTokenComponent;
class ULeeAttackerProfileComponent;

/** ULeeAttackerProfileComponent의 어느 공격 슬롯을 설정할지 지정한다 */
UENUM(BlueprintType)
enum class ELeeAttackProfileSlot : uint8
{
	Light,
	Heavy,
	Ranged
};

/**
 * 어택 토큰/공격 프로필의 게임플레이 태그 값을 이름으로 설정하는 헬퍼.
 *
 * 존재 이유: 파이썬은 FGameplayTag를 문자열로 만들 수 없다(엔진 제약).
 * 그래서 SlotConfigs(TMap<FGameplayTag,...>)나 Reservation.QuotaTag 같은
 * 태그가 들어가는 값은 파이썬에서 직접 채울 수 없어, 이름을 받아 변환해 주는
 * 진입점을 C++에 둔다. 에디터에서 손으로 채우던 설정을 스크립트로 대체하기 위함이다.
 */
UCLASS()
class GAS_PROJECT_API ULeeEnemySetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 어택 토큰 컴포넌트의 SlotConfigs에 Quota 태그 하나의 상한을 설정한다.
	 * 예: Souls.Attacker.Melee = 동시 2명, Souls.Attacker.Ranged = 동시 3명
	 * @param MaxTotalCost 0이면 Cost 총합 제한을 쓰지 않는다
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Setup", meta = (DevelopmentOnly))
	static bool SetAttackTokenSlotConfig(ULeeAttackTokenComponent* TokenComponent,
	                                     FName QuotaTagName, int32 MaxAttackers, int32 MaxTotalCost = 0);

	/**
	 * 공격 프로필 컴포넌트의 슬롯 하나(약/강/원거리)에 Quota 태그와 Cost를 설정한다.
	 * QuotaTagName은 SlotConfigs에 등록하는 상위 태그가 아니라 개별 공격 식별 태그를 넣는다.
	 * 예: Souls.Attacker.Melee.Light (상위 Souls.Attacker.Melee 슬롯을 소비한다)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Setup", meta = (DevelopmentOnly))
	static bool SetAttackerProfileReservation(ULeeAttackerProfileComponent* ProfileComponent,
	                                          ELeeAttackProfileSlot Slot, FName QuotaTagName, int32 Cost = 1);

	/**
	 * 어빌리티 셋에 어빌리티를 등록한다.
	 * FLeeAbilitySet_GameplayAbility의 필드가 EditDefaultsOnly라 파이썬에서 직접 못 채우기 때문에 둔 진입점이다.
	 * @param bReplaceAll true면 기존 목록을 비우고 이 어빌리티 하나만 남긴다
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AI|Setup", meta = (DevelopmentOnly))
	static bool GrantAbilityToSet(ULeeAbilitySet* AbilitySet, TSubclassOf<ULeeGameplayAbility> Ability,
	                              int32 AbilityLevel = 1, bool bReplaceAll = true);
};
