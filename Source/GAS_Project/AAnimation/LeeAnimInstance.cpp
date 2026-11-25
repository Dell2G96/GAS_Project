// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"

void ULeeAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		// ASC가 PlayerState에 있어 애님 초기화 시점엔 아직 없을 수 있다.
		// PawnExtComp 델리게이트로 ASC 준비 시(이미 준비됐으면 즉시) Property Map을 초기화한다.
		if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(OwningActor))
		{
			PawnExtComp->OnAbilitySystemInitialized_RegistedAndCall(
				FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
		}
		else
		{
			// UE_LOG(LogTemp, Warning, TEXT("[LeeAnimInstance] PawnExtensionComponent not found on %s"), *GetNameSafe(OwningActor));
		}
	}
}

// ASC 준비 완료 시 호출 — PawnExtComp가 들고 있는 ASC로 직접 Property Map 초기화
// (ASC가 PlayerState에 있어 Actor 인터페이스 조회로는 null일 수 있어 PawnExtComp에서 직접 가져온다)
void ULeeAnimInstance::OnAbilitySystemInitialized()
{
	if (AActor* OwningActor = GetOwningActor())
	{
		if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(OwningActor))
		{
			if (UAbilitySystemComponent* ASC = PawnExtComp->GetLeeAbilitySystemComponent())
			{
				// UE_LOG(LogTemp, Warning, TEXT("[LeeAnimInstance] PropertyMap init OK. AnimInstance=%s ASC=%s"),
				// 	*GetNameSafe(this), *GetNameSafe(ASC));
				InitializeWithAbilitySystem(ASC);
			}
		}
	}
}

void ULeeAnimInstance::InitializeWithAbilitySystem(class UAbilitySystemComponent* ASC)
{
	GameplayTagPropertyMap.Initialize(this, ASC);
}

// 매 프레임 게임 스레드 갱신 — 락온 스탠스·공중 여부를 레이어/상태 머신이 읽을 수 있게 미러링
void ULeeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ULeeTargetLockComponent* LockComp =
		ULeeTargetLockComponent::FindTargetLockComponent(GetOwningActor());
	GuardLeftFootBack = LockComp ? LockComp->IsGuardLeftFootBack() : false;

	// 26.11.26 추가 - 공중 여부 (Thread-safe 함수에서 컴포넌트에 접근할 수 없으므로 여기서 미리 담아 둔다)
	const ACharacter* Character = Cast<ACharacter>(GetOwningActor());
	const UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	bIsFalling = MoveComp ? MoveComp->IsFalling() : false;
}
