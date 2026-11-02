// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
// #include "GameFramework/GameplayMessageSubsystem.h"
#include "LeeReticleWidgetBase.generated.h"

// struct FLeeTargetLockMessage;

/**
 * 무기 조준선 위젯 베이스.
 * 26.11.03 추가 - 타겟 락온 중에는 조준선을 숨기고, 락온이 풀리면 다시 표시한다.
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeReticleWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	ULeeReticleWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void InitializeFromWeapon(class ULeeWeaponInstance* InWeapon);

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponInitialized();

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULeeWeaponInstance> WeaponInstance;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class ULeeInventoryItemInstance> InventoryInstance;

	// 26.11.03 추가 - 빌드 검증 완료, 학습용으로 주석 처리
	// virtual void NativeConstruct() override;
	// virtual void NativeDestruct() override;
	//
// private:
	// /** 락온 메시지 수신 → 내 Pawn의 락온일 때만 표시/숨김 갱신 */
	// void HandleTargetLockMessage(FGameplayTag Channel, const FLeeTargetLockMessage& Message);
	//
	// /** 락온 여부에 따라 조준선 Visibility 적용 */
	// void ApplyTargetLockVisibility(bool bLocked);
	//
	// FGameplayMessageListenerHandle TargetLockListenerHandle;
	//
	// /** 생성 시점의 원래 Visibility (락온 해제 시 이 값으로 복귀) */
	// ESlateVisibility DefaultVisibility = ESlateVisibility::HitTestInvisible;
};
