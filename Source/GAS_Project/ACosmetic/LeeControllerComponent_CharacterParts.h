// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeCharacterPartTypes.h"
#include "Components/ControllerComponent.h"
#include "Components/PawnComponent.h"
#include "UObject/Object.h"

#include "LeeControllerComponent_CharacterParts.generated.h"


/** ControllerComponent가 소유하는 Character Parts */
USTRUCT()
struct FLeeControllerCharacterPartEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FLeeCharacterPart Part;

	//CharacterPart 핸들 
	FLeeCharacterPartHandle Handle;
};

UCLASS(meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeControllerComponent_CharacterParts : public UControllerComponent
{
	GENERATED_BODY()

public:
	ULeeControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	class ULeePawnComponent_CharacterParts* GetPawnCustomizer() const;

	UFUNCTION(BlueprintCallable, Category= Cosmetics)
	void AddCharacterPart(const FLeeCharacterPart& NewPart);

	void AddCharacterPartInternal(const FLeeCharacterPart& NewPart);

	void RemoveAllCharacterParts();

	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UPROPERTY(EditAnywhere, Category= Cosmetics)
	TArray<FLeeControllerCharacterPartEntry> CharacterParts;
};
