// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeDevelopmentStatics.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "GAS_Project/ADevelopment/LeeDeveloperSettings.h"

bool ULeeDevelopmentStatics::ShouldSkipDirectlyToGameplay()
{
#if WITH_EDITOR
    /** 에디터에서만 동작하며, 전체 게임 흐름 테스트 설정에 따라 결정합니다 */
    if (GIsEditor)
    {
       return !GetDefault<ULeeDeveloperSettings>()->bTestFullGameFlowInPIE;
    }
#endif
    return false;
}

bool ULeeDevelopmentStatics::ShouldLoadCosmeticBackgrounds()
{
#if WITH_EDITOR
    /** 에디터에서 화장품 배경 로드 여부를 개발자 설정에 따라 결정합니다 */
    if (GIsEditor)
    {
       return !GetDefault<ULeeDeveloperSettings>()->bSkipLoadingCosmeticBackgroundsInPIE;
    }
#endif
    return true;
}

bool ULeeDevelopmentStatics::CanPlayerBotsAttack()
{
#if WITH_EDITOR
    /** 에디터에서 봇 공격 허용 여부를 개발자 설정에 따라 결정합니다 */
    if (GIsEditor)
    {
       return GetDefault<ULeeDeveloperSettings>()->bAllowPlayerBotsToAttack;
    }
#endif
    return true;
}

/** 
 * Play-In-Editor에서 '서버' 치트를 실행할 가장 적절한 월드를 찾습니다
 * 전용 서버가 있으면 전용 서버 월드, 없으면 리슨 서버 또는 첫 번째 월드 반환
 */
UWorld* ULeeDevelopmentStatics::FindPlayInEditorAuthorityWorld()
{
    check(GEngine);

    // 서버 월드를 찾습니다 (PIE 월드 중 전용 서버 우선, 없으면 리슨 서버, 없으면 첫 번째 월드)
    UWorld* ServerWorld = nullptr;
#if WITH_EDITOR
    for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
    {
       if (WorldContext.WorldType == EWorldType::PIE)
       {
          if (UWorld* TestWorld = WorldContext.World())
          {
             if (WorldContext.RunAsDedicated)
             {
                // 이상적인 경우: 전용 서버
                ServerWorld = TestWorld;
                break;
             }
             else if (ServerWorld == nullptr)
             {
                ServerWorld = TestWorld;
             }
             else
             {
                // 이미 후보가 있으면 더 나은 월드인지 확인 (NetMode가 낮을수록 우선순위 높음)
                if (TestWorld->GetNetMode() < ServerWorld->GetNetMode())
                {
                   ServerWorld = TestWorld;
                }
             }
          }
       }
    }
#endif

    return ServerWorld;
}

/** 모든 블루프린트 자산을 가져옵니다 */
TArray<FAssetData> ULeeDevelopmentStatics::GetAllBlueprints()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    TArray<FAssetData> BlueprintList;
    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    /** 블루프린트 클래스 경로로 필터링하여 모든 블루프린트 목록을 가져옵니다 */
    AssetRegistryModule.Get().GetAssets(Filter, BlueprintList);

    return BlueprintList;
}

/** 
 * 지정된 이름의 블루프린트 클래스를 찾습니다
 * @param TargetNameRaw 검색할 타겟 이름 ( _C 접미사 제거)
 * @param DesiredBaseClass 원하는 기본 클래스
 */
UClass* ULeeDevelopmentStatics::FindBlueprintClass(const FString& TargetNameRaw, UClass* DesiredBaseClass)
{
    FString TargetName = TargetNameRaw;
    /** 블루프린트 클래스 이름에서 _C 접미사를 제거합니다 */
    TargetName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);

    TArray<FAssetData> BlueprintList = ULeeDevelopmentStatics::GetAllBlueprints();
    for (const FAssetData& AssetData : BlueprintList)
    {
       /** 자산 이름 또는 전체 경로가 일치하는지 확인합니다 */
       if ((AssetData.AssetName.ToString() == TargetName) || (AssetData.GetObjectPathString() == TargetName))
       {
          if (UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset()))
          {
             if (UClass* GeneratedClass = BP->GeneratedClass)
             {
                /** 원하는 기본 클래스의 자식인지 확인합니다 */
                if (GeneratedClass->IsChildOf(DesiredBaseClass))
                {
                   return GeneratedClass;
                }
             }
          }
       }
    }

    return nullptr;
}

/** 
 * 치트 콘솔에서 사용할 수 있도록 최적화된 짧은 이름으로 클래스를 찾습니다
 * @param SearchToken 검색 토큰 (짧은 이름)
 * @param DesiredBaseClass 원하는 기본 클래스
 * @param bLogFailures 실패 시 로그 출력 여부
 */
UClass* ULeeDevelopmentStatics::FindClassByShortName(const FString& SearchToken, UClass* DesiredBaseClass, bool bLogFailures)
{
    check(DesiredBaseClass);

    FString TargetName = SearchToken;

    // 네이티브 클래스와 로드된 자산을 먼저 확인한 후 에셋 레지스트리에 의존합니다
    bool bIsValidClassName = true;
    /** 유효한 클래스 이름인지 확인합니다 */
    if (TargetName.IsEmpty() || TargetName.Contains(TEXT(" ")))
    {
       bIsValidClassName = false;
    }
    else if (!FPackageName::IsShortPackageName(TargetName))
    {
       if (TargetName.Contains(TEXT(".")))
       {
          // type'path' 형식을 path로 변환 (따옴표가 없으면 원본 반환)
          TargetName = FPackageName::ExportTextPathToObjectPath(TargetName);

          FString PackageName;
          FString ObjectName;
          TargetName.Split(TEXT("."), &PackageName, &ObjectName);

          const bool bIncludeReadOnlyRoots = true;
          FText Reason;
          /** 긴 패키지 이름 유효성 검사 */
          if (!FPackageName::IsValidLongPackageName(PackageName, bIncludeReadOnlyRoots, &Reason))
          {
             bIsValidClassName = false;
          }
       }
       else
       {
          bIsValidClassName = false;
       }
    }

    UClass* ResultClass = nullptr;
    if (bIsValidClassName)
    {
       /** 느린 클래스 검색으로 네이티브 클래스 찾기 시도 */
       ResultClass = UClass::TryFindTypeSlow<UClass>(TargetName);
    }

    // 아직 아무것도 찾지 못했다면, 요구사항에 맞는 블루프린트를 에셋 레지스트리에서 찾습니다
    if (ResultClass == nullptr)
    {
       ResultClass = FindBlueprintClass(TargetName, DesiredBaseClass);
    }

    // 찾은 클래스를 검증합니다
    if (ResultClass != nullptr)
    {
       /** 원하는 기본 클래스의 자식인지 확인합니다 */
       if (!ResultClass->IsChildOf(DesiredBaseClass))
       {
          if (bLogFailures)
          {
             /** 타입 불일치 경고 로그 */
             UE_LOG(LogConsoleResponse, Warning, TEXT("에셋 %s을(를) 찾았으나 %s 타입이 아닙니다"), *ResultClass->GetPathName(), *DesiredBaseClass->GetName());
          }
          ResultClass = nullptr;
       }
    }
    else
    {
       if (bLogFailures)
       {
          /** 클래스 찾기 실패 로그 */
          UE_LOG(LogConsoleResponse, Warning, TEXT("%s 타입의 %s 클래스를 찾지 못했습니다"), *DesiredBaseClass->GetName(), *SearchToken);
       }
    }

    return ResultClass;
}