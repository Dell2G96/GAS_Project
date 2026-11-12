// 26.11.12 신규 - 재도전용 아레나 문

#pragma once

#include "CoreMinimal.h"
#include "LeeBossEntranceBase.h"
#include "LeeBossArenaDoor.generated.h"

class UBoxComponent;

// 재도전 때 쓰는 문. 처음엔 열려 있어 걸어서 들어가고, 아레나 안쪽 트리거에 들어오면 보스전이 바로 시작된다 (인트로 생략)
// 전투가 시작되면 봉쇄되어 밖으로 나갈 수 없고, 바깥 동료는 F로 들어올 수 있다
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeBossArenaDoor : public ALeeBossEntranceBase
{
	GENERATED_BODY()

public:
	ALeeBossArenaDoor();

protected:
	virtual void BeginPlay() override;
	virtual ELeeBossEntranceState GetActivatedState() const override { return ELeeBossEntranceState::Open; }

	// 아레나 안쪽 입장 감지 트리거 (문 안쪽에 둔다)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossEntrance")
	TObjectPtr<UBoxComponent> EntryTrigger;

private:
	// 열려 있는 동안 플레이어가 트리거에 들어오면 입장을 알린다 (서버)
	UFUNCTION()
	void HandleEntryTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
