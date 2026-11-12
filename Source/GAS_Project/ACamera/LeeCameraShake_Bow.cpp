// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeCameraShake_Bow.h"

#include "Shakes/WaveOscillatorCameraShakePattern.h"

// 차지 중 흔들림 — 회전 위주의 잔떨림, Duration 0 = 무한 재생 (어빌리티가 멈춘다)
ULeeCameraShake_BowCharge::ULeeCameraShake_BowCharge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	UWaveOscillatorCameraShakePattern* Pattern = CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("RootShakePattern"));
	Pattern->Duration = 0.0f;
	Pattern->BlendInTime = 0.1f;
	Pattern->BlendOutTime = 0.1f;
	Pattern->Pitch.Amplitude = 0.6f;
	Pattern->Pitch.Frequency = 25.0f;
	Pattern->Yaw.Amplitude = 0.6f;
	Pattern->Yaw.Frequency = 22.0f;
	SetRootShakePattern(Pattern);
}

// 최대 차지 흔들림 — 위치·회전 모두 크게, 0.35초
ULeeCameraShake_BowFullCharge::ULeeCameraShake_BowFullCharge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	UWaveOscillatorCameraShakePattern* Pattern = CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("RootShakePattern"));
	Pattern->Duration = 0.35f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.15f;
	Pattern->X.Amplitude = 4.0f;
	Pattern->X.Frequency = 30.0f;
	Pattern->Z.Amplitude = 3.0f;
	Pattern->Z.Frequency = 28.0f;
	Pattern->Pitch.Amplitude = 3.0f;
	Pattern->Pitch.Frequency = 30.0f;
	Pattern->Yaw.Amplitude = 2.5f;
	Pattern->Yaw.Frequency = 27.0f;
	SetRootShakePattern(Pattern);
}

// 26.11.12 추가 - 퍼펙트 가드 흔들림 — 높은 주파수로 짧게 '탁' 튀는 느낌
ULeeCameraShake_PerfectGuard::ULeeCameraShake_PerfectGuard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	UWaveOscillatorCameraShakePattern* Pattern = CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("RootShakePattern"));
	Pattern->Duration = 0.18f;
	Pattern->BlendInTime = 0.0f;
	Pattern->BlendOutTime = 0.1f;
	Pattern->X.Amplitude = 3.0f;
	Pattern->X.Frequency = 40.0f;
	Pattern->Pitch.Amplitude = 1.5f;
	Pattern->Pitch.Frequency = 35.0f;
	Pattern->Roll.Amplitude = 1.0f;
	Pattern->Roll.Frequency = 30.0f;
	SetRootShakePattern(Pattern);
}
