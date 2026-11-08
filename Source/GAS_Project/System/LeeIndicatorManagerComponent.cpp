// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeIndicatorManagerComponent.h"

#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ULeeIndicatorManagerComponent::ULeeIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

ULeeIndicatorManagerComponent* ULeeIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<ULeeIndicatorManagerComponent>();
	}
	return nullptr;
}

void ULeeIndicatorManagerComponent::AddIndicator(class UIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded.Broadcast(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void ULeeIndicatorManagerComponent::RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);
		OnIndicatorRemoved.Broadcast(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}

// 26.11.08 추가 - 적 HP 바 표시 판정 타이머 시작
void ULeeIndicatorManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(NameplateTimer, this, &ThisClass::UpdateNameplateVisibility, NameplateUpdateInterval, true);
	}
}

// 26.11.08 추가 - 타이머 정리
void ULeeIndicatorManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NameplateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

// 26.11.08 추가 - 위젯 클래스 이름이 NameplateWidgetPrefix로 시작하면 적 HP 바로 본다
bool ULeeIndicatorManagerComponent::IsNameplateIndicator(const UIndicatorDescriptor* Indicator) const
{
	return Indicator && Indicator->GetIndicatorClass().GetAssetName().StartsWith(NameplateWidgetPrefix);
}

// 26.11.08 추가 - 적 HP 바: 플레이어 캐릭터 기준 최대 거리 안 + 화면 안 + 벽에 가리지 않음 → 보임
void ULeeIndicatorManagerComponent::UpdateNameplateVisibility()
{
	APlayerController* PC = GetController<APlayerController>();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 거리는 플레이어 캐릭터 기준 (캐릭터가 없으면 카메라 기준)
	const APawn* MyPawn = PC->GetPawn();
	const FVector PlayerLocation = MyPawn ? MyPawn->GetActorLocation() : ViewLocation;

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);

	for (UIndicatorDescriptor* Indicator : Indicators)
	{
		if (!IsNameplateIndicator(Indicator) || !Indicator->GetSceneComponent())
		{
			continue;
		}

		AActor* Target = Indicator->GetSceneComponent()->GetOwner();
		const APawn* TargetPawn = Cast<APawn>(Target);
		if (!Target || (TargetPawn && TargetPawn->IsPlayerControlled()))
		{
			continue;	// 플레이어(파티원) 이름표는 건드리지 않는다
		}

		// 1) 플레이어 캐릭터와의 거리
		bool bVisible = FVector::DistSquared(PlayerLocation, Target->GetActorLocation()) <= FMath::Square(NameplateMaxDistance);

		// 2) 화면 안에 있는가 (머리 높이 지점이 뷰포트 안에 투영되는지)
		const FVector HeadLocation = Target->GetActorLocation() + FVector(0.0f, 0.0f, Target->GetSimpleCollisionHalfHeight() * 0.8f);
		if (bVisible)
		{
			FVector2D ScreenPosition;
			bVisible = PC->ProjectWorldLocationToScreen(HeadLocation, ScreenPosition)
				&& ScreenPosition.X >= 0.0f && ScreenPosition.X <= ViewportX
				&& ScreenPosition.Y >= 0.0f && ScreenPosition.Y <= ViewportY;
		}

		// 3) 벽에 가리지 않는가
		if (bVisible)
		{
			// 벽·지형(WorldStatic)만 검사 — 다른 캐릭터에 가린 것은 숨기지 않는다
			FCollisionQueryParams Params(SCENE_QUERY_STAT(LeeNameplateOcclusion), false);
			Params.AddIgnoredActor(Target);
			Params.AddIgnoredActor(MyPawn);
			bVisible = !GetWorld()->LineTraceTestByObjectType(ViewLocation, HeadLocation, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		}

		Indicator->SetDesiredVisibility(bVisible);
	}
}
