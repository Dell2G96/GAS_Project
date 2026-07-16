#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "LeeAnimNotifyState_GameplayEvent.generated.h"

/**
 * 구간 시작/끝에서 GameplayEvent만 발사하는 범용 AnimNotifyState (서버 전용).
 * 기존 ULeeAnimNotify_GameplayEvent(단발)의 NotifyState 버전.
 *
 * 태그를 직접 부여/제거하지 않는다 — 상태 변경은 이벤트를 수신한 어빌리티가 GE로 처리 (안전장치 a).
 * i-frame 판정의 1차 근거는 Dodge 어빌리티의 서버 타이머이며
 * 이 NotifyState는 보조 신호/다른 윈도우(슈퍼아머 등) 재사용 용도
 */
UCLASS(DisplayName = "Lee Gameplay Event Window (Server)")
class GAS_PROJECT_API ULeeAnimNotifyState_GameplayEvent : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
	
private:
	/** 서버에서만 오너에게 이벤트 발사 (기존 단발 Notify와 동일 패턴) */
	static void SendEventToOwner(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag);

protected:
	/** 구간 시작 시 발사할 GameplayEvent 태그 (예: Souls.Events.Window.IFrame.Begin) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Event")
	FGameplayTag BeginEventTag;

	/** 구간 끝에서 발사할 GameplayEvent 태그 (예: Souls.Events.Window.IFrame.End) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Event")
	FGameplayTag EndEventTag;

};
