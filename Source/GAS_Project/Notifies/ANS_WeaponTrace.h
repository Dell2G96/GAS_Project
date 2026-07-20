#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_WeaponTrace.generated.h"

class UMeshComponent;
class ULeeEquipmentInstance;

// 무기 든 적의 근접 공격 히트 판정용 AnimNotifyState.
// 손 소켓을 훑는 ANS_ToggleTrace(짐승용)와 달리, LeeEquipment로 스폰된 무기 액터의
// 메시 소켓(자루→칼끝 2점)을 매 틱 스윕하고, 히트 시 ANS_ToggleTrace와 동일한
// GameplayEvent 규약(Abilities.Enemy.Trace + HitResult)을 공격자에게 전송한다.
UCLASS()
class GAS_PROJECT_API UANS_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UANS_WeaponTrace();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

public:
	// 무기 메시에서 판정 구간의 시작(자루) 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Sockets")
	FName WeaponBaseSocket = "WeaponBase";

	// 무기 메시에서 판정 구간의 끝(칼끝) 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Sockets")
	FName WeaponTipSocket = "WeaponTip";

	// 스윕 구체 반경(칼날 두께)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Shape")
	float TraceRadius = 8.f;

	// 히트 판정용 충돌 채널
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 지정한 태도(기본 Hostile)인 대상만 히트 처리
	UPROPERTY(EditAnywhere, Category="WeaponTrace|Team")
	TEnumAsByte<ETeamAttitude::Type> TargetTeam = ETeamAttitude::Hostile;

	// 어느 장비 인스턴스를 무기로 볼지 (기본: 근접무기 인스턴스). 두 소켓 존재 여부로 최종 판별
	UPROPERTY(EditAnywhere, Category="WeaponTrace|Equipment")
	TSubclassOf<ULeeEquipmentInstance> WeaponInstanceType;

	// 자기 자신 무시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Filter")
	bool bIgnoreOwner = true;

	// 서버에서만 판정(멀티 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Network")
	bool bServerOnly = true;

	// 디버그 드로우
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Debug")
	bool bDrawDebug = false;

private:
	// MeshComp별 런타임 데이터(무기 메시 캐시 + 중복 히트 방지 집합)
	struct FWeaponTraceRuntimeData
	{
		TWeakObjectPtr<UMeshComponent> WeaponMesh;
		TSet<TWeakObjectPtr<AActor>> HitActors;
	};

	// 동일 노티가 여러 캐릭터에 깔릴 수 있으니 MeshComp 기준으로 상태 보관
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FWeaponTraceRuntimeData> RuntimeMap;

	// 이 머신에서 판정을 돌려야 하는지(서버 권위)
	bool ShouldRunTrace(const USkeletalMeshComponent* MeshComp) const;

	// LeeEquipment에서 두 소켓을 모두 가진 무기 메시를 찾아 반환(없으면 nullptr)
	UMeshComponent* ResolveWeaponMesh(USkeletalMeshComponent* MeshComp) const;

	// 히트 대상에 규약대로 GameplayEvent 전송(팀 필터 포함)
	void HandleHit(AActor* OwnerActor, const FHitResult& HitResult) const;
};

