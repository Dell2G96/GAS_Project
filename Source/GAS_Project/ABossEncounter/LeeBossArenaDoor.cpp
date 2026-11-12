// 26.11.12 신규 - 재도전용 아레나 문

#include "LeeBossArenaDoor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

// 문 안쪽에 Pawn 오버랩 트리거를 둔다
ALeeBossArenaDoor::ALeeBossArenaDoor()
{
	EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
	EntryTrigger->SetupAttachment(SceneRoot);
	EntryTrigger->SetRelativeLocation(FVector(400.f, 0.f, 0.f));
	EntryTrigger->SetBoxExtent(FVector(100.f, 300.f, 200.f));
	EntryTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EntryTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	EntryTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EntryTrigger->SetGenerateOverlapEvents(true);
}

// 서버에서만 입장 트리거를 구독한다
void ALeeBossArenaDoor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		EntryTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleEntryTriggerOverlap);
	}
}

// 열린 상태에서 플레이어가 들어오면 관리자에 알린다 (관리자가 대기 단계의 첫 입장만 처리)
void ALeeBossArenaDoor::HandleEntryTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (EntranceState == ELeeBossEntranceState::Open && Pawn && Pawn->IsPlayerControlled())
	{
		NotifyPlayerEntered(Pawn);
	}
}
