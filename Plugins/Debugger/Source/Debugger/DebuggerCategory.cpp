#include "DebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AttributeSet.h"
#include "GameFramework/PlayerController.h"

namespace DebuggerCategoryHelper
{
	// ASC가 가진 AttributeSet들 중에서 이름이 InName인 어트리뷰트 값을 찾아 반환 (없으면 0)
	static float FindAttributeValue(const UAbilitySystemComponent* InASC, const FName& InName)
	{
		for (const UAttributeSet* set : InASC->GetSpawnedAttributes())
		{
			if (set == nullptr)
				continue;

			FProperty* property = FindFProperty<FProperty>(set->GetClass(), InName);
			if (property == nullptr)
				continue;

			return InASC->GetNumericAttribute(FGameplayAttribute(property));
		}

		return 0.f;
	}
}

// 데이터팩을 복제 대상으로 등록
FDebugger::FDebugger()
{
	bShowOnlyWithDebugActor = false;
	SetDataPackReplication<FRepData>(&DataPack);
}

// RegisterCategory에 넘기는 카테고리 생성 함수
TSharedRef<FGameplayDebuggerCategory> FDebugger::MakeInstance()
{
	return MakeShareable(new FDebugger());
}

// 액터 한 명분 데이터 직렬화
void FDebugger::FActorStat::Serialize(FArchive& Ar)
{
	Ar << ActorName;
	Ar << bHasASC;
	Ar << Health;
	Ar << MaxHealth;
	Ar << Stamina;
	Ar << MaxStamina;
	Ar << Tags;
}

// 데이터팩 직렬화 (서버 → 클라이언트 복제용). Player → Target 순서로 처리
void FDebugger::FRepData::Serialize(FArchive& Ar)
{
	Player.Serialize(Ar);
	Target.Serialize(Ar);
}

// 액터 하나의 ASC에서 HP/스태미나와 태그를 수집 (액터가 없으면 OutStat을 기본값 그대로 둠)
void FDebugger::CollectActorStat(AActor* InActor, FActorStat& OutStat)
{
	if (InActor == nullptr)
		return;

	OutStat.ActorName = InActor->GetName();

	UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	OutStat.bHasASC = (asc != nullptr);
	if (asc == nullptr)
		return;

	OutStat.Health     = DebuggerCategoryHelper::FindAttributeValue(asc, "Health");
	OutStat.MaxHealth  = DebuggerCategoryHelper::FindAttributeValue(asc, "MaxHealth");
	OutStat.Stamina    = DebuggerCategoryHelper::FindAttributeValue(asc, "Stamina");
	OutStat.MaxStamina = DebuggerCategoryHelper::FindAttributeValue(asc, "MaxStamina");
	OutStat.Tags       = asc->GetOwnedGameplayTags().ToStringSimple();
}

// (서버) 내 플레이어 Pawn과 선택한 액터의 정보를 수집
void FDebugger::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (OwnerPC != nullptr)
		CollectActorStat(OwnerPC->GetPawn(), DataPack.Player);

	CollectActorStat(DebugActor, DataPack.Target);
}

// 액터 하나의 정보를 [제목] 블록으로 출력
void FDebugger::DrawActorStat(const TCHAR* InTitle, const FActorStat& InStat, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("{orange}[%s]"), InTitle);

	if (InStat.ActorName.IsEmpty())
	{
		CanvasContext.Printf(TEXT("{red}  액터 없음"));
		return;
	}

	CanvasContext.Printf(TEXT("{yellow}  Actor : {white}%s"), *InStat.ActorName);

	if (InStat.bHasASC == false)
	{
		CanvasContext.Printf(TEXT("{red}  AbilitySystemComponent 없음"));
		return;
	}

	CanvasContext.Printf(TEXT("{yellow}  Health : {green}%.1f / %.1f"), InStat.Health, InStat.MaxHealth);
	CanvasContext.Printf(TEXT("{yellow}  Stamina : {cyan}%.1f / %.1f"), InStat.Stamina, InStat.MaxStamina);
	CanvasContext.Printf(TEXT("{yellow}  Tags : {white}%s"), InStat.Tags.IsEmpty() ? TEXT("None") : *InStat.Tags);
}

// (클라이언트) 복제된 데이터를 Player → Target 순서로 화면에 출력
void FDebugger::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	DrawActorStat(TEXT("Player"), DataPack.Player, CanvasContext);
	DrawActorStat(TEXT("Target"), DataPack.Target, CanvasContext);
}
#endif
