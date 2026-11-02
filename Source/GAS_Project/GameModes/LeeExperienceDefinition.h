// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeExperienceDefinition.generated.h"

// Experience(맵)가 허용하는 플레이 방식. 필드는 1인 고정, 보스전만 외부 접속 허용
UENUM(BlueprintType)
enum class ELeePlayMode : uint8
{
	SingleField,		// 필드: 1인 고정, 외부 접속 거부
	MultiplayerBoss		// 보스전: 세션 참가 허용 (MaxPlayers까지)
};

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeeExperienceDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif
	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	TObjectPtr<class ULeePawnData> DefaultPawnData;

	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	TArray<FString> GameFeaturesToEnable;

	// 이 Experience의 플레이 방식 (GameSession::ApproveLogin에서 접속 허용 판단)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode")
	ELeePlayMode PlayMode = ELeePlayMode::SingleField;

	// MultiplayerBoss일 때 최대 인원 (호스트 포함)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode", meta=(ClampMin=1, EditCondition="PlayMode==ELeePlayMode::MultiplayerBoss"))
	int32 MaxPlayers = 3;

	// 26.11.02 추가 - 보스전 전멸 시 전원이 이동할 로비 맵 (로비 맵의 Experience도 MultiplayerBoss여야 접속이 유지된다)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode", meta=(EditCondition="PlayMode==ELeePlayMode::MultiplayerBoss"))
	TSoftObjectPtr<UWorld> BossLobbyMap;

	// 26.11.02 추가 - 전멸 후 YOU DIED를 보여주고 로비로 이동하기까지 대기 시간(초)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode", meta=(ClampMin=0, EditCondition="PlayMode==ELeePlayMode::MultiplayerBoss"))
	float WipeTravelDelay = 5.0f;

	// 26.11.02 추가 - 로비에서 전원 준비 후 카운트다운이 끝나면 다 같이 이동할 보스맵 (로비 Experience에 지정)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode", meta=(EditCondition="PlayMode==ELeePlayMode::MultiplayerBoss"))
	TSoftObjectPtr<UWorld> BossMap;

	// 26.11.02 추가 - 보스 처치 후 VICTORY를 보여주고 각자 자기 필드로 돌아가기까지 대기 시간(초)
	UPROPERTY(EditDefaultsOnly, Category="PlayMode", meta=(ClampMin=0, EditCondition="PlayMode==ELeePlayMode::MultiplayerBoss"))
	float VictoryTravelDelay = 10.0f;

	// 26.10.30 사용 안함 - 레거시 봇 스포너 방식(BotIdentifier → PawnData). Controller BP의 EnemyPawnData로 교체됨
	// // EnemyPawn Data 추가
	// UPROPERTY(EditDefaultsOnly, Category= Gameplay)
	// TMap<int32, TObjectPtr<const ULeePawnData>> EnemyPawnClasses;
	
	// Gameplay 용도에 맞게 분류의 목적으로 사용
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<class ULeeExperienceActionSet>> ActionSets;

	// 일반적인 GameFeatureAction의 역할
	UPROPERTY(EditDefaultsOnly,Instanced, Category="Actions")
	TArray<TObjectPtr<class UGameFeatureAction>> Actions;
};
