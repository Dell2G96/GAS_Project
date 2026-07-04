// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "LeeAnimNotify_GameplayEvent.generated.h"

/**
 * 몽타주의 특정 타이밍에 오너 액터로 GameplayEvent를 발사하는 범용 AnimNotify.
 * 서버에서만 발사한다 (데미지 등 게임플레이 로직용 — 클라이언트 중복 실행 방지).
 *
 * 사용처: 피니셔 공격자 몽타주의 타격 타이밍에 배치,
 *         EventTag = Souls.Events.Finisher.Damage → GA_Finisher가 수신해 데미지 GE 적용.
 *
 * 여러 번 타격하는 몽타주(콤보형 처형 등)라면 이 노티파이를 히트 수만큼 배치하면 된다.
 * GA_Finisher는 이 이벤트를 반복 수신하도록 되어 있으며, 각 노티파이의 DamageMultiplier로
 * 히트별 데미지 비율을 조절한다 (예: 3연타를 0.34/0.33/0.33로 나누거나, 전부 1.0으로 두면
 * 매 히트가 FinisherData.Damage 전체를 적용).
 */
UCLASS(DisplayName = "Lee Gameplay Event (Server)")
class GAS_PROJECT_API ULeeAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 발사할 GameplayEvent 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Event")
	FGameplayTag EventTag;

	/**
	 * 이 히트의 데미지 비율. FinisherData.Damage에 곱해져 적용된다.
	 * 다단 히트 몽타주에서 노티파이마다 다르게 설정해 데미지를 분배할 수 있다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Event", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;
};
