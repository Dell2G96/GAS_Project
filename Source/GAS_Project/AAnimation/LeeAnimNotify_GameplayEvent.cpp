// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeAnimNotify_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

void ULeeAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !EventTag.IsValid())
	{
		return;
	}

	// 26.11.03 수정 - 기본은 서버 전용. bSendOnLocalClient면 직접 조종 중인 클라이언트에서도 발사
	// (로컬 클라이언트는 서버가 복제한 몽타주를 무시하므로, 콤보 전환은 클라이언트 인스턴스가 직접 해야 한다)
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bLocalClient = bSendOnLocalClient && OwnerPawn && OwnerPawn->IsLocallyControlled();
	if (!Owner->HasAuthority() && !bLocalClient)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.EventMagnitude = DamageMultiplier;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}

FString ULeeAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? EventTag.ToString() : TEXT("Lee Gameplay Event");
}
