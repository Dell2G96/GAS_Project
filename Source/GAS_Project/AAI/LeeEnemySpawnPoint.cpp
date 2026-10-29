// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEnemySpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"

// PlayerStart와 같은 크기의 표시용 캡슐(충돌 없음, 게임 중 숨김)과 방향 화살표를 만든다
ALeeEnemySpawnPoint::ALeeEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(40.f, 92.f);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComponent->SetHiddenInGame(true);
	SetRootComponent(CapsuleComponent);

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(CapsuleComponent);
		ArrowComponent->ArrowColor = FColor::Red;
	}
#endif
}
