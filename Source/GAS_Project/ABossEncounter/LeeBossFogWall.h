// 26.11.12 신규 - 첫 도전용 안개벽

#pragma once

#include "CoreMinimal.h"
#include "LeeBossEntranceBase.h"
#include "LeeBossFogWall.generated.h"

// 첫 도전 때 보스방 문 앞에 서는 안개벽. 처음부터 막혀 있고, 바깥에서 F로 통과하면 보스전이 시작된다
// 안개 이펙트는 BP의 On Entrance State Changed에서 Blocked일 때 켜고 Disabled일 때 끈다
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeBossFogWall : public ALeeBossEntranceBase
{
	GENERATED_BODY()

protected:
	virtual ELeeBossEntranceState GetActivatedState() const override { return ELeeBossEntranceState::Blocked; }
};
