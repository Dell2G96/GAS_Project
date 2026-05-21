// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbilityTargetData_Finish.h"

#include "Animation/AnimMontage.h"

bool FLeeGameplayAbilityTargetData_Finish::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << PairID;

	UAnimMontage* Montage = VictimMontage.Get();
	Ar << Montage;
	if (Ar.IsLoading())
	{
		VictimMontage = Montage;
	}

	FVector Location = AttackerWorldTransform.GetLocation();
	FQuat Rotation = AttackerWorldTransform.GetRotation();
	FVector Scale = AttackerWorldTransform.GetScale3D();

	Ar << Location;
	Ar << Rotation;
	Ar << Scale;

	if (Ar.IsLoading())
	{
		AttackerWorldTransform = FTransform(Rotation, Location, Scale);
	}

	uint8 FinishTypeValue = static_cast<uint8>(FinishType);
	Ar << FinishTypeValue;
	if (Ar.IsLoading())
	{
		FinishType = static_cast<ELeeFinishType>(FinishTypeValue);
	}

	bOutSuccess = true;
	return true;
}
