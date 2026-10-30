// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSavePointActor.h"

#include "LeeSavePointSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS_Project/APhysics/LeeCollisionChannels.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"

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

// 상호작용 스캔에 걸리면 휴식 옵션을 제공한다 (26.10.30 수정 - 점화 전에는 IgniteText 문구 사용)
void ALeeSavePointActor::GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& InteractionBuilder)
{
	FInteractionOption OptionToShow = Option;
	if (!bIsLit && !IgniteText.IsEmpty())
	{
		OptionToShow.Text = IgniteText;
	}
	InteractionBuilder.AddInteractionOption(OptionToShow);
}

// Subsystem에 등록해 SavePointId로 찾을 수 있게 한다 (26.10.30 추가 - 세이브에 점화 기록이 있으면 켠 상태로 시작)
void ALeeSavePointActor::BeginPlay()
{
	Super::BeginPlay();

	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->RegisterSavePoint(this);
	}

	if (const ULeeSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>())
	{
		bIsLit = SaveSubsystem->IsSavePointLit(GetWorld(), SavePointId);
	}
	K2_OnLitStateChanged(bIsLit);
}

// 꺼져 있을 때만 점화 상태로 바꾸고 BP 연출 이벤트를 호출한다
void ALeeSavePointActor::Ignite()
{
	if (bIsLit)
	{
		return;
	}

	bIsLit = true;
	K2_OnLitStateChanged(bIsLit);
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
