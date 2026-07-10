// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeCameraMode_LockOn.h"

#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"

ULeeCameraMode_LockOn::ULeeCameraMode_LockOn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FieldOfView = 75.0f;
	BlendTime = 0.2f;
	BlendFunction = ELeeCameraModeBlendFunction::EaseOut;
}

void ULeeCameraMode_LockOn::OnPushed()
{
	// 스택 재진입 시 마지막 프레임 회전으로 스냅되지 않도록, 다음 UpdateView에서 현재
	// PivotRotation을 기준으로 다시 초기화한다 (첫 프레임 급격한 보간 방지)
	bRotationInitialized = false;
}

void ULeeCameraMode_LockOn::UpdateView(float DeltaTime)
{
	AActor* Player = GetTargetActor();
	check(Player);

	if (!bRotationInitialized)
	{
		CurrentRotation = GetPivotRotation();
		bRotationInitialized = true;
	}

	const ULeeTargetLockComponent* Lock = ULeeTargetLockComponent::FindTargetLockComponent(Player);
	AActor* Target = Lock ? Lock->GetLockedTarget() : nullptr;

	const FVector PlayerPivot = GetPivotLocation();

	if (Target)
	{
		const FVector TargetFocus = Lock->GetLockedTargetFocusLocation();
		FRotator Desired = (TargetFocus - PlayerPivot).Rotation();
		Desired.Pitch = FMath::ClampAngle(Desired.Pitch, PitchMin, PitchMax);

		// 타겟 전환/최초 락온 시 즉시 스냅하지 않고 보간 — 카메라가 자연스럽게 새 타겟으로 이동
		CurrentRotation = FMath::RInterpTo(CurrentRotation, Desired, DeltaTime, RotationInterpSpeed);
	}
	// Target이 없으면 (해제 직후 블렌드 아웃 구간) 마지막으로 계산된 CurrentRotation을 그대로 유지

	const FVector DesiredCameraLocation = PlayerPivot + CurrentRotation.RotateVector(CameraOffset);

	// 벽 침투 방지: 플레이어 → 카메라 방향 구체 스윕, 막히면 히트 지점까지 당긴다
	FVector FinalLocation = DesiredCameraLocation;
	if (UWorld* World = Player->GetWorld())
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeLockOnCamera), false, Player);
		if (Target)
		{
			QueryParams.AddIgnoredActor(Target);
		}

		FHitResult Hit;
		if (World->SweepSingleByChannel(Hit, PlayerPivot, DesiredCameraLocation, FQuat::Identity,
			ECC_Camera, FCollisionShape::MakeSphere(CollisionProbeRadius), QueryParams))
		{
			FinalLocation = Hit.Location;
		}
	}

	View.Location = FinalLocation;
	View.Rotation = CurrentRotation;
	View.FieldOfView = FieldOfView;

	// ★ 피니셔 카메라와 정반대: 여기서는 ControlRotation을 구동한다.
	// LeeCameraComponent::GetCameraView()가 매 프레임 이 값을 PC에 강제 적용하므로,
	// 이동 입력 기준(Input_Move)과 캐릭터 DesiredRotation이 자동으로 타겟 방향이 된다.
	View.ControlRotation = CurrentRotation;
}
