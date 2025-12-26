// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AttackTrace.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UANS_AttackTrace : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName StartSocket = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName EndSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	bool bUseWeaponMeshSockets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	float Radius = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	bool bSingleHitPerActor = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	bool bClientAlsoTrace = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FGameplayTag HitTagEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	TSubclassOf<class UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	bool bDrawDebug = true;

	// 서버만 실제 판정/적용을 수행 (클라는 디버그만)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Net")
	bool bServerOnly = true;
	
private:
	struct FTraceState
	{
		TWeakObjectPtr<USkeletalMeshComponent> TraceMesh;
		FVector PreStart = FVector::ZeroVector;
		FVector PreEnd = FVector::ZeroVector;
		TSet<TWeakObjectPtr<AActor>> AlreadyHitActors;
		bool bInitaillzed = false;

		void Reset();
	};
	
	// MeshComp별 상태(동일 노티가 여러 캐릭에 깔릴 수 있으니 Map으로)
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FTraceState> States;

	void DoTrace(USkeletalMeshComponent* MeshComp, float DeltaTime) const;
	void CollectHits(UWorld* World, const FVector& A, const FVector& B, float InRadius, ECollisionChannel Channel,
					 const TArray<AActor*>& Ignore, TArray<FHitResult>& OutHits) const;
	void HandleHit(USkeletalMeshComponent* MeshComp, const FHitResult& Hit, FTraceState& State) const;
	bool ShouldRunOnThisMachine(const AActor* Owner) const;

	// 캐릭터마다 별개의 상태를 갖도록 메쉬 포인터 키로 보관
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FTraceState> StateMap;

	FTraceState& GetState(USkeletalMeshComponent* MeshComp);

	USkeletalMeshComponent* ResolveTraceMesh(USkeletalMeshComponent* DefaultMesh) const;

	void DoSweepAndApply(
		USkeletalMeshComponent* MeshComp,
		FTraceState& State,
		const FVector& From,
		const FVector& To,
		UWorld* World
	);

	void SendGasEventOrApplyGE(
		AActor* InstigatorActor,
		AActor* TargetActor,
		const FHitResult& Hit
	) const;

	bool ShouldApplyOnThisMachine(AActor* Owner) const;
	
};

