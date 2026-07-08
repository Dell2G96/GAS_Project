// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeCameraMode_Finisher.h"

#include "DrawDebugHelpers.h"
#include "GAS_Project/ACharacter/LeeHeroComponent.h"

ULeeCameraMode_Finisher::ULeeCameraMode_Finisher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FieldOfView = 65.0f;
	BlendTime = 0.25f;
	BlendFunction = ELeeCameraModeBlendFunction::EaseInOut;
}

void ULeeCameraMode_Finisher::OnPushed()
{
	// 카메라 모드 인스턴스는 스택에서 클래스당 1개로 재사용되므로,
	// 시퀀스마다 앵커를 새로 캐시하려면 Push 순간에 리셋해야 한다
	bAnchorCached = false;
}

void ULeeCameraMode_Finisher::UpdateView(float DeltaTime)
{
	AActor* Attacker = GetTargetActor();
	check(Attacker);

	UWorld* World = Attacker->GetWorld();

	if (!bAnchorCached)
	{
		AActor* Victim = nullptr;
		if (const ULeeHeroComponent* Hero = ULeeHeroComponent::FindHeroComponent(Attacker))
		{
			Victim = Hero->GetAbilityCameraFocusTarget();
		}

		const FVector AttackerLocation = Attacker->GetActorLocation();
		const FVector VictimLocation = Victim
			? Victim->GetActorLocation()
			: AttackerLocation + Attacker->GetActorForwardVector() * 150.0f;

		const float AnchorYaw = (VictimLocation - AttackerLocation).GetSafeNormal2D().Rotation().Yaw;
		CachedAnchor = FTransform(FRotator(0.0f, AnchorYaw, 0.0f), (AttackerLocation + VictimLocation) * 0.5f);
		CachedVictim = Victim;
		bAnchorCached = true;
	}

	const FVector DesiredLocation = CachedAnchor.TransformPosition(CameraOffset);
	const FVector FocusLocation = CachedAnchor.TransformPosition(FocusOffset);

	// 벽 침투 방지: 포커스 → 카메라 방향 구체 스윕, 막히면 히트 지점까지 당긴다
	FVector FinalLocation = DesiredLocation;
	if (World)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LeeFinisherCamera), false, Attacker);
		if (AActor* Victim = CachedVictim.Get())
		{
			Params.AddIgnoredActor(Victim);
		}

		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, FocusLocation, DesiredLocation, FQuat::Identity,
			ECC_Camera, FCollisionShape::MakeSphere(CollisionProbeRadius), Params))
		{
			FinalLocation = Hit.Location;
		}
	}

	View.Location = FinalLocation;
	View.Rotation = (FocusLocation - FinalLocation).Rotation();
	View.FieldOfView = FieldOfView;

	// ★ 복귀의 핵심: ControlRotation은 절대 건드리지 않고 플레이어가 보던 방향을 통과시킨다.
	// LeeCameraComponent::GetCameraView()가 매 프레임 View.ControlRotation을 PC에 강제 적용하므로,
	// 시네마틱 방향(View.Rotation)을 넣으면 컨트롤 회전이 오염되어 종료 후 엉뚱한 방향에서 복귀한다.
	View.ControlRotation = GetPivotRotation();

	// 임시 디버그: 앵커 위치 시각화 (튜닝 확인 후 제거할 것)
	if (World)
	{
		DrawDebugSphere(World, CachedAnchor.GetLocation(), 10.0f, 12, FColor::Yellow, false, 0.1f);
	}
}
