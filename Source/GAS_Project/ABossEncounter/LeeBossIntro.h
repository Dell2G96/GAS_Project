// 26.11.12 신규 - 보스 인트로 연출 베이스

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LeeBossIntro.generated.h"

// 보스 인트로 연출의 공통 형태 (Strategy). 포효·시네마틱 등을 이 클래스를 상속해 갈아끼운다
// 연출이 끝나면 반드시 OnFinished를 한 번 호출해야 전투가 시작된다
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class GAS_PROJECT_API ULeeBossIntro : public UObject
{
	GENERATED_BODY()

public:
	// [서버] 연출을 시작한다
	virtual void StartIntro(AActor* Boss, FSimpleDelegate OnFinished) PURE_VIRTUAL(ULeeBossIntro::StartIntro, );

	// [서버] 진행 중인 연출을 정리한다 (완료 알림은 보내지 않음)
	virtual void CancelIntro() {}
};
