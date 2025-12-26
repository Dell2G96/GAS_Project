// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ToggleTrace.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EMeleeTraceHand : uint8
{
	None  UMETA(DisplayName="None"),
	Left  UMETA(DisplayName="Left"),
	Right UMETA(DisplayName="Right"),
	Both  UMETA(DisplayName="Both"),
};

USTRUCT()
struct FMeleeTraceRuntimeData
{
	GENERATED_BODY()

	FVector PrevLeft = FVector::ZeroVector;
	FVector PrevRight = FVector::ZeroVector;

	TSet<TWeakObjectPtr<AActor>> HitActorsLeft;
	TSet<TWeakObjectPtr<AActor>> HitActorsRight;

	bool bHasPrevLeft = false;
	bool bHasPrevRight = false;
};

UCLASS()
class GAS_PROJECT_API UANS_ToggleTrace : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	//----------------------------------------------------------------------------//
	UPROPERTY(EditAnywhere, Category="GAS|Team")
	TEnumAsByte<ETeamAttitude::Type> TargetTeam = ETeamAttitude::Hostile;

	
	// 어떤 손을 트레이스할지 (공격별로 노티파이에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace")
	EMeleeTraceHand TraceHand = EMeleeTraceHand::Both;

	// 왼손/오른손 소켓 이름 (캐릭터 스켈레탈 메시 소켓)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Sockets")
	FName LeftSocketName = "hand_l_socket";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Sockets")
	FName RightSocketName = "hand_r_socket";

	// 스윕 반경(무기/손 타격 범위)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Shape")
	float TraceRadius = 12.f;

	// 히트 판정용 충돌 채널
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Collision")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// 자기 자신/아군 제외용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Filter")
	bool bIgnoreOwner = true;

	// 서버에서만 트레이스(멀티 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Network")
	bool bServerOnly = true;

	// 디버그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|Debug")
	bool bDrawDebug = false;

	// 히트 시 호출할 게임플레이 이벤트 태그(원하면 GAS로 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace|GAS")
	FName HitEventName = "MeleeHit"; // 간단히 Name으로. 프로젝트에 맞게 바꿔도 됨.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace")
	TSubclassOf<class UGameplayEffect> DefaultDamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeleeTrace")
	FGameplayTag HitTagEvent;
	

private:
	// NotifyState 인스턴스는 애셋에 붙고, 런타임 데이터는 캐릭터별로 따로 들고 있어야 안전함
	// MeshComp 기준으로 맵에 저장
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FMeleeTraceRuntimeData> RuntimeMap;

private:
	bool ShouldRunTrace(const USkeletalMeshComponent* MeshComp) const;
	bool IsHandEnabled(EMeleeTraceHand HandToCheck) const;

	bool GetSocketLocationSafe(USkeletalMeshComponent* MeshComp, const FName& SocketName, FVector& OutLocation) const;

	void DoHandTrace(
		USkeletalMeshComponent* MeshComp,
		AActor* OwnerActor,
		const FName& SocketName,
		FVector& InOutPrevPos,
		bool& InOutHasPrev,
		TSet<TWeakObjectPtr<AActor>>& InOutHitActorsThisHand,
		bool bLeftHand
	);

	void HandleHit(
		USkeletalMeshComponent* MeshComp,
		AActor* OwnerActor,
		const FHitResult& HitResult,
		bool bLeftHand
	) const;
	
	void DoDamageNew(struct FGameplayEventData Data) const;

	//----------------------------------------------------------------------------//

public:
	
	class ACCharacter* OwnerCharacter;
	
	
};


