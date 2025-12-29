#include "GA_Roll.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"

UGA_Roll::UGA_Roll()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// InstancedPerActor라 멤버변수 상태가 다음 실행까지 남습니다.

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	// 오너는 즉시 반응, 서버는 확정 후 다른 클라에 전파하는 실행 정책입니다.
}

void UGA_Roll::ResetTransientState()
{
	bMontageStarted = false;
	// 다음 실행에서 PlayRollMontage가 막히지 않게 초기화합니다.

	CachedCharacter = nullptr;
	// 캐시도 초기화합니다.
}

void UGA_Roll::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ResetTransientState();
	// InstancedPerActor라 실행마다 상태를 반드시 초기화합니다.

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	// 부모 활성화 호출입니다.

	ACPlayerCharacter* Character = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	// 어빌리티의 아바타를 플레이어 캐릭터로 캐스팅합니다.

	CachedCharacter = Character;
	// 캐릭터를 캐시합니다.

	if (!Character || !RollMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		// 필수 데이터가 없으면 즉시 종료합니다.
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		// 코스트/쿨다운/태그 조건이 실패하면 활성 상태가 남지 않게 바로 종료합니다.
		return;
	}

	const bool bIsLocal = IsLocallyControlled();
	// 오너 클라(또는 리슨서버의 로컬)인지 확인합니다.

	if (bIsLocal)
	{
		const FVector Dir2D = ComputeRollDirection2D_Local(Character);
		// 로컬에서 입력 기반으로 방향을 계산합니다.

		const float Dist = RollDistance;
		// 설정된 롤 거리입니다.

		Character->ApplyRollWarpTarget_Local(Dir2D, Dist);
		// 로컬에서 즉시 워프 타깃을 잡아 체감 반응성을 확보합니다.

		Character->Server_SetRollWarpData(Dir2D, Dist);
		// 서버에 방향/거리를 보내고, 서버가 멀티캐스트로 모두에게 워프 타깃을 세팅하게 합니다.
	}

	PlayRollMontage();
	// 로컬/서버 모두에서 호출되더라도 bMontageStarted로 중복이 막힙니다.
	// 서버가 몽타주를 재생해야 다른 클라에게도 재생이 전파됩니다.
}

FVector UGA_Roll::ComputeRollDirection2D_Local(ACCharacter* Character) const
{
	if (!Character)
	{
		return FVector::ForwardVector;
		// 캐릭터가 없으면 월드 전방을 반환합니다.
	}

	FVector Dir = FVector::ZeroVector;
	// 방향 기본값입니다.

	const FVector Pending = Character->GetPendingMovementInputVector();
	// 이번 프레임 입력 누적 벡터입니다.

	if (Pending.SizeSquared() > 0.0001f)
	{
		Dir = Pending;
		// 입력이 있으면 입력 방향을 씁니다.
	}
	else if (Character->GetCharacterMovement() &&
		Character->GetCharacterMovement()->GetCurrentAcceleration().SizeSquared() > 0.0001f)
	{
		Dir = Character->GetCharacterMovement()->GetCurrentAcceleration();
		// 입력이 0이면 가속 방향을 씁니다.
	}
	else
	{
		Dir = Character->GetActorForwardVector();
		// 둘 다 없으면 전방 벡터로 대체합니다.
	}

	Dir.Z = 0.f;
	// 수평 2D 방향으로 고정합니다.

	if (Dir.SizeSquared() < 0.0001f)
	{
		Dir = FVector::ForwardVector;
		// 그래도 0이면 월드 전방으로 방어합니다.
	}

	Dir.Normalize();
	// 정규화합니다.

	return Dir;
	// 최종 방향을 반환합니다.
}

void UGA_Roll::PlayRollMontage()
{
	if (bMontageStarted)
	{
		return;
		// 중복 재생을 방지합니다.
	}

	bMontageStarted = true;
	// 몽타주 시작 플래그를 올립니다.

	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			RollMontage,
			1.f,
			NAME_None,
			false,
			1.f,
			0.f);
	// GAS 경유로 몽타주를 재생합니다.

	if (!Task)
	{
		OnMontageEnded();
		// 태스크 생성 실패 시 즉시 종료합니다.
		return;
	}

	Task->OnCompleted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
	// 정상 완료 시 종료합니다.

	Task->OnInterrupted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
	// 인터럽트 시 종료합니다.

	Task->OnCancelled.AddDynamic(this, &UGA_Roll::OnMontageEnded);
	// 취소 시 종료합니다.

	Task->OnBlendOut.AddDynamic(this, &UGA_Roll::OnMontageEnded);
	// 루트모션 몽타주에서 Completed가 안 오는 케이스 대비로 블렌드아웃도 종료로 처리합니다.

	Task->ReadyForActivation();
	// 태스크를 실행합니다.

	const float MontageLen = RollMontage ? RollMontage->GetPlayLength() : 0.f;
	// 몽타주 길이를 가져옵니다.

	if (MontageLen > 0.f)
	{
		UAbilityTask_WaitDelay* Failsafe = UAbilityTask_WaitDelay::WaitDelay(this, MontageLen + FailsafeExtraTime);
		// 몽타주가 끝나지 않는 상황을 대비해 안전 종료 타이머를 겁니다.

		if (Failsafe)
		{
			Failsafe->OnFinish.AddDynamic(this, &UGA_Roll::OnFailsafeTimeout);
			// 시간이 지나도 종료가 안 되면 강제로 EndAbility를 호출합니다.

			Failsafe->ReadyForActivation();
			// 안전 종료 타이머를 실행합니다.
		}
	}
}

void UGA_Roll::OnFailsafeTimeout()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		// 몽타주 이벤트가 오지 않는 경우에도 어빌리티가 영구 활성화되지 않도록 강제 종료합니다.
	}
}

void UGA_Roll::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	// 어빌리티를 종료합니다.
}

void UGA_Roll::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (ASC->GetCurrentMontage() == RollMontage)
		{
			ASC->CurrentMontageStop(0.1f);
			// 종료 시점에 몽타주가 남아있으면 정리합니다.
		}
	}

	ResetTransientState();
	// InstancedPerActor 상태 변수를 확실히 초기화합니다.

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// 부모 종료를 마지막에 호출합니다.
}
