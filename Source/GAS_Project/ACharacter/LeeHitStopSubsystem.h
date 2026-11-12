// 26.11.12 신규 - 싱글 전용 히트스톱

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeHitStopSubsystem.generated.h"

// 타격 한 번의 히트스톱 값 (공격 GA의 AttackData 기본값, 판정 노티파이 HitOverride로 타격별 덮어쓰기)
USTRUCT(BlueprintType)
struct FLeeHitStopParams
{
	GENERATED_BODY()

	// 멈춰 있는 실제 시간(초). 0이면 히트스톱 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Duration = 0.07f;

	// 멈춰 있는 동안의 속도 배율 (0.05 = 5% 속도로 거의 정지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TimeScale = 0.05f;
};

// 타격 순간 공격자와 피격자만 잠깐 느리게 만드는 히트스톱 (CustomTimeDilation 사용)
// 네트워크가 없는 단독 실행(싱글)에서만 동작한다. 멀티에서는 캐릭터 이동 예측이 어긋나므로 적용하지 않는다
UCLASS()
class GAS_PROJECT_API ULeeHitStopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	// 싱글이면 두 액터에 히트스톱을 건다. 이미 멈춰 있으면 끝나는 시간만 늘린다
	// 26.11.12 수정 - 같은 공격자의 연속 타격은 점점 짧게, 공격자는 피격자보다 짧게 멈춘다
	void ApplyHitStop(AActor* Attacker, AActor* Victim, float Duration, float TimeScale);

	// 히트스톱을 쓸 수 있는 월드인지 (단독 실행)
	bool CanUseHitStop() const;

private:
	// 끝날 시간이 지난 액터의 속도를 원래대로 되돌리고, 남은 액터가 있으면 다음 확인을 예약한다
	void RestoreExpired();

	// 모든 액터 속도 복구 (월드 종료 시)
	void RestoreAll();

	// 26.11.12 추가 - 공격자의 연속 타격 수에 따라 이번 히트스톱 시간 배율을 구한다 (콤보 중 렉처럼 느껴지는 것 방지)
	float ConsumeChainScale(AActor* Attacker, float Duration);

	// 멈춘 액터 → 실제 시간 기준 끝나는 시각
	TMap<TWeakObjectPtr<AActor>, double> StoppedActors;

	// 26.11.12 추가 - 공격자별 연속 타격 기록 (마지막 히트스톱 시각, 연속 횟수)
	struct FChainState
	{
		double LastHitTime = 0.0;
		int32 ChainCount = 0;
	};
	TMap<TWeakObjectPtr<AActor>, FChainState> AttackerChains;

	// ── 26.11.12 추가 - 콤보 감쇠 값 ──
	// 이 시간(초) 안에 같은 공격자가 다시 히트스톱을 걸면 콤보로 본다
	static constexpr double ChainWindow = 0.8;
	// 이 시간(초) 안의 히트는 같은 휘두르기(여러 명 동시 타격)로 보고 횟수를 늘리지 않는다
	static constexpr double SameSwingWindow = 0.05;
	// 연속 타격마다 곱하는 배율 (1타 100% → 2타 55% → 3타 30%)
	static constexpr float ChainDecay = 0.55f;
	// 감쇠 하한
	static constexpr float MinChainScale = 0.3f;
	// 이 시간(초) 이상인 히트스톱은 강공격으로 보고 감쇠 없이 그대로 건다 (HitOverride로 막타를 길게 준 경우)
	static constexpr float HeavyHitDuration = 0.1f;
	// 공격자는 피격자보다 짧게 멈춘다 (내 조작이 끊기는 느낌 줄이기)
	static constexpr float AttackerDurationScale = 0.6f;

	FTimerHandle RestoreTimerHandle;
};
