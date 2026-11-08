// 26.11.08 신규 - Enemy 공용 반응 가드: 플레이어 근접 공격의 타격 시점 감지

#pragma once

#include "CoreMinimal.h"
#include "LeeGuardThreat.generated.h"

class UAnimMontage;

/**
 * 플레이어 공격 감지 기억 — Lee Combat Context 평가기 인스턴스 데이터 안에 둔다.
 * 판정 구간(ANS_WeaponTrace)마다 확률을 한 번만 굴리기 위해 "이미 굴린 구간"을 기억한다.
 */
USTRUCT()
struct FLeeGuardThreatState
{
	GENERATED_BODY()

	/** 지금 보고 있는 플레이어 공격 몽타주 */
	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> Montage;

	/** 직전 틱 재생 위치 — 값이 뒤로 가면 같은 몽타주를 다시 재생한 것으로 본다 */
	UPROPERTY()
	float LastPosition = 0.f;

	/** 확률을 이미 굴린 판정 노티파이 번호 (Montage->Notifies 인덱스) */
	UPROPERTY()
	int32 RolledNotifyIndex = INDEX_NONE;

	/** 이번 구간: 가드하기로 했는가 */
	UPROPERTY()
	bool bGuardRolled = false;

	/** 이번 구간: 퍼펙트 가드를 노리는가 */
	UPROPERTY()
	bool bPerfectRolled = false;

	/** 이번 구간: 퍼펙트 갱신 신호를 이미 보냈는가 */
	UPROPERTY()
	bool bRefreshIssued = false;

	/** 모든 기억을 지운다 (타깃 없음, 공격 끝, 몽타주 교체 시) */
	void Reset();
};

/** 한 틱의 감지 결과 — 평가기 출력으로 그대로 옮긴다 */
struct FLeeGuardThreatResult
{
	/** 플레이어 근접 공격의 판정 구간이 진행 중이거나 다가오는 중인가 */
	bool bThreatActive = false;

	/** 지금 가드를 올려야 하는가 */
	bool bShouldGuardNow = false;

	/** 이번 틱에 퍼펙트 판정 시간을 다시 열어야 하는가 (구간당 한 번만 참) */
	bool bShouldRefreshPerfect = false;
};

namespace LeeGuardThreat
{
	/** 타깃 플레이어의 공격 몽타주를 읽어 다음 타격까지 남은 시간을 계산하고, 구간마다 가드·퍼펙트 확률을 한 번 굴린다 */
	FLeeGuardThreatResult Update(FLeeGuardThreatState& State, const AActor* Target,
		float GuardChance, float PerfectGuardChance, float PerfectGuardLeadTime);
}
