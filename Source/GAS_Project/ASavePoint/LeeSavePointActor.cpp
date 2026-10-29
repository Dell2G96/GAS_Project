// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSavePointActor.h"

#include "LeeSavePointSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS_Project/APhysics/LeeCollisionChannels.h"

// 메시(루트)와 상호작용 감지 구체를 만든다. 구체는 Interaction 채널 쿼리에만 반응한다
ALeeSavePointActor::ALeeSavePointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SavePointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SavePointMesh"));
	SetRootComponent(SavePointMesh);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SavePointMesh);
	InteractionSphere->SetSphereRadius(100.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(Lee_TraceChannel_Interaction, ECR_Overlap);
}

// 상호작용 스캔에 걸리면 휴식 옵션을 제공한다
void ALeeSavePointActor::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

// Subsystem에 등록해 SavePointId로 찾을 수 있게 한다
void ALeeSavePointActor::BeginPlay()
{
	Super::BeginPlay();

	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->RegisterSavePoint(this);
	}
}

// Subsystem에서 등록 해제
void ALeeSavePointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->UnregisterSavePoint(this);
	}

	Super::EndPlay(EndPlayReason);
}
