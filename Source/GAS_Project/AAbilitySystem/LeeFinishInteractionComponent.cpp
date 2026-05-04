// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishInteractionComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "LeeFinishTargetComponent.h"
#include "GAS_Project/MyTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"

ULeeFinishInteractionComponent::ULeeFinishInteractionComponent()
{
	// Debug 시각화를 위해 틱 활성화 (Shipping 빌드에서는 자동 비활성화 가능)
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

	// Phase 4: ASC 태그 구독을 BeginPlay에서 하지 않는다.
	// 플레이어가 박스에 진입할 때 BindToOwnerAbilitySystem을 호출하고,
	// 마지막 플레이어가 박스를 떠날 때 UnbindFromOwnerAbilitySystem을 호출한다.
	// 이로써 박스 밖에 플레이어가 없을 때는 불필요한 태그 이벤트를 수신하지 않는다.
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
	// PlayersInBox 기준으로 정리한다. RemoveCandidate는 후보가 없어도 no-op이므로 안전하다.
	for (const TWeakObjectPtr<AActor>& WeakPlayer : PlayersInExecutionBox)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
			{
				TargetComp->RemoveCandidate(this, ELeeFinishType::Execution);
			}
		}
	}
	for (const TWeakObjectPtr<AActor>& WeakPlayer : PlayersInAssassinationBox)
	{
		if (AActor* Player = WeakPlayer.Get())
		{
			if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
			{
				TargetComp->RemoveCandidate(this, ELeeFinishType::Assassination);
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
	UE_LOG(LogTemp, Verbose, TEXT("[FinishInteraction] HandleBeginOverlap: %s / Type=%d"), *GetNameSafe(OtherActor), (int32)Type);

	if (!IsPlayerActor(OtherActor) || OtherActor == GetOwner())
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>>& InBoxSet = (Type == ELeeFinishType::Execution)
		? PlayersInExecutionBox : PlayersInAssassinationBox;

	const bool bWasEmpty = PlayersInExecutionBox.IsEmpty() && PlayersInAssassinationBox.IsEmpty();
	InBoxSet.Add(OtherActor);

	// Phase 4: 첫 플레이어 진입 시 ASC 태그 구독 시작
	if (bWasEmpty)
	{
		BindToOwnerAbilitySystem();
	}

	// [Bug 1 수정] 인라인 CanBeExecutedBy 체크 대신 ReevaluateAllCandidates를 직접 호출한다.
	//
	// 기존 인라인 체크의 문제:
	//   Groggy 태그가 이미 붙어있는 상태에서 CanBeExecutedBy()가 일시적으로 false를 반환하면
	//   (예: GE 커밋 타이밍, Dead 태그 순간 중복 등) AddCandidate가 호출되지 않는다.
	//   이후 Groggy 태그가 변하지 않으면 OnRelevantTagChanged가 발생하지 않아
	//   ReevaluateAllCandidates가 영원히 호출되지 않고 UI가 나타나지 않는다.
	//
	// ReevaluateAllCandidates 호출로 통일하면:
	//   - 중복 AddCandidate는 TargetComp 내부에서 처리됨
	//   - Bind 이후 즉시 현재 상태를 정확하게 반영함
	ReevaluateAllCandidates();
}

void ULeeFinishInteractionComponent::HandleEndOverlap(AActor* OtherActor, ELeeFinishType Type)
{
	if (!IsPlayerActor(OtherActor))
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>>& InBoxSet = (Type == ELeeFinishType::Execution)
		? PlayersInExecutionBox : PlayersInAssassinationBox;

	// [Bug 2 수정] TSet<TWeakObjectPtr<AActor>>::Remove(AActor*)는 암시적 변환 시
	// TWeakObjectPtr의 내부 SerialNumber 불일치로 Remove가 실패할 수 있다.
	// 람다 기반 RemoveAll로 raw pointer 직접 비교하여 안전하게 제거한다.
	InBoxSet.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& Weak)
	{
		return Weak.Get() == OtherActor;
	});

	BroadcastLeftIfActive(OtherActor, Type);

	// Phase 4: 마지막 플레이어가 박스를 떠나면 ASC 태그 구독 해제
	if (PlayersInExecutionBox.IsEmpty() && PlayersInAssassinationBox.IsEmpty())
	{
		UnbindFromOwnerAbilitySystem();
	}
}

void ULeeFinishInteractionComponent::BroadcastEnteredIfNew(AActor* Player, ELeeFinishType Type)
{
	// ActiveExecutionCandidates 가드 제거: AddCandidate가 내부적으로 중복 체크를 수행하므로
	// 이 함수를 여러 번 호출해도 안전하다. EndOverlap 누락 등으로 가드가 stale 상태가 되면
	// 재진입 시 AddCandidate가 호출되지 않는 버그가 발생했었다.
	if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
	{
		TargetComp->AddCandidate(this, Type);
	}
}

void ULeeFinishInteractionComponent::BroadcastLeftIfActive(AActor* Player, ELeeFinishType Type)
{
	// RemoveCandidate는 후보가 없으면 no-op이므로 가드 없이 직접 호출해도 안전하다.
	if (ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>())
	{
		TargetComp->RemoveCandidate(this, Type);
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
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Groggy); // 레거시 태그 — Souls::Status_Groggy가 정규 태그이며 향후 제거 예정
}

bool ULeeFinishInteractionComponent::CanBeAssassinatedBy(AActor* Player) const
{
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystem();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated] FAIL:  ASC 없음"));
		return false;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		ASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated] FAIL:  Dead/Executing/Assassinating 태그"));
		return false;
	}

	// Unaware 태그가 직접 부여되어 있으면 통과
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated] PASS:  Unaware 태그 있음"));
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
		// Perception이 없으면 암살 불가 — Unaware 태그로만 허용
		UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated] FAIL:  Perception 없음 + Unaware 태그 없음"));
		return false;
	}

	FActorPerceptionBlueprintInfo PerceptionInfo;
	PerceptionComp->GetActorsPerception(Player, PerceptionInfo);
	for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated]    FAIL: AI가 플레이어 인지 중"));
			return false;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[CanBeAssassinated] PASS: AI가 플레이어 미인지"));
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
	// Phase 4: bActive 판단을 ActiveSet(stale 가능) 대신 LeeFinishTargetComponent::HasCandidate로 교체.
	// AddCandidate/RemoveCandidate가 내부에서 중복을 처리하므로 조건이 맞으면 무조건 호출해도 안전하다.

	// Execution: 박스 안에 있지만 아직 유효 후보로 등록되지 않았거나, 더 이상 유효하지 않은 경우 갱신
	TArray<TWeakObjectPtr<AActor>> InBoxCopy = PlayersInExecutionBox.Array();
	for (const TWeakObjectPtr<AActor>& Weak : InBoxCopy)
	{
		AActor* Player = Weak.Get();
		if (!Player)
		{
			// stale TWeakObjectPtr 제거: RemoveAll로 raw pointer null 비교
			PlayersInExecutionBox.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !W.IsValid(); });
			continue;
		}

		const bool bCan = CanBeExecutedBy(Player);
		ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>();
		const bool bActive = TargetComp && TargetComp->HasCandidate(this, ELeeFinishType::Execution);

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
			// stale TWeakObjectPtr 제거: RemoveAll로 raw pointer null 비교
			PlayersInAssassinationBox.RemoveAll([](const TWeakObjectPtr<AActor>& W) { return !W.IsValid(); });
			continue;
		}

		const bool bCan = CanBeAssassinatedBy(Player);
		ULeeFinishTargetComponent* TargetComp = Player->FindComponentByClass<ULeeFinishTargetComponent>();
		const bool bActive = TargetComp && TargetComp->HasCandidate(this, ELeeFinishType::Assassination);

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
