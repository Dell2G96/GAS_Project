 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeUserFacingExperience.generated.h"


class FString;
class UCommonSession_HostSessionRequest;
class UObject;
class UTexture2D;
class UUserWidget;
struct FFrame;

UCLASS(BlueprintType)
class GAS_PROJECT_API ULeeUserFacingExperience : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Experience", meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Experience", meta=(AllowedTypes="LeeExperienceDefinition"))
	FPrimaryAssetId ExperienceID;


	// 게임에 URL 옵션으로 전달할 인자
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TMap<FString, FString> ExtraArgs;

	// UI 에서 표시될 기본 Title
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText Text;

	// 부제목
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileSubTitle;

	// 해당 Experience 설명
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TitleDescription;

	// UI ICON
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TObjectPtr<UTexture2D> TileIcon;

	// 해당 Experience 에 진입할 떄 표시할 로딩 화면 위젯
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TSoftClassPtr<UUserWidget> LoadingScreenWidget;

	// true일 경우 QuickPlay 을 위한 기본 Experience 사용 UI에서 우선순위를 가짐
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bIsDefaultExperience = false;

	// true 일 경우, 프론트앤드 의 Experiece List에 표시됨
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bShowInFrontend = true;

	// true 일 경우, 게임의 Replay 기록됨
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bRecordReplay = false;

	// 해당 세션의 최대 플레이어 수
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	int32 MaxPlayerCount = 4;
	
	UFUNCTION(BlueprintCallable, blueprintPure = false, meta = (WorldContext = "WorldContextObject"))
	class UCommonSession_HostSessionRequest* CreateHostingRequest(const UObject* WorldContextObject) const;
};
