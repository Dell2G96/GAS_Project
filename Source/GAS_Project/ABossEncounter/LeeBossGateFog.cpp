// 26.11.12 신규 - 필드 보스 대문의 "첫 도전" 안개 표시

#include "LeeBossGateFog.h"

#include "Components/StaticMeshComponent.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"

// 안개 메시 하나만 가진 표시용 액터를 만든다 (충돌·그림자·복제 없음)
ALeeBossGateFog::ALeeBossGateFog()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	FogMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FogMesh"));
	FogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogMesh->SetCastShadow(false);
	FogMesh->SetCanEverAffectNavigation(false);
	RootComponent = FogMesh;
}

// 이 PC의 세이브에 도전 기록이 있으면 안개를 숨긴다 (기록이 없으면 그대로 보임)
void ALeeBossGateFog::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* GameInstance = GetGameInstance();
	const ULeeSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULeeSaveSubsystem>() : nullptr;
	const bool bFirstAttempt = !SaveSubsystem || !SaveSubsystem->HasAttemptedBoss(BossId);

	SetActorHiddenInGame(!bFirstAttempt);
}
