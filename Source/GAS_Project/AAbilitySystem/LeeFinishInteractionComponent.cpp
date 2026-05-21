// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishInteractionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GAS_Project/MyTags.h"
#include "LeeFinishTargetComponent.h"

namespace
{
	bool ContainsWeakActor(const TSet<TWeakObjectPtr<AActor>>& Set, const AActor* Actor)
	{
		for (const TWeakObjectPtr<AActor>& WeakActor : Set)
		{
			if (WeakActor.Get() == Actor)
			{
				return true;
			}
		}

		return false;
	}
}

ULeeFinishInteractionComponent::ULeeFinishInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	bWantsInitializeComponent = true;
}

void ULeeFinishInteractionComponent::InitializeComponent()
{
	Super::InitializeComponent();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USceneComponent* AttachRoot = Owner->GetRootComponent();

	if (!Box_Execution)
	{
		Box_Execution = NewObject<UBoxComponent>(Owner, TEXT("Box_FinishExecution"));
		Box_Execution->SetupAttachment(AttachRoot);
		Box_Execution->SetBoxExtent(ExecutionBoxExtent);
		Box_Execution->SetRelativeLocation(ExecutionBoxOffset);
		Box_Execution->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box_Execution->SetCollisionObjectType(ECC_WorldDynamic);
		Box_Execution->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box_Execution->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Box_Execution->SetGenerateOverlapEvents(true);
		Box_Execution->RegisterComponent();
	}

	if (!Box_Assassination)
	{
		Box_Assassination = NewObject<UBoxComponent>(Owner, TEXT("Box_FinishAssassination"));
		Box_Assassination->SetupAttachment(AttachRoot);
		Box_Assassination->SetBoxExtent(AssassinationBoxExtent);
		Box_Assassination->SetRelativeLocation(AssassinationBoxOffset);
		Box_Assassination->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box_Assassination->SetCollisionObjectType(ECC_WorldDynamic);
		Box_Assassination->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box_Assassination->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Box_Assassination->SetGenerateOverlapEvents(true);
		Box_Assassination->RegisterComponent();
	}
}

void ULeeFinishInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Box_Execution)
	{
		Box_Execution->OnComponentBeginOverlap.AddDynamic(this, &ULeeFinishInteractionComponent::OnExecutionBoxBeginOverlap);
		Box_Execution->OnComponentEndOverlap.AddDynamic(this, &ULeeFinishInteractionComponent::OnExecutionBoxEndOverlap);
	}

	if (Box_Assassination)
	{
		Box_Assassination->OnComponentBeginOverlap.AddDynamic(this, &ULeeFinishInteractionComponent::OnAssassinationBoxBeginOverlap);
		Box_Assassination->OnComponentEndOverlap.AddDynamic(this, &ULeeFinishInteractionComponent::OnAssassinationBoxEndOverlap);
	}
}

void ULeeFinishInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (bDrawDebugBoxes)
	{
		if (Box_Execution)
		{
			DrawDebugBox(GetWorld(), Box_Execution->GetComponentLocation(),
				Box_Execution->GetScaledBoxExtent(), Box_Execution->GetComponentQuat(),
				FColor::Green, false, 0.15f);
		}

		if (Box_Assassination)
		{
			DrawDebugBox(GetWorld(), Box_Assassination->GetComponentLocation(),
				Box_Assassination->GetScaledBoxExtent(), Box_Assassination->GetComponentQuat(),
				FColor::Blue, false, 0.15f);
		}
	}
#endif
}

void ULeeFinishInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Box_Execution)
	{
		Box_Execution->OnComponentBeginOverlap.RemoveDynamic(this, &ULeeFinishInteractionComponent::OnExecutionBoxBeginOverlap);
		Box_Execution->OnComponentEndOverlap.RemoveDynamic(this, &ULeeFinishInteractionComponent::OnExecutionBoxEndOverlap);
	}

	if (Box_Assassination)
	{
		Box_Assassination->OnComponentBeginOverlap.RemoveDynamic(this, &ULeeFinishInteractionComponent::OnAssassinationBoxBeginOverlap);
		Box_Assassination->OnComponentEndOverlap.RemoveDynamic(this, &ULeeFinishInteractionComponent::OnAssassinationBoxEndOverlap);
	}

	for (const TWeakObjectPtr<AActor>& WeakPlayer : PlayersInExecutionBox)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
			{
				TargetComp->UnregisterOverlap(this, ELeeFinishType::Execution);
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& WeakPlayer : PlayersInAssassinationBox)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
			{
				TargetComp->UnregisterOverlap(this, ELeeFinishType::Assassination);
			}
		}
	}

	PlayersInExecutionBox.Reset();
	PlayersInAssassinationBox.Reset();

	Super::EndPlay(EndPlayReason);
}

void ULeeFinishInteractionComponent::OnExecutionBoxBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandleBeginOverlap(OtherActor, ELeeFinishType::Execution);
}

void ULeeFinishInteractionComponent::OnExecutionBoxEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	HandleEndOverlap(OtherActor, ELeeFinishType::Execution);
}

void ULeeFinishInteractionComponent::OnAssassinationBoxBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandleBeginOverlap(OtherActor, ELeeFinishType::Assassination);
}

void ULeeFinishInteractionComponent::OnAssassinationBoxEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	HandleEndOverlap(OtherActor, ELeeFinishType::Assassination);
}

void ULeeFinishInteractionComponent::HandleBeginOverlap(AActor* OtherActor, ELeeFinishType Type)
{
	if (!IsPlayerActor(OtherActor) || OtherActor == GetOwner())
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>>& InBoxSet = (Type == ELeeFinishType::Execution)
		? PlayersInExecutionBox : PlayersInAssassinationBox;
	InBoxSet.Add(OtherActor);

	if (ULeeFinishTargetComponent* TargetComp = OtherActor->FindComponentByClass<ULeeFinishTargetComponent>())
	{
		TargetComp->RegisterOverlap(this, Type);
	}
}

void ULeeFinishInteractionComponent::HandleEndOverlap(AActor* OtherActor, ELeeFinishType Type)
{
	if (!IsPlayerActor(OtherActor))
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>>& InBoxSet = (Type == ELeeFinishType::Execution)
		? PlayersInExecutionBox : PlayersInAssassinationBox;

	for (auto It = InBoxSet.CreateIterator(); It; ++It)
	{
		if (It->Get() == OtherActor)
		{
			It.RemoveCurrent();
		}
	}

	if (ULeeFinishTargetComponent* TargetComp = OtherActor->FindComponentByClass<ULeeFinishTargetComponent>())
	{
		TargetComp->UnregisterOverlap(this, Type);
	}
}

bool ULeeFinishInteractionComponent::IsPlayerActor(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn && Pawn->IsPlayerControlled();
}

UAbilitySystemComponent* ULeeFinishInteractionComponent::GetOwnerAbilitySystem() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

bool ULeeFinishInteractionComponent::CanBeExecutedBy(AActor* /*Player*/) const
{
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Groggy);
}

bool ULeeFinishInteractionComponent::CanBeAssassinatedBy(AActor* /*Player*/) const
{
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware);
}

bool ULeeFinishInteractionComponent::IsCandidateValidFor(AActor* Player, ELeeFinishType Type) const
{
	if (!Player)
	{
		return false;
	}

	switch (Type)
	{
	case ELeeFinishType::Execution:
		return ContainsWeakActor(PlayersInExecutionBox, Player) && CanBeExecutedBy(Player);
	case ELeeFinishType::Assassination:
		return ContainsWeakActor(PlayersInAssassinationBox, Player) && CanBeAssassinatedBy(Player);
	default:
		return false;
	}
}
