// 26.11.12 신규 - 싱글 전용 히트스톱

#include "LeeHitStopSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"

// 게임/PIE 월드에서만 만든다
bool ULeeHitStopSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// 월드가 끝날 때 멈춘 액터가 남지 않게 복구
void ULeeHitStopSubsystem::Deinitialize()
{
	RestoreAll();
	Super::Deinitialize();
}

// 네트워크가 없는 단독 실행일 때만 (리슨 서버·클라이언트는 이동 예측 때문에 제외)
bool ULeeHitStopSubsystem::CanUseHitStop() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() == NM_Standalone;
}

// 두 액터의 CustomTimeDilation을 낮추고, 실제 시간 기준으로 끝나는 시각을 기록한다
// 26.11.12 수정 - 콤보 감쇠 배율을 곱하고, 공격자는 피격자보다 짧게 멈춘다
void ULeeHitStopSubsystem::ApplyHitStop(AActor* Attacker, AActor* Victim, float Duration, float TimeScale)
{
	if (!CanUseHitStop() || Duration <= 0.0f)
	{
		return;
	}

	const float VictimDuration = Duration * ConsumeChainScale(Attacker, Duration);
	const float AttackerDuration = VictimDuration * AttackerDurationScale;
	const double Now = FPlatformTime::Seconds();
	const float ClampedScale = FMath::Clamp(TimeScale, 0.0f, 1.0f);

	// 액터 하나를 TimeScale로 늦추고 끝나는 시각을 기록한다 (이미 멈춰 있으면 더 늦은 시각으로)
	auto StopActor = [this, Now, ClampedScale](AActor* Actor, float StopDuration)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		Actor->CustomTimeDilation = ClampedScale;

		double& StoredEndTime = StoppedActors.FindOrAdd(Actor, 0.0);
		StoredEndTime = FMath::Max(StoredEndTime, Now + StopDuration);
	};
	StopActor(Attacker, AttackerDuration);
	StopActor(Victim, VictimDuration);

	// 멈춘 액터 중 가장 빨리 끝나는 시각에 맞춰 복구를 확인한다 (월드 타이머는 CustomTimeDilation의 영향을 받지 않음)
	double FirstEndTime = TNumericLimits<double>::Max();
	for (const TPair<TWeakObjectPtr<AActor>, double>& Pair : StoppedActors)
	{
		FirstEndTime = FMath::Min(FirstEndTime, Pair.Value);
	}
	if (!StoppedActors.IsEmpty())
	{
		const float FirstDelay = FMath::Max(static_cast<float>(FirstEndTime - Now), 0.001f);
		GetWorld()->GetTimerManager().SetTimer(RestoreTimerHandle, this, &ThisClass::RestoreExpired, FirstDelay, false);
	}
}

// 26.11.12 추가 - 같은 공격자가 ChainWindow 안에 다시 때리면 횟수를 늘리고 배율을 줄인다
// 같은 휘두르기에 여러 명이 맞은 경우는 횟수를 늘리지 않고, 강공격(HeavyHitDuration 이상)은 감쇠 없이 1을 돌려주고 콤보를 새로 시작한다
float ULeeHitStopSubsystem::ConsumeChainScale(AActor* Attacker, float Duration)
{
	if (!IsValid(Attacker))
	{
		return 1.0f;
	}

	const double Now = FPlatformTime::Seconds();
	FChainState& Chain = AttackerChains.FindOrAdd(Attacker);
	const double SinceLastHit = Now - Chain.LastHitTime;

	if (Duration >= HeavyHitDuration || SinceLastHit > ChainWindow)
	{
		Chain.ChainCount = 0;
	}
	else if (SinceLastHit > SameSwingWindow)
	{
		++Chain.ChainCount;
	}
	Chain.LastHitTime = Now;

	return FMath::Max(FMath::Pow(ChainDecay, static_cast<float>(Chain.ChainCount)), MinChainScale);
}

// 끝난 액터는 속도 1로, 아직 남은 액터가 있으면 가장 빨리 끝나는 시각에 맞춰 다시 확인
void ULeeHitStopSubsystem::RestoreExpired()
{
	const double Now = FPlatformTime::Seconds();
	double NextEndTime = TNumericLimits<double>::Max();

	for (auto It = StoppedActors.CreateIterator(); It; ++It)
	{
		AActor* Actor = It.Key().Get();
		if (!Actor || It.Value() <= Now + KINDA_SMALL_NUMBER)
		{
			if (Actor)
			{
				Actor->CustomTimeDilation = 1.0f;
			}
			It.RemoveCurrent();
			continue;
		}
		NextEndTime = FMath::Min(NextEndTime, It.Value());
	}

	if (!StoppedActors.IsEmpty())
	{
		const float Delay = FMath::Max(static_cast<float>(NextEndTime - Now), 0.001f);
		GetWorld()->GetTimerManager().SetTimer(RestoreTimerHandle, this, &ThisClass::RestoreExpired, Delay, false);
	}
}

// 남아 있는 모든 액터의 속도를 1로 되돌린다
void ULeeHitStopSubsystem::RestoreAll()
{
	for (const TPair<TWeakObjectPtr<AActor>, double>& Pair : StoppedActors)
	{
		if (AActor* Actor = Pair.Key.Get())
		{
			Actor->CustomTimeDilation = 1.0f;
		}
	}
	StoppedActors.Empty();
	AttackerChains.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RestoreTimerHandle);
	}
}
