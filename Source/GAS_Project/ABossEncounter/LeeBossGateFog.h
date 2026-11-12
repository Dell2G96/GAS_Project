// 26.11.12 신규 - 필드 보스 대문의 "첫 도전" 안개 표시

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeeBossGateFog.generated.h"

class UStaticMeshComponent;

// 필드 맵의 보스 대문 앞에 놓는 안개 (표시 전용, 충돌·상호작용 없음)
// 시작할 때 이 PC의 세이브를 보고, BossId 보스에 한 번도 도전하지 않았으면 안개를 보이고 도전한 적이 있으면 숨긴다
// 실제 입장은 대문 앞 보스 요청(B_BossRequest)이 맡고, 아레나 안개벽은 보스 맵의 ALeeBossFogWall이 맡는다
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeBossGateFog : public AActor
{
	GENERATED_BODY()

public:
	ALeeBossGateFog();

protected:
	virtual void BeginPlay() override;

	// 확인할 보스 ID (보스 맵 BossEncounterManager의 BossId와 같아야 한다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|BossGateFog")
	FName BossId = TEXT("Boss_Stone");

	// 안개 평면 (메시·머티리얼은 레벨이나 BP에서 지정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|BossGateFog")
	TObjectPtr<UStaticMeshComponent> FogMesh;
};
