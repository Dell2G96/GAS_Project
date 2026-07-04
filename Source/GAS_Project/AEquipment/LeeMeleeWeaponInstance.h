// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AWeapons/LeeWeaponInstance.h"
#include "LeeMeleeWeaponInstance.generated.h"

class ULeeFinisherData;

/**
 * 근접 무기 장비 인스턴스.
 * ULeeWeaponInstance(1인칭/3인칭 애님 레이어 선택 - EquippedAnimSet/UnEquippedAnimSet)를 그대로 상속해
 * 기존 무기 인스턴스 체인(예: B_WeaponInstance_Katana_Souls)의 기능을 유지하면서,
 * 무기별 처형/암살 데이터(FinisherData)만 추가로 보유한다.
 * EquipmentDefinition의 InstanceType으로 지정하고, BP에서 FinisherData 에셋을 연결한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeMeleeWeaponInstance : public ULeeWeaponInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Lee|Finisher")
	ULeeFinisherData* GetFinisherData() const { return FinisherData; }

protected:
	/** 이 무기의 처형/암살 몽타주·데미지·정렬 데이터 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	TObjectPtr<ULeeFinisherData> FinisherData;
};
