#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeHitOverride.h"
#include "ANS_WeaponTrace.generated.h"

class UMeshComponent;
class ULeeEquipmentInstance;

UENUM(BlueprintType)
enum class ELeeMeleeTraceSource : uint8
{
	EquippedWeapon UMETA(DisplayName="Equipped Weapon"),
	CharacterMesh UMETA(DisplayName="Character Mesh")
};

USTRUCT(BlueprintType)
struct FLeeWeaponTraceTarget
{
	GENERATED_BODY()
	/** WID에서 캐릭터에 장비 액터를 부착한 소켓. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName EquipmentAttachSocket;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName StartSocket = TEXT("WeaponBase");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName EndSocket = TEXT("WeaponTip");
};

// 장착 무기 또는 노티파이가 재생되는 캐릭터 메시의 소켓/본으로 근접 공격을 판정한다.
// 손 소켓을 훑는 ANS_ToggleTrace(짐승용)와 달리, LeeEquipment로 스폰된 무기 액터의
// 메시 소켓(자루→칼끝 2점)을 매 틱 스윕하고, 히트 시 ANS_ToggleTrace와 동일한
// GameplayEvent 규약(Abilities.Enemy.Trace + HitResult)을 공격자에게 전송한다.
UCLASS()
class GAS_PROJECT_API UANS_WeaponTrace : public UAnimNotifyState, public ILeeHitOverrideProvider
{
	GENERATED_BODY()

public:
	UANS_WeaponTrace();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

public:
	/** 기존 몽타주는 장착 무기를 기본값으로 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Source")
	ELeeMeleeTraceSource TraceSource = ELeeMeleeTraceSource::EquippedWeapon;

	/** 비어 있으면 기존 첫 무기 탐색. 지정 시 각 장비를 독립적으로 판정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Targets", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::EquippedWeapon", EditConditionHides))
	TArray<FLeeWeaponTraceTarget> WeaponTargets;

	/** Character Mesh 모드의 시작 소켓 또는 본. 끝과 같으면 한 지점의 구체 판정. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Body", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::CharacterMesh", EditConditionHides))
	FName BodyStartSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Body", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::CharacterMesh", EditConditionHides))
	FName BodyEndSocket = NAME_None;

	// 무기 메시에서 판정 구간의 시작(자루) 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Sockets", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::EquippedWeapon", EditConditionHides))
	FName WeaponBaseSocket = "WeaponBase";

	// 무기 메시에서 판정 구간의 끝(칼끝) 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Sockets", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::EquippedWeapon", EditConditionHides))
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
	UPROPERTY(EditAnywhere, Category="WeaponTrace|Equipment", meta=(EditCondition="TraceSource == ELeeMeleeTraceSource::EquippedWeapon", EditConditionHides))
	TSubclassOf<ULeeEquipmentInstance> WeaponInstanceType;

	// 자기 자신 무시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Filter")
	bool bIgnoreOwner = true;

	// 서버에서만 판정(멀티 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Network")
	bool bServerOnly = true;

	/** [타격별 Override] 이 판정 구간의 히트에만 적용할 데미지·넉백. 체크하지 않으면 공격 GA의 AttackData 값 사용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|HitOverride")
	FLeeHitOverride HitOverride;

	// ILeeHitOverrideProvider — 공격 GA가 히트 이벤트의 OptionalObject로 이 값을 읽는다
	virtual const FLeeHitOverride& GetHitOverride() const override { return HitOverride; }

	// 디버그 드로우
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WeaponTrace|Debug")
	bool bDrawDebug = false;

private:
	struct FWeaponTraceSourceData
	{
		TWeakObjectPtr<UMeshComponent> TraceMesh;
		FName StartSocket;
		FName EndSocket;
		// 선택된 무기별로 대상당 1회. 서로 다른 무기의 적중은 별도로 허용한다.
		TSet<TWeakObjectPtr<AActor>> HitActors;
	};
	struct FWeaponTraceRuntimeData
	{
		TArray<FWeaponTraceSourceData> Sources;
	};

	// 동일 노티가 여러 캐릭터에 깔릴 수 있으니 MeshComp 기준으로 상태 보관
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FWeaponTraceRuntimeData> RuntimeMap;

	// 이 머신에서 판정을 돌려야 하는지(서버 권위)
	bool ShouldRunTrace(const USkeletalMeshComponent* MeshComp) const;

	// LeeEquipment에서 두 소켓을 모두 가진 무기 메시를 찾아 반환(없으면 nullptr)
	UMeshComponent* ResolveWeaponMesh(USkeletalMeshComponent* MeshComp) const;

	UMeshComponent* ResolveTraceMesh(USkeletalMeshComponent* MeshComp) const;
	void GetTraceSocketNames(FName& OutStart, FName& OutEnd) const;
	bool HasTraceSockets(const UMeshComponent* Mesh) const;
	void ResolveTraceSources(USkeletalMeshComponent* MeshComp, TArray<FWeaponTraceSourceData>& OutSources) const;
	friend class FLeeWeaponTraceSourceTest;
	friend class FLeeDualWeaponTraceTest;

	// 히트 대상에 규약대로 GameplayEvent 전송(팀 필터 포함)
	void HandleHit(AActor* OwnerActor, const FHitResult& HitResult) const;
};

