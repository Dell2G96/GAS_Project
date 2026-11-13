// 26.11.13 신규 - 몽타주 이동 잠금 구간 AnimNotifyState

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LeeAnimNotifyState_MoveLock.generated.h"

/**
 * 몽타주 구간 동안 ASC에 Souls.Status.MoveLock 태그를 붙여 이동·몸 회전을 막는 노티파이 스테이트.
 * 이동은 클라이언트 예측이 있으므로 서버·클라이언트 모두에서 각자 붙이고 뗀다. 플레이어·적 공용.
 */
UCLASS(DisplayName = "Lee Move Lock")
class GAS_PROJECT_API ULeeAnimNotifyState_MoveLock : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	ULeeAnimNotifyState_MoveLock();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
