#pragma once
#include "CoreMinimal.h"
class UMeshComponent;

/** 캐릭터에 장비가 부착된 소켓으로 메시를 찾는다. 잘못된 소켓은 다른 장비로 대체하지 않는다. */
namespace LeeEquipmentMeshSelection
{
	GAS_PROJECT_API TArray<UMeshComponent*> FindMeshes(AActor* Owner, FName EquipmentAttachSocket);
	GAS_PROJECT_API TArray<UMeshComponent*> FindPreviewMeshes(const AActor* Owner);

	/** 26.11.25 추가 - Owner가 든 무기 중 BaseSocket·TipSocket을 모두 가진 메시를 찾아 칼날 선분(손잡이→칼끝) 월드 위치를 돌려준다. 못 찾으면 false */
	GAS_PROJECT_API bool FindWeaponBlade(AActor* Owner, FName BaseSocket, FName TipSocket, FVector& OutBase, FVector& OutTip);
}
