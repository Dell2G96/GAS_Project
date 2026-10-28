#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

/*
Gameplay Debugger "Debugger" 카테고리: 
 - 내 플레이어(Player)와 선택한 액터(Target)의 HP/스태미나와 보유 GameplayTag를 화면에 출력한다
 - (GAS_Project 모듈에 의존하지 않도록 어트리뷰트를 이름으로 찾는다)
*/
class FDebugger : public FGameplayDebuggerCategory
{
public:
	FDebugger();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	// 액터 한 명분의 표시 정보 (Player / Target 공용)
	struct FActorStat
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

	// 서버에서 수집해 클라이언트로 복제되는 데이터
	struct FRepData
	{
		FActorStat Player;	// 디버거를 연 플레이어의 Pawn
		FActorStat Target;	// 선택한 DebugActor

		void Serialize(FArchive& Ar);
	};

	// 액터 하나의 ASC에서 HP/스태미나와 태그를 수집
	static void CollectActorStat(AActor* InActor, FActorStat& OutStat);

	// 액터 하나의 정보를 화면에 출력
	static void DrawActorStat(const TCHAR* InTitle, const FActorStat& InStat, FGameplayDebuggerCanvasContext& CanvasContext);

	FRepData DataPack;
};
#endif
