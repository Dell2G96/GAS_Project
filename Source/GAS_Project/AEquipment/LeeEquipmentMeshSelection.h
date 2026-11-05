#pragma once
#include "CoreMinimal.h"
class UMeshComponent;

/** 캐릭터에 장비가 부착된 소켓으로 메시를 찾는다. 잘못된 소켓은 다른 장비로 대체하지 않는다. */
namespace LeeEquipmentMeshSelection
{
	GAS_PROJECT_API TArray<UMeshComponent*> FindMeshes(AActor* Owner, FName EquipmentAttachSocket);
	GAS_PROJECT_API TArray<UMeshComponent*> FindPreviewMeshes(const AActor* Owner);
}
