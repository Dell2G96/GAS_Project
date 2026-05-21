// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWeaponInstance.h"

ULeeWeaponInstance::ULeeWeaponInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

TSubclassOf<class UAnimInstance> ULeeWeaponInstance::PickBestAnimLayer(bool bEquipped,
	const FGameplayTagContainer& CosmeticTags) const
{
	const FLeeAnimLayerSelectionSet& SetToQuery = (bEquipped ? EquippedAnimSet : UnEquippedAnimSet);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

bool ULeeWeaponInstance::SelectRandomPair(ELeeFinishType Type, FLeeFinishMontagePair& OutPair) const
{
	const TArray<FLeeFinishMontagePair>* SourcePairs = nullptr;

	switch (Type)
	{
	case ELeeFinishType::Execution:
		SourcePairs = &ExecutionPairs;
		break;
	case ELeeFinishType::Assassination:
		SourcePairs = &AssassinationPairs;
		break;
	default:
		return false;
	}

	if (!SourcePairs || SourcePairs->IsEmpty())
	{
		return false;
	}

	const int32 StartIndex = FMath::RandRange(0, SourcePairs->Num() - 1);
	for (int32 Offset = 0; Offset < SourcePairs->Num(); ++Offset)
	{
		const int32 Index = (StartIndex + Offset) % SourcePairs->Num();
		const FLeeFinishMontagePair& Candidate = (*SourcePairs)[Index];
		if (Candidate.IsValidPair())
		{
			OutPair = Candidate;
			return true;
		}
	}

	return false;
}
