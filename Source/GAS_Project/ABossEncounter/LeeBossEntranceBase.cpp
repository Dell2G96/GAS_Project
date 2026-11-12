// 26.11.12 신규 - 보스 아레나 입구 공통 베이스

#include "LeeBossEntranceBase.h"

#include "LeeBossEncounterManager.h"
#include "LeeGameplayAbility_PassBossEntrance.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "GAS_Project/APhysics/LeeCollisionChannels.h"
#include "GAS_Project/AInteraction/InteractionQuery.h"

namespace LeeBossEntranceTags
{
	static const FName ShowWhenBlocked(TEXT("ShowWhenBlocked"));
	static const FName ShowWhenOpen(TEXT("ShowWhenOpen"));
}

// 벽(Pawn만 막음), 상호작용 박스, 안쪽 도착 지점을 만든다. 처음엔 꺼진 상태
ALeeBossEntranceBase::ALeeBossEntranceBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BlockerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockerBox"));
	BlockerBox->SetupAttachment(SceneRoot);
	BlockerBox->SetBoxExtent(FVector(20.f, 200.f, 200.f));
	BlockerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(80.f, 200.f, 200.f));
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(Lee_TraceChannel_Interaction, ECR_Overlap);
	// 26.11.12 추가 - F 프롬프트를 고르는 카메라 라인 트레이스(GA_Trigger_Interaction, 프로필 Interactable_OverlapDynamic)는
	// PhysicsBody 채널로 쏜다. 외형 메시가 NoCollision이라 이 박스가 그 트레이스에 걸려야 프롬프트가 뜬다 (화톳불은 메시가 대신 걸림)
	InteractionBox->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InsidePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("InsidePoint"));
	InsidePoint->SetupAttachment(SceneRoot);
	InsidePoint->SetRelativeLocation(FVector(250.f, 0.f, 0.f));

	Option.InteractionAbilityToGrant = ULeeGameplayAbility_PassBossEntrance::StaticClass();
}

// 시작 시 현재 상태(기본 Disabled)를 충돌·BP 연출에 한 번 반영한다
void ALeeBossEntranceBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyEntranceState();
}

// 입구 상태를 복제한다 (클라이언트도 충돌·F 프롬프트를 같은 상태로 맞춤)
void ALeeBossEntranceBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALeeBossEntranceBase, EntranceState);
}

// 막혀 있을 때 바깥쪽 플레이어에게만 F 옵션을 준다 (안쪽에서는 나갈 수 없음)
void ALeeBossEntranceBase::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	if (EntranceState == ELeeBossEntranceState::Blocked && IsOutside(InteractQuery.RequestingAvatar.Get()))
	{
		InteractionBuilder.AddInteractionOption(Option);
	}
}

// [서버] 관리자를 기억하고 하위 클래스가 정한 첫 상태로 켠다
void ALeeBossEntranceBase::ActivateEntrance(ALeeBossEncounterManager* InManager)
{
	Manager = InManager;
	SetEntranceState(GetActivatedState());
}

// [서버] 봉쇄: 막힘 상태로 바꾼다
void ALeeBossEntranceBase::SealEntrance()
{
	SetEntranceState(ELeeBossEntranceState::Blocked);
}

// [서버] 이번 판에 쓰지 않으므로 끈다
void ALeeBossEntranceBase::DisableEntrance()
{
	SetEntranceState(ELeeBossEntranceState::Disabled);
}

// 바깥쪽 플레이어를 안쪽 지점으로 옮기고 입장을 알린다 (늦게 온 동료도 이 방법으로 들어온다)
void ALeeBossEntranceBase::PassThrough(APawn* Pawn)
{
	if (!HasAuthority() || !Pawn || EntranceState != ELeeBossEntranceState::Blocked || !IsOutside(Pawn))
	{
		return;
	}

	const FRotator InsideRotation = InsidePoint->GetComponentRotation();
	Pawn->TeleportTo(InsidePoint->GetComponentLocation(), InsideRotation);
	if (AController* Controller = Pawn->GetController())
	{
		Controller->ClientSetRotation(InsideRotation);
	}

	NotifyPlayerEntered(Pawn);
}

// 관리자에게 입장을 알린다 (관리자가 대기 단계의 첫 입장만 처리)
void ALeeBossEntranceBase::NotifyPlayerEntered(APawn* Pawn)
{
	if (Manager.IsValid())
	{
		Manager->HandlePlayerEntered(this, Pawn);
	}
}

// [서버] 상태를 바꾸고 바로 반영한다
void ALeeBossEntranceBase::SetEntranceState(ELeeBossEntranceState NewState)
{
	if (!HasAuthority())
	{
		return;
	}

	EntranceState = NewState;
	ApplyEntranceState();
}

// [클라이언트] 복제된 상태를 반영한다
void ALeeBossEntranceBase::OnRep_EntranceState()
{
	ApplyEntranceState();
}

// Blocked일 때만 벽·상호작용 충돌을 켠다
void ALeeBossEntranceBase::ApplyEntranceState()
{
	const bool bBlocked = EntranceState == ELeeBossEntranceState::Blocked;
	BlockerBox->SetCollisionEnabled(bBlocked ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	InteractionBox->SetCollisionEnabled(bBlocked ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	ApplyStateVisuals();
	K2_OnEntranceStateChanged(EntranceState);
}

// 26.11.12 추가 - 컴포넌트 태그로 상태별 외형을 켜고 끈다 (BP 그래프 없이 메시·이펙트에 태그만 붙이면 됨)
// ShowWhenBlocked: 막힘일 때만 보임 (안개, 닫힌 문짝) / ShowWhenOpen: 열림일 때만 보임 / 태그 없음: 항상 보임 (문틀)
void ALeeBossEntranceBase::ApplyStateVisuals()
{
	TInlineComponentArray<USceneComponent*> SceneComponents(this);
	for (USceneComponent* Component : SceneComponents)
	{
		if (Component->ComponentHasTag(LeeBossEntranceTags::ShowWhenBlocked))
		{
			Component->SetVisibility(EntranceState == ELeeBossEntranceState::Blocked, /*bPropagateToChildren*/ true);
		}
		else if (Component->ComponentHasTag(LeeBossEntranceTags::ShowWhenOpen))
		{
			Component->SetVisibility(EntranceState == ELeeBossEntranceState::Open, /*bPropagateToChildren*/ true);
		}
	}
}

// 액터 앞(X+)이 안쪽이므로, 앞 방향과 반대편에 있으면 바깥쪽
bool ALeeBossEntranceBase::IsOutside(const AActor* Avatar) const
{
	if (!Avatar)
	{
		return false;
	}

	const FVector ToAvatar = Avatar->GetActorLocation() - GetActorLocation();
	return FVector::DotProduct(ToAvatar, GetActorForwardVector()) < 0.f;
}
