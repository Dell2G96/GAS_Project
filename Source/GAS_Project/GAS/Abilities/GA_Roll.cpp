// // Fill out your copyright notice in the Description page of Project Settings.
//
//
// #include "GA_Roll.h"
//
// #include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
// #include "Abilities/Tasks/AbilityTask_WaitDelay.h"
// #include "GameFramework/CharacterMovementComponent.h"
// #include "GAS_Project/Characters/Player/CPlayerCharacter.h"
//
// UGA_Roll::UGA_Roll()
// {
// 	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
//
// 	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
// }
//
// void UGA_Roll::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
// 	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
// {
// 	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
//
// 	Reset();
// 	ACPlayerCharacter* Character = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
// 	// 아바타 액터를 캐릭터로 캐스팅.
//     
// 	CachedCharacter = Character;
// 	// 캐릭터를 캐시.
//
// 	if (!Character || !RollMontage)
// 	{
// 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
// 		return;
// 	}
//
// 	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
// 	{
// 		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
// 		return;
// 	}
//
// 	const bool bIsLocal = IsLocallyControlled();
//
// 	const bool bIsServer = HasAuthority(&CurrentActivationInfo);
//
// 	if (bIsLocal)
// 	{
// 		const FVector Dir2D = ComputeRollDirection2D_Local(Character);
// 		const float Dist = RollDistance;
//
// 		Character->ApplyRollWarpTarget_Local(Dir2D, Dist);
//
// 		// 서버 확정 + 멀티캐스트
// 		Character->Server_SetRollWarpData(Dir2D, Dist);
//
// 		PlayRollMontage();
// 	}
//
// 	if (bIsServer && !bIsLocal)
// 	{
// 		ServerStartSequence = Character->GetRollWarpSequence();
// 		ServerWaitElapsed = 0.f;
// 		TryPlayServerMontageWhenWarpReady();
// 	}
// }
//
// void UGA_Roll::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
// 	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
// {
// 	// 몽타주가 끝나기 전에 능력이 종료되는 경우도 있으니 정리
// 	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
// 	{
// 		if (ASC->GetCurrentMontage() == RollMontage)
// 		{
// 			ASC->CurrentMontageStop(0.1f);
// 		}
// 	}
//
// 	Reset(); // 중요: 종료 시에도 초기화
// 	CachedCharacter = nullptr;
//
// 	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
// }
//
// FVector UGA_Roll::ComputeRollDirection2D_Local(class ACCharacter* Character) const
// {
// 	FVector Dir = FVector::ZeroVector;
// 	// 기본 방향을 0
//
// 	if (!Character)
// 	{
// 		return FVector::ForwardVector;
// 		// 캐릭터가 없으면 전방으로 대체
// 	}
//
// 	const FVector Pending = Character->GetPendingMovementInputVector();
// 	// 이번 프레임 누적 입력 벡터를 사용
//
// 	if (Pending.SizeSquared() > 0.0001f)
// 	{
// 		Dir = Pending;
// 		// 입력이 있으면 그 방향을 사용
// 	}
// 	else if (Character->GetCharacterMovement() && Character->GetCharacterMovement()->GetCurrentAcceleration().SizeSquared() > 0.0001f)
// 	{
// 		Dir = Character->GetCharacterMovement()->GetCurrentAcceleration();
// 		// 입력이 없으면 현재 가속 방향을 사용
// 	}
// 	else
// 	{
// 		Dir = Character->GetActorForwardVector();
// 		// 둘 다 없으면 전방 벡터로 대체
// 	}
//
// 	Dir.Z = 0.f;
// 	// 2D 방향으로 고정
//
// 	if (Dir.SizeSquared() < 0.0001f)
// 	{
// 		Dir = FVector::ForwardVector;
// 		// 그래도 0이면 월드 전방으로 방어
// 	}
//
// 	Dir.Normalize();
//
// 	return Dir;
// 	// 최종 방향을 반환
// }
//
// void UGA_Roll::PlayRollMontage()
// {
// 	// 중복 재생을 막기
// 	if (bMontageStarted)
// 	{
// 		return;
// 	}
// 	// 시작 플래그를 설정
// 	bMontageStarted = true;
//
// 	// GAS 경유로 몽타주를 재생
// 	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,RollMontage,1.f,NAME_None,false,1.f,0.f);
//
//
// 	// 태스크 생성 실패 시 종료
// 	if (!Task)						
// 	{
// 		bMontageStarted = false;
// 		OnMontageEnded();
// 		return;
// 	}
//
// 	Task->OnCompleted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
// 	Task->OnInterrupted.AddDynamic(this, &UGA_Roll::OnMontageEnded);
// 	Task->OnCancelled.AddDynamic(this, &UGA_Roll::OnMontageEnded);
// 	Task->ReadyForActivation();
// }
//
// void UGA_Roll::TryPlayServerMontageWhenWarpReady()
// {
// 	if (bMontageStarted)
// 	{
// 		return;
// 	}
// 	// 이미 시작했으면 중복 실행을 막기
//
// 	ACPlayerCharacter* Character = CachedCharacter.Get();
// 	// 캐시된 캐릭터를 가져옴
//
// 	if (!Character)
// 	{
// 		OnMontageEnded();
// 		// 캐릭터가 사라졌으면 종료 루틴
//
// 		return;
// 	}
//
// 	const uint8 CurrentSeq = Character->GetRollWarpSequence();
// 	// 현재 시퀀스를 확인
//
// 	if (CurrentSeq != ServerStartSequence)
// 	{
// 		Character->ApplyRollWarpTarget_Local(Character->GetRollWarpDirection(), Character->GetRollWarpDistance());
// 		// 서버에서도 도착한 Dir/Dist로 워프 타깃을 확정 적용
//
// 		PlayRollMontage();
// 		// 서버 몽타주를 시작
//
// 		return;
// 	}
//
// 	if (ServerWaitElapsed >= ServerWaitTimeout)
// 	{
// 		Character->ApplyRollWarpTarget_Local(Character->GetActorForwardVector(), RollDistance);
// 		// 타임아웃이면 전방 기준으로라도 워프 타깃을 세팅.
//
// 		PlayRollMontage();
// 		// 서버 몽타주를 시작.
//
// 		return;
// 	}
//
// 	ServerWaitElapsed += ServerPollInterval;
// 	// 대기 시간을 누적.
//
// 	UAbilityTask_WaitDelay* WaitTask = UAbilityTask_WaitDelay::WaitDelay(this, ServerPollInterval);
// 	// 잠깐 대기 태스크를 만듭니다.
//
// 	if (!WaitTask)
// 	{
// 		Character->ApplyRollWarpTarget_Local(Character->GetActorForwardVector(), RollDistance);
// 		// 태스크 생성 실패 시 전방으로 방어.
//
// 		PlayRollMontage();
// 		// 서버 몽타주를 시작.
//
// 		return;
// 	}
//
// 	WaitTask->OnFinish.AddDynamic(this, &UGA_Roll::TryPlayServerMontageWhenWarpReady);
// 	// 대기 종료 후 다시 체크하도록 바인딩.
//
// 	WaitTask->ReadyForActivation();
// 	// 태스크를 실행.
// }
//
// void UGA_Roll::OnMontageEnded()
// {
// 	bMontageStarted = false;
// 	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
// 	// 어빌리티를 종료
// }
//
// void UGA_Roll::Reset()
// {
// 	bMontageStarted = false;
// 	ServerStartSequence = 0;
// 	ServerWaitElapsed = 0.f;
// }
