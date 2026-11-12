// 26.11.12 신규 - 보스전 진행 단계 관리자

#include "LeeBossEncounterManager.h"

#include "EngineUtils.h"
#include "LeeBossEntranceBase.h"
#include "LeeBossIntro_Roar.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"
#include "GAS_Project/GameModes/LeeBossPartySubsystem.h"

// 복제 액터로 만들고, 기본 인트로는 포효로 둔다
ALeeBossEncounterManager::ALeeBossEncounterManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	Intro = CreateDefaultSubobject<ULeeBossIntro_Roar>(TEXT("Intro"));
}

// 월드에 배치된 관리자 중 첫 번째를 찾는다
ALeeBossEncounterManager* ALeeBossEncounterManager::Find(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ALeeBossEncounterManager> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

// 서버: 입구를 고르고 파티 승패 이벤트를 구독한다
void ALeeBossEncounterManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	SetupEntrance();

	if (ULeeBossPartySubsystem* PartySubsystem = GetWorld()->GetSubsystem<ULeeBossPartySubsystem>())
	{
		BossDefeatedHandle = PartySubsystem->OnBossDefeated.AddUObject(this, &ThisClass::HandleBossDefeated);
		PartyWipedHandle = PartySubsystem->OnPartyWiped.AddUObject(this, &ThisClass::HandlePartyWiped);
	}
}

// 구독 해제와 진행 중인 인트로 정리
void ALeeBossEncounterManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULeeBossPartySubsystem* PartySubsystem = GetWorld() ? GetWorld()->GetSubsystem<ULeeBossPartySubsystem>() : nullptr)
	{
		PartySubsystem->OnBossDefeated.Remove(BossDefeatedHandle);
		PartySubsystem->OnPartyWiped.Remove(PartyWipedHandle);
	}

	if (Intro)
	{
		Intro->CancelIntro();
	}

	Super::EndPlay(EndPlayReason);
}

// 진행 단계를 복제한다
void ALeeBossEncounterManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALeeBossEncounterManager, Phase);
}

// 호스트 세이브에 도전 기록이 없으면 안개벽, 있으면 문을 켠다. 지정이 빠진 쪽은 다른 입구로 대신한다
void ALeeBossEncounterManager::SetupEntrance()
{
	const ULeeSaveSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>() : nullptr;
	bFirstAttempt = !(SaveSubsystem && SaveSubsystem->HasAttemptedBoss(BossId));

	ActiveEntrance = bFirstAttempt ? FirstAttemptEntrance : RetryEntrance;
	ALeeBossEntranceBase* UnusedEntrance = bFirstAttempt ? RetryEntrance : FirstAttemptEntrance;
	if (!ActiveEntrance)
	{
		ActiveEntrance = UnusedEntrance;
		UnusedEntrance = nullptr;
	}

	if (UnusedEntrance)
	{
		UnusedEntrance->DisableEntrance();
	}

	if (ActiveEntrance)
	{
		ActiveEntrance->ActivateEntrance(this);
	}
	else
	{
		UE_LOG(LogLee, Warning, TEXT("[BossEncounter] %s: 입구(안개벽/문)가 지정되지 않았습니다. 입장으로 전투를 시작할 수 없습니다."), *GetName());
	}

	UE_LOG(LogLee, Log, TEXT("[BossEncounter] BossId=%s, 첫 도전=%d, 입구=%s"), *BossId.ToString(), bFirstAttempt, *GetNameSafe(ActiveEntrance));
}

// 대기 중 첫 입장이면: 첫 도전은 기록 후 인트로, 재도전은 바로 전투
void ALeeBossEncounterManager::HandlePlayerEntered(ALeeBossEntranceBase* Entrance, APawn* EnteredPawn)
{
	if (!HasAuthority() || Entrance != ActiveEntrance || Phase != ELeeBossEncounterPhase::Waiting)
	{
		return;
	}

	UE_LOG(LogLee, Log, TEXT("[BossEncounter] %s 입장 → 보스전 시작"), *GetNameSafe(EnteredPawn));

	if (bFirstAttempt)
	{
		RecordForAllPlayers(/*bDefeated*/ false);
	}

	SetPhase(bFirstAttempt && Intro ? ELeeBossEncounterPhase::Intro : ELeeBossEncounterPhase::Combat);
}

// 호스트는 바로 저장, 원격 플레이어는 Client RPC로 각자 세이브에 저장하게 한다 (도전/처치 공용)
void ALeeBossEncounterManager::RecordForAllPlayers(bool bDefeated)
{
	if (BossId.IsNone())
	{
		UE_LOG(LogLee, Warning, TEXT("[BossEncounter] %s: BossId가 비어 있어 보스 기록을 남기지 않습니다."), *GetName());
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ALeePlayerController* PlayerController = Cast<ALeePlayerController>(It->Get());
		if (!PlayerController)
		{
			continue;
		}

		if (PlayerController->IsLocalController())
		{
			if (ULeeSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>())
			{
				bDefeated ? SaveSubsystem->RecordBossDefeated(BossId) : SaveSubsystem->RecordBossAttempt(BossId);
			}
		}
		else if (bDefeated)
		{
			PlayerController->ClientRecordBossDefeated(BossId);
		}
		else
		{
			PlayerController->ClientRecordBossAttempt(BossId);
		}
	}
}

// 단계 전환: 허용된 전환만 받고, 진입 처리 후 구독자에게 알린다
void ALeeBossEncounterManager::SetPhase(ELeeBossEncounterPhase NewPhase)
{
	if (!HasAuthority() || !CanTransition(Phase, NewPhase))
	{
		return;
	}

	UE_LOG(LogLee, Log, TEXT("[BossEncounter] 단계 %s → %s"),
		*UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase));

	Phase = NewPhase;
	EnterPhase(NewPhase);
	BroadcastPhaseChanged();
}

// 대기 → 인트로/전투, 인트로 → 전투, 인트로/전투 → 승리/전멸만 허용
bool ALeeBossEncounterManager::CanTransition(ELeeBossEncounterPhase From, ELeeBossEncounterPhase To)
{
	switch (From)
	{
	case ELeeBossEncounterPhase::Waiting:
		return To == ELeeBossEncounterPhase::Intro || To == ELeeBossEncounterPhase::Combat;
	case ELeeBossEncounterPhase::Intro:
		return To == ELeeBossEncounterPhase::Combat || To == ELeeBossEncounterPhase::Victory || To == ELeeBossEncounterPhase::Wipe;
	case ELeeBossEncounterPhase::Combat:
		return To == ELeeBossEncounterPhase::Victory || To == ELeeBossEncounterPhase::Wipe;
	default:
		return false;
	}
}

// 단계별 진입 처리 (서버). 맵 이동은 기존 ULeeBossPartySubsystem이 맡는다
void ALeeBossEncounterManager::EnterPhase(ELeeBossEncounterPhase NewPhase)
{
	switch (NewPhase)
	{
	case ELeeBossEncounterPhase::Intro:
		if (ActiveEntrance)
		{
			ActiveEntrance->SealEntrance();
		}
		Intro->StartIntro(FindBossActor(), FSimpleDelegate::CreateUObject(this, &ThisClass::HandleIntroFinished));
		break;

	case ELeeBossEncounterPhase::Combat:
		if (ActiveEntrance)
		{
			ActiveEntrance->SealEntrance();
		}
		break;

	case ELeeBossEncounterPhase::Victory:
		if (Intro)
		{
			Intro->CancelIntro();
		}
		RecordForAllPlayers(/*bDefeated*/ true);	// 파티 전원 각자 세이브에 처치 기록
		break;

	case ELeeBossEncounterPhase::Wipe:
		if (Intro)
		{
			Intro->CancelIntro();
		}
		break;

	default:
		break;
	}
}

// 지정한 보스, 비어 있으면 "Boss" 태그가 붙은 액터 (보스는 스폰 지점에서 런타임에 생성되므로 레벨에서 지정할 수 없다)
AActor* ALeeBossEncounterManager::FindBossActor() const
{
	if (BossActor)
	{
		return BossActor;
	}

	TArray<AActor*> BossActors;
	UGameplayStatics::GetAllActorsWithTag(this, BossActorTag, BossActors);
	return BossActors.Num() > 0 ? BossActors[0] : nullptr;
}

// 인트로가 끝나면 전투 시작
void ALeeBossEncounterManager::HandleIntroFinished()
{
	SetPhase(ELeeBossEncounterPhase::Combat);
}

// ULeeBossPartySubsystem 알림: 보스 처치
void ALeeBossEncounterManager::HandleBossDefeated()
{
	SetPhase(ELeeBossEncounterPhase::Victory);
}

// ULeeBossPartySubsystem 알림: 파티 전멸
void ALeeBossEncounterManager::HandlePartyWiped()
{
	SetPhase(ELeeBossEncounterPhase::Wipe);
}

// 클라이언트: 복제된 단계를 구독자에게 알린다
void ALeeBossEncounterManager::OnRep_Phase()
{
	BroadcastPhaseChanged();
}

// C++ 구독자(보스 AI 등)와 BP 이벤트에 단계 변경을 알린다
void ALeeBossEncounterManager::BroadcastPhaseChanged()
{
	OnPhaseChanged.Broadcast(Phase);
	K2_OnPhaseChanged(Phase);
}
