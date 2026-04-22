// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishInteractionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS_Project/MyTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"

ULeeFinishInteractionComponent::ULeeFinishInteractionComponent()
{
	// 이벤트 기반이므로 틱 불필요
	PrimaryComponentTick.bCanEverTick = false;
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

	// 박스를 동적 생성 (블루프린트가 아닌 C++에서 기본 제공)
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

	BindToOwnerAbilitySystem();
}

void ULeeFinishInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromOwnerAbilitySystem();

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

	// 남아있는 후보 정리 (Player 측 UI가 깨끗이 닫히도록)
	AActor* Owner = GetOwner();
	for (const TWeakObjectPtr<AActor>& WeakPlayer : ActiveExecutionCandidates)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			OnFinishCandidateLeft.Broadcast(Owner, Player, ELeeFinishType::Execution);
		}
	}
	for (const TWeakObjectPtr<AActor>& WeakPlayer : ActiveAssassinationCandidates)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			OnFinishCandidateLeft.Broadcast(Owner, Player, ELeeFinishType::Assassination);
		}
	}

	PlayersInExecutionBox.Reset();
	PlayersInAssassinationBox.Reset();
	ActiveExecutionCandidates.Reset();
	ActiveAssassinationCandidates.Reset();

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

	// 진입 직후 조건 판단. Groggy/Unaware가 아직 아니면 등록하지 않고,
	// 나중에 태그가 붙으면 OnRelevantTagChanged → ReevaluateAllCandidates에서 뒤늦게 브로드캐스트.
	if ((Type == ELeeFinishType::Execution && CanBeExecutedBy(OtherActor)) ||
		(Type == ELeeFinishType::Assassination && CanBeAssassinatedBy(OtherActor)))
	{
		BroadcastEnteredIfNew(OtherActor, Type);
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

	InBoxSet.Remove(OtherActor);
	BroadcastLeftIfActive(OtherActor, Type);
}

void ULeeFinishInteractionComponent::BroadcastEnteredIfNew(AActor* Player, ELeeFinishType Type)
{
	TSet<TWeakObjectPtr<AActor>>& ActiveSet = (Type == ELeeFinishType::Execution)
		? ActiveExecutionCandidates : ActiveAssassinationCandidates;

	if (ActiveSet.Contains(Player))
	{
		return;
	}

	ActiveSet.Add(Player);
	OnFinishCandidateEntered.Broadcast(GetOwner(), Player, Type);
}

void ULeeFinishInteractionComponent::BroadcastLeftIfActive(AActor* Player, ELeeFinishType Type)
{
	TSet<TWeakObjectPtr<AActor>>& ActiveSet = (Type == ELeeFinishType::Execution)
		? ActiveExecutionCandidates : ActiveAssassinationCandidates;

	if (!ActiveSet.Contains(Player))
	{
		return;
	}

	ActiveSet.Remove(Player);
	OnFinishCandidateLeft.Broadcast(GetOwner(), Player, Type);
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
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Groggy);
}

bool ULeeFinishInteractionComponent::CanBeAssassinatedBy(AActor* Player) const
{
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	// Unaware 태그가 직접 부여되어 있으면 통과
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
	{
		return true;
	}

	// 없으면 AI 퍼셉션으로 실제 인지 여부 확인
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !Player)
	{
		return false;
	}

	AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
	UAIPerceptionComponent* PerceptionComp = AIController ? AIController->GetPerceptionComponent() : nullptr;
	if (!PerceptionComp)
	{
		// AI가 없는 더미라면 일단 허용
		return true;
	}

	FActorPerceptionBlueprintInfo PerceptionInfo;
	PerceptionComp->GetActorsPerception(Player, PerceptionInfo);
	for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			return false;
		}
	}
	return true;
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
		return PlayersInExecutionBox.Contains(Player) && CanBeExecutedBy(Player);
	case ELeeFinishType::Assassination:
		return PlayersInAssassinationBox.Contains(Player) && CanBeAssassinatedBy(Player);
	default:
		return false;
	}
}

void ULeeFinishInteractionComponent::BindToOwnerAbilitySystem()
{
	if (bBoundToASC)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	if (!ASC)
	{
		return;
	}

	// 관련 태그 변동 시 재평가 — 리뷰 문서 #5(상태 변화와 오버랩 동기화) 대응
	GroggyTagChangedHandle = ASC->RegisterGameplayTagEvent(
		MyTags::Souls::Status_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULeeFinishInteractionComponent::OnRelevantTagChanged);

	UnawareTagChangedHandle = ASC->RegisterGameplayTagEvent(
		MyTags::Souls::Status_Unaware, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULeeFinishInteractionComponent::OnRelevantTagChanged);

	DeadTagChangedHandle = ASC->RegisterGameplayTagEvent(
		MyTags::Status::Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULeeFinishInteractionComponent::OnRelevantTagChanged);

	ExecutingTagChangedHandle = ASC->RegisterGameplayTagEvent(
		MyTags::Status::Executing, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULeeFinishInteractionComponent::OnRelevantTagChanged);

	AssassinatingTagChangedHandle = ASC->RegisterGameplayTagEvent(
		MyTags::Souls::Status_Assassinating, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULeeFinishInteractionComponent::OnRelevantTagChanged);

	bBoundToASC = true;
}

void ULeeFinishInteractionComponent::UnbindFromOwnerAbilitySystem()
{
	if (!bBoundToASC)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerAbilitySystem())
	{
		ASC->RegisterGameplayTagEvent(MyTags::Souls::Status_Groggy, EGameplayTagEventType::NewOrRemoved)
			.Remove(GroggyTagChangedHandle);
		ASC->RegisterGameplayTagEvent(MyTags::Souls::Status_Unaware, EGameplayTagEventType::NewOrRemoved)
			.Remove(UnawareTagChangedHandle);
		ASC->RegisterGameplayTagEvent(MyTags::Status::Dead, EGameplayTagEventType::NewOrRemoved)
			.Remove(DeadTagChangedHandle);
		ASC->RegisterGameplayTagEvent(MyTags::Status::Executing, EGameplayTagEventType::NewOrRemoved)
			.Remove(ExecutingTagChangedHandle);
		ASC->RegisterGameplayTagEvent(MyTags::Souls::Status_Assassinating, EGameplayTagEventType::NewOrRemoved)
			.Remove(AssassinatingTagChangedHandle);
	}

	bBoundToASC = false;
}

void ULeeFinishInteractionComponent::OnRelevantTagChanged(const FGameplayTag /*Tag*/, int32 /*NewCount*/)
{
	ReevaluateAllCandidates();
}

void ULeeFinishInteractionComponent::ReevaluateAllCandidates()
{
	// Execution: 박스 안에 있지만 아직 유효 후보로 브로드캐스트되지 않았던 Player를 확인
	TArray<TWeakObjectPtr<AActor>> InBoxCopy = PlayersInExecutionBox.Array();
	for (const TWeakObjectPtr<AActor>& Weak : InBoxCopy)
	{
		AActor* Player = Weak.Get();
		if (!Player)
		{
			PlayersInExecutionBox.Remove(Weak);
			continue;
		}

		const bool bCan = CanBeExecutedBy(Player);
		const bool bActive = ActiveExecutionCandidates.Contains(Player);

		if (bCan && !bActive)
		{
			BroadcastEnteredIfNew(Player, ELeeFinishType::Execution);
		}
		else if (!bCan && bActive)
		{
			BroadcastLeftIfActive(Player, ELeeFinishType::Execution);
		}
	}

	// Assassination도 동일
	InBoxCopy = PlayersInAssassinationBox.Array();
	for (const TWeakObjectPtr<AActor>& Weak : InBoxCopy)
	{
		AActor* Player = Weak.Get();
		if (!Player)
		{
			PlayersInAssassinationBox.Remove(Weak);
			continue;
		}

		const bool bCan = CanBeAssassinatedBy(Player);
		const bool bActive = ActiveAssassinationCandidates.Contains(Player);

		if (bCan && !bActive)
		{
			BroadcastEnteredIfNew(Player, ELeeFinishType::Assassination);
		}
		else if (!bCan && bActive)
		{
			BroadcastLeftIfActive(Player, ELeeFinishType::Assassination);
		}
	}
}
