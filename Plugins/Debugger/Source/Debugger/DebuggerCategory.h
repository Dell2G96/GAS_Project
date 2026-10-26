#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

// Gameplay Debugger "Debugger" 카테고리: 선택한 액터의 HP/스태미나와 보유 GameplayTag를 화면에 출력한다
// (GAS_Project 모듈에 의존하지 않도록 어트리뷰트를 이름으로 찾는다)
class FDebugger : public FGameplayDebuggerCategory
{
public:
	FDebugger();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	// 서버에서 수집해 클라이언트로 복제되는 데이터
	struct FRepData
	{
		FString ActorName;
		bool bHasASC = false;
		float Health = 0.f;
		float MaxHealth = 0.f;
		float Stamina = 0.f;
		float MaxStamina = 0.f;
		FString Tags;

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;
};
#endif
