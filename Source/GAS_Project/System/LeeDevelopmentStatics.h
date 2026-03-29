// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"


#include "LeeDevelopmentStatics.generated.h"

class UClass;
class UObject;
class UWorld;
struct FAssetData;
struct FFrame;

UCLASS()
class GAS_PROJECT_API ULeeDevelopmentStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 게임 로직이 매치 워밍업/플레이어 대기 등의 단계를 건너뛰고 바로 게임플레이로 이동해야하는지 여부
	// 에디터에서만 bTetfullGameFlowInPIE(개발자설정) 가 false 일 떄 true 반환하며, 그 외에는 항상 false
	UFUNCTION(BlueprintCallable, Category="Lyra")
	static bool ShouldSkipDirectlyToGameplay();

	/** 
	* 에디터에서 화장품 배경을 로드해야 하는지 여부
	* 에디터에서 bSkipLoadingCosmeticBackgroundsInPIE(Lyra 개발자 설정)가 true일 때만 false를 반환하며, 그 외에는 항상 true
	*/
	UFUNCTION(BlueprintCallable, Category = "Lyra", meta=(ExpandBoolAsExecs="ReturnValue"))
	static bool ShouldLoadCosmeticBackgrounds();

	/** 
   * 봇 플레이어가 공격할 수 있는지 여부
   * 에디터에서 bSkipLoadingCosmeticBackgroundsInPIE(Lyra 개발자 설정)가 true일 때만 false를 반환하며, 그 외에는 항상 true
   */
	UFUNCTION(BlueprintCallable, Category = "Lyra")
	static bool CanPlayerBotsAttack();

	/** 
	 * '서버' 치트를 실행할 가장 적절한 플레이-인-에디터 월드를 찾습니다
	 * standalone 실행 시 유일한 월드, 리슨 서버, 또는 전용 서버일 수 있습니다
	 */
	static UWorld* FindPlayInEditorAuthorityWorld();

	/** 
	 * 치트 콘솔을 통해 사용할 때의 사용성을 개선하기 위한 휴리스틱을 사용해 짧은 이름으로 클래스를 찾습니다
	 */
	static UClass* FindClassByShortName(const FString& SearchToken, UClass* DesiredBaseClass, bool bLogFailures = true);

	template <typename DesiredClass>
	/** 템플릿 버전의 짧은 이름으로 클래스 찾기 */
	static TSubclassOf<DesiredClass> FindClassByShortName(const FString& SearchToken, bool bLogFailures = true)
	{
		return FindClassByShortName(SearchToken, DesiredClass::StaticClass(), bLogFailures);
	}

private:
	/** 모든 블루프린트를 가져옵니다 */
	static TArray<FAssetData> GetAllBlueprints();
	
	/** 지정된 이름의 블루프린트 클래스를 찾습니다 */
	static UClass* FindBlueprintClass(const FString& TargetNameRaw, UClass* DesiredBaseClass);
	
};
