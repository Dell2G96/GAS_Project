#include "DebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AttributeSet.h"

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

// 데이터팩 직렬화 (서버 → 클라이언트 복제용)
void FDebugger::FRepData::Serialize(FArchive& Ar)
{
	Ar << ActorName;
	Ar << bHasASC;
	Ar << Health;
	Ar << MaxHealth;
	Ar << Stamina;
	Ar << MaxStamina;
	Ar << Tags;
}

// (서버) 선택한 액터의 ASC에서 HP/스태미나와 태그를 수집
void FDebugger::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (DebugActor == nullptr)
		return;

	DataPack.ActorName = DebugActor->GetName();

	UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(DebugActor);
	DataPack.bHasASC = (asc != nullptr);
	if (asc == nullptr)
		return;

	DataPack.Health     = DebuggerCategoryHelper::FindAttributeValue(asc, "Health");
	DataPack.MaxHealth  = DebuggerCategoryHelper::FindAttributeValue(asc, "MaxHealth");
	DataPack.Stamina    = DebuggerCategoryHelper::FindAttributeValue(asc, "Stamina");
	DataPack.MaxStamina = DebuggerCategoryHelper::FindAttributeValue(asc, "MaxStamina");
	DataPack.Tags       = asc->GetOwnedGameplayTags().ToStringSimple();
}

// (클라이언트) 복제된 데이터를 화면에 출력
void FDebugger::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (DataPack.ActorName.IsEmpty())
	{
		CanvasContext.Printf(TEXT("{red}DebugActor 없음"));
		return;
	}

	CanvasContext.Printf(TEXT("{yellow}Actor : {white}%s"), *DataPack.ActorName);

	if (DataPack.bHasASC == false)
	{
		CanvasContext.Printf(TEXT("{red}AbilitySystemComponent 없음"));
		return;
	}

	CanvasContext.Printf(TEXT("{yellow}Health : {green}%.1f / %.1f"), DataPack.Health, DataPack.MaxHealth);
	CanvasContext.Printf(TEXT("{yellow}Stamina : {cyan}%.1f / %.1f"), DataPack.Stamina, DataPack.MaxStamina);
	CanvasContext.Printf(TEXT("{yellow}Tags : {white}%s"), DataPack.Tags.IsEmpty() ? TEXT("None") : *DataPack.Tags);
}
#endif
