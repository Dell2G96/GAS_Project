#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ConeTrace.generated.h"

// 모래 뿌리기처럼 "앞으로 퍼지는" 근접 공격의 히트 판정용 AnimNotifyState.
// 소켓 위치를 꼭짓점으로 하는 전방 원뿔(거리 + 좌우 각도) 안의 대상을 찾아,
// ANS_WeaponTrace와 동일한 GameplayEvent 규약(Abilities.Enemy.Trace + HitResult)으로 공격자에게 전송한다.
UCLASS()
class GAS_PROJECT_API UANS_ConeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

public:
	// 원뿔 꼭짓점 소켓 (캐릭터 메시). 없으면 캐릭터 위치를 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Shape")
	FName OriginSocket = "SandAttack";

	// 원뿔 길이 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Shape", meta=(ClampMin="10.0"))
	float Range = 450.f;

	// 원뿔 반각 (정면 기준 좌우 각각 허용 각도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Shape", meta=(ClampMin="1.0", ClampMax="180.0"))
	float HalfAngleDeg = 40.f;

	// 대상 탐색용 충돌 채널
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 벽 뒤 대상은 맞지 않도록 시야(Visibility) 검사
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Collision")
	bool bCheckLineOfSight = true;

	// 지정한 태도(기본 Hostile)인 대상만 히트 처리
	UPROPERTY(EditAnywhere, Category="ConeTrace|Team")
	TEnumAsByte<ETeamAttitude::Type> TargetTeam = ETeamAttitude::Hostile;

	// 서버에서만 판정(멀티 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Network")
	bool bServerOnly = true;

	// 디버그 드로우
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ConeTrace|Debug")
	bool bDrawDebug = false;

private:
	// 동일 노티가 여러 캐릭터에 깔릴 수 있으니 MeshComp별로 이미 맞춘 액터를 보관 (중복 히트 방지)
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSet<TWeakObjectPtr<AActor>>> HitActorsMap;

	// 대상이 원뿔 안에 있고 시야가 막히지 않았는지 검사
	bool IsInsideCone(const UWorld* World, const AActor* OwnerActor, const FVector& Origin, const FVector& Forward, AActor* Candidate) const;

	// 팀 필터 통과 시 공격자에게 Abilities.Enemy.Trace 이벤트 전송
	void HandleHit(AActor* OwnerActor, AActor* HitActor, const FVector& Origin) const;
};
