// 26.11.11 신규 - 슈퍼아머 구간 AnimNotifyState

#include "LeeAnimNotifyState_SuperArmor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/MyTags.h"

// 생성자 — 에디터 타임라인 색 지정
ULeeAnimNotifyState_SuperArmor::ULeeAnimNotifyState_SuperArmor()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 200, 0, 255);
#endif
}

// 구간 시작 — 서버에서 슈퍼아머 태그 추가
void ULeeAnimNotifyState_SuperArmor::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddLooseGameplayTag(GetArmorTag());
	}
}

// 구간 끝(몽타주가 끊겨도 호출됨) — 서버에서 슈퍼아머 태그 제거
void ULeeAnimNotifyState_SuperArmor::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	const FGameplayTag ArmorTag = GetArmorTag();
	if (ASC && ASC->GetTagCount(ArmorTag) > 0)
	{
		ASC->RemoveLooseGameplayTag(ArmorTag);
	}
}

// 몽타주 타임라인에 보일 이름
FString ULeeAnimNotifyState_SuperArmor::GetNotifyName_Implementation() const
{
	switch (ArmorLevel)
	{
	case ELeeSuperArmorLevel::Heavy:
		return TEXT("Super Armor [Heavy]");
	case ELeeSuperArmorLevel::NoDamage:
		return TEXT("Super Armor [No Damage]");
	default:
		return TEXT("Super Armor [Light]");
	}
}

// ArmorLevel → 슈퍼아머 태그
FGameplayTag ULeeAnimNotifyState_SuperArmor::GetArmorTag() const
{
	switch (ArmorLevel)
	{
	case ELeeSuperArmorLevel::Heavy:
		return MyTags::Souls::Status_SuperArmor_Heavy;
	case ELeeSuperArmorLevel::NoDamage:
		return MyTags::Souls::Status_SuperArmor_NoDamage;
	default:
		return MyTags::Souls::Status_SuperArmor_Light;
	}
}
