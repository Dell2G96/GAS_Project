// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GAS_Project/AInteraction/IInteractableTarget.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "LeeSavePointActor.generated.h"

class ALeePlayerStart;
class USphereComponent;
class UStaticMeshComponent;

// 세이브 포인트 Actor — F 상호작용으로 휴식(GA_Rest) 옵션을 제공하고, 휴식한 플레이어의 부활 지점(PlayerStart)을 가진다
// BeginPlay/EndPlay에서 ULeeSavePointSubsystem에 등록/해제되어 SavePointId로 조회된다
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeSavePointActor : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	ALeeSavePointActor();

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder) override;
	//~End of IInteractableTarget interface

	FName GetSavePointId() const { return SavePointId; }
	ALeePlayerStart* GetRespawnPlayerStart() const { return RespawnPlayerStart; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 세이브 포인트 외형 (BP에서 메시 지정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|SavePoint")
	TObjectPtr<UStaticMeshComponent> SavePointMesh;

	// 상호작용 감지용 구체 (Interaction 채널에만 Overlap)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|SavePoint")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 세이브 포인트 고유 ID (레벨 안에서 겹치면 안 됨, Phase 3 세이브에서 사용)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lee|SavePoint")
	FName SavePointId;

	// 이 세이브 포인트에서 휴식한 플레이어가 부활할 PlayerStart (세이브 포인트 옆에 배치 후 지정, 일반 스폰 후보에서는 제외됨)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lee|SavePoint")
	TObjectPtr<ALeePlayerStart> RespawnPlayerStart;

	// 상호작용 옵션 (BP에서 InteractionAbilityToGrant = GA_Rest, Text = "휴식" 지정)
	UPROPERTY(EditAnywhere, Category = "Lee|SavePoint")
	FInteractionOption Option;
};
