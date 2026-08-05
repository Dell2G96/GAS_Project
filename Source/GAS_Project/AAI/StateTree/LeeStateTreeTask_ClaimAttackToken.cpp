// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_ClaimAttackToken.h"

#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"

// 완료 판정에 참여하지 않고(같은 State의 Ability Task가 결정), Tick 없이 예약/반납만 담당한다
FLeeStateTreeTask_ClaimAttackToken::FLeeStateTreeTask_ClaimAttackToken()
{
	bShouldCallTick = false;

	// 자식 State가 바뀌어도(Approach → Attack) 토큰을 계속 들고 있어야 하므로 재선택 시 Exit/Enter를 받지 않는다.
	// 기본값(true)이면 형제 이동마다 반납 → 재예약이 일어나고 PerAttackerCooldown에 걸려 공격을 못 한다.
	bShouldStateChangeOnReselect = false;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

// 상태 진입 — TokenComponent가 있으면 즉시 1회 Claim 시도, 실패 시 Failed로 AI 재판단 유도
EStateTreeRunStatus FLeeStateTreeTask_ClaimAttackToken::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.TokenComponent)
	{
		// 타겟에 토큰 컴포넌트가 없음 = 제한 없이 동작 (§4-3 정책과 동일)
		RecordUseOnProfile(Data);
		return EStateTreeRunStatus::Running;
	}

	if (!Data.RequesterPawn)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_ClaimAttackToken] RequesterPawn이 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	FLeeAttackClaimHandle NewHandle;
	if (!Data.TokenComponent->TryClaim(Data.RequesterPawn, Data.Reservation.QuotaTag, Data.Reservation.Cost, NewHandle))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.Handle = NewHandle;

	// 이 공격을 "지금 썼다"고 기록해야 Lee Time Since Last Use Consideration이 동작한다
	RecordUseOnProfile(Data);

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 어떤 경로로 끝났든(성공/실패/외부 전이) 발급받은 토큰이 있으면 반드시 반납
void FLeeStateTreeTask_ClaimAttackToken::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.TokenComponent && Data.Handle.IsValid())
	{
		Data.TokenComponent->ReleaseByHandle(Data.Handle);
	}

	Data.Handle = FLeeAttackClaimHandle();
}

// 예약이 성사된 시점을 Enemy의 AttackerProfile에 사용 이력으로 남긴다 (Q8 (b) — 쿨다운 GE 없이 AI 선택 편향만 담당)
void FLeeStateTreeTask_ClaimAttackToken::RecordUseOnProfile(const FInstanceDataType& Data) const
{
	if (!Data.RequesterPawn || !Data.Reservation.QuotaTag.IsValid())
	{
		return;
	}

	if (ULeeAttackerProfileComponent* Profile = ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.RequesterPawn))
	{
		Profile->RecordUse(Data.Reservation.QuotaTag);
	}
}
