// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeNumberPopComponent_NiagaraText.h"

#include "LeeDamagePopStyleNiagara.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "GAS_Project/LeeLogChannels.h"

ULeeNumberPopComponent_NiagaraText::ULeeNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeNumberPopComponent_NiagaraText::AddNumberPop(const FLeeNumberPopRequest& NewRequest)
{
	int32 LocalDamage = NewRequest.NumberToDisplay;

	if (NewRequest.bIsCriticalDamage)
	{
		LocalDamage *= -1;
	}

	if (!NiagaraComp)
	{
		NiagaraComp = NewObject<UNiagaraComponent>(GetOwner());
		if (Style != nullptr)
		{
			NiagaraComp->SetAsset(Style->TextNiagara);
			NiagaraComp->bAutoActivate = false;
		}

		NiagaraComp->SetupAttachment(nullptr);
		check(NiagaraComp);
		NiagaraComp->RegisterComponent();
	}

	NiagaraComp->Activate(false);
	NiagaraComp->SetWorldLocation(NewRequest.WorldLocation);

	UE_LOG(LogLee, Log, TEXT("DamageHit location : %s"), *(NewRequest.WorldLocation.ToString()));

	TArray<FVector4> DamageList = UNiagaraDataInterfaceArrayFunctionLibrary::GetNiagaraArrayVector4(NiagaraComp, Style->NiagaraArrayName);
	DamageList.Add(FVector4(NewRequest.WorldLocation.X, NewRequest.WorldLocation.Y, NewRequest.WorldLocation.Z, LocalDamage));
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NiagaraComp, Style->NiagaraArrayName, DamageList);
}
