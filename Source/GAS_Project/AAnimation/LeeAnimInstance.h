// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "LeeAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void InitializeWithAbilitySystem(class UAbilitySystemComponent* ASC);

protected:
	// ASC 준비 시점(또는 이미 준비됐으면 즉시)에 호출 → GameplayTag Property Map 초기화
	void OnAbilitySystemInitialized();

public:

	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	float GroundDistance = -1.f;

	/** 가드 스탠스 미러 — 판정은 ULeeTargetLockComponent가 하고, 여기선 레이어가 읽도록 복사만 한다.
	 *  (Thread-safe 레이어 함수에서 액터/컴포넌트 접근이 금지되므로 게임 스레드에서 미리 담아둔다) */
	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	bool GuardLeftFootBack = false;

	/** 26.11.26 추가 - 이동 컴포넌트가 지금 공중(MOVE_Falling)인지 미러. 띄워진 Enemy의 착지 판정(FallLoop → Land)에 쓴다.
	 *  이동 모드는 다른 클라이언트에도 복제되므로 게스트 화면에서도 맞게 나온다 */
	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	bool bIsFalling = false;

	UPROPERTY(EditDefaultsOnly, Category="GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;
		
	
};
