

#include "CAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"


ACAIController::ACAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>("PathFollowingComponent"))
{
	if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		UE_LOG(LogTemp, Warning, TEXT("------------- CrowdFollowingComponent is Readdy -------------"));
	}

	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>("EnemySenseConfig_Sight");
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false;
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;
	AISenseConfig_Sight->SightRadius = 5000.f;
	AISenseConfig_Sight->LoseSightRadius = 8000.f;
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 90.f;
	
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	PerceptionComp->ConfigureSense(*AISenseConfig_Sight);
	PerceptionComp->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	PerceptionComp->OnTargetPerceptionUpdated.AddUniqueDynamic(this,&ThisClass::OnEnemyPerceptionUpdated);

	SetGenericTeamId(FGenericTeamId(2));

}

ETeamAttitude::Type ACAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);
	
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(PawnToCheck->GetController());
	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() != GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}
	return ETeamAttitude::Friendly;
}

void ACAIController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed() && Actor)
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, FString::Printf(TEXT("AIController : Name : %s"), *Actor->GetName()));
		}
	}
}

void ACAIController::BeginPlay()
{
	Super::BeginPlay();
	
}


