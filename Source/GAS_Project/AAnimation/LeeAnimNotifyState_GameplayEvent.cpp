// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeAnimNotifyState_GameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

// 구간 시작 — BeginEventTag 발사
void ULeeAnimNotifyState_GameplayEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SendEventToOwner(MeshComp, BeginEventTag);
}

// 구간 끝 — EndEventTag 발사
void ULeeAnimNotifyState_GameplayEvent::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SendEventToOwner(MeshComp, EndEventTag);
}

// 에디터 타임라인에 표시될 이름
FString ULeeAnimNotifyState_GameplayEvent::GetNotifyName_Implementation() const
{
	return BeginEventTag.IsValid() ? BeginEventTag.ToString() : TEXT("Lee Gameplay Event Window");
}

// 서버에서만 오너 액터에게 GameplayEvent 발사 — 태그는 직접 만지지 않는다
void ULeeAnimNotifyState_GameplayEvent::SendEventToOwner(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag)
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}
