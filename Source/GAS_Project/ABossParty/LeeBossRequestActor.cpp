// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossRequestActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS_Project/APhysics/LeeCollisionChannels.h"

// 메시(루트)와 상호작용 감지 구체를 만든다. 구체는 Interaction 채널 쿼리에만 반응한다
ALeeBossRequestActor::ALeeBossRequestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RequestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RequestMesh"));
	SetRootComponent(RequestMesh);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RequestMesh);
	InteractionSphere->SetSphereRadius(100.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(Lee_TraceChannel_Interaction, ECR_Overlap);
}

// 상호작용 스캔에 걸리면 "도움 요청" 옵션을 제공한다
void ALeeBossRequestActor::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}
