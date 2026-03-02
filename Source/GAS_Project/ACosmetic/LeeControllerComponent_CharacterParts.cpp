// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeControllerComponent_CharacterParts.h"

#include "LeePawnComponent_CharacterParts.h"

ULeeControllerComponent_CharacterParts::ULeeControllerComponent_CharacterParts(
	const FObjectInitializer& ObjectInitializer)
		:Super(ObjectInitializer)
{
}

void ULeeControllerComponent_CharacterParts::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (AController* OwningController = GetController<AController>())
		{
			OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);
		}
	}
}

void ULeeControllerComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllCharacterParts();
	Super::EndPlay(EndPlayReason);
}

class ULeePawnComponent_CharacterParts* ULeeControllerComponent_CharacterParts::GetPawnCustomizer() const
{
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		// 생각해보면, 우리는 앞서 HakPawnComponent_CharacterParts를 상속받는 B_MannequinPawnCosmetics를 이미 B_Hero_ShooterMannequin에 추가하였다.
		// B_MannequinPawnCosmetics를 반환되길 기대한다
		return ControlledPawn->FindComponentByClass<ULeePawnComponent_CharacterParts>();
	}
	return nullptr;
}

void ULeeControllerComponent_CharacterParts::AddCharacterPart(const FLeeCharacterPart& NewPart)
{
	AddCharacterPartInternal(NewPart);
}

void ULeeControllerComponent_CharacterParts::AddCharacterPartInternal(const FLeeCharacterPart& NewPart)
{
	FLeeControllerCharacterPartEntry& NewEntry = CharacterParts.AddDefaulted_GetRef();
	NewEntry.Part = NewPart;

	if (ULeePawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		NewEntry.Handle = PawnCustomizer->AddCharacterPart(NewPart);
	}
}

void ULeeControllerComponent_CharacterParts::RemoveAllCharacterParts()
{
	if (ULeePawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		for (FLeeControllerCharacterPartEntry& Entry : CharacterParts)
		{
			PawnCustomizer->RemoveCharacterPart(Entry.Handle);
		}
	}
	CharacterParts.Reset();
}

void ULeeControllerComponent_CharacterParts::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// 이전 OldPawn에 대해서는 Character Parts를 전부 제거
	if (ULeePawnComponent_CharacterParts* OldCustomizer = OldPawn ? OldPawn->FindComponentByClass<ULeePawnComponent_CharacterParts>() : nullptr)
	{
		for (FLeeControllerCharacterPartEntry& Entry : CharacterParts)
		{
			OldCustomizer->RemoveCharacterPart(Entry.Handle);
			Entry.Handle.Reset();
		}
	}

	// 새로운 Pawn에 대해서 기존 Controller가 가지고 있는 Character Parts를 추가
	if (ULeePawnComponent_CharacterParts* NewCustomizer = NewPawn ? NewPawn->FindComponentByClass<ULeePawnComponent_CharacterParts>() : nullptr)
	{
		for (FLeeControllerCharacterPartEntry& Entry : CharacterParts)
		{
			check(!Entry.Handle.IsValid());
			Entry.Handle = NewCustomizer->AddCharacterPart(Entry.Part);
		}
	}
}
