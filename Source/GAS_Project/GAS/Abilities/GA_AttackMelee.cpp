// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_AttackMelee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"


UGA_AttackMelee::UGA_AttackMelee()
{
	return;
}

void UGA_AttackMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!K2_CommitAbility()) return;

	if (!HasAuthority(&ActivationInfo)) return;

	UAbilityTask_PlayMontageAndWait* PlayMontageAndWait = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage);
	PlayMontageAndWait->OnBlendOut.AddDynamic(this, &UGA_AttackMelee::K2_EndAbility);
	PlayMontageAndWait->OnCancelled.AddDynamic(this, &UGA_AttackMelee::K2_EndAbility);
	PlayMontageAndWait->OnCompleted.AddDynamic(this, &UGA_AttackMelee::K2_EndAbility);
	PlayMontageAndWait->OnInterrupted.AddDynamic(this, &UGA_AttackMelee::K2_EndAbility);
	PlayMontageAndWait->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* Task =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetAttackEventTag(), nullptr, false, true);

	Task->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventTaskReceived);
	Task->ReadyForActivation();
	
}

void UGA_AttackMelee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_AttackMelee::OnGameplayEventTaskReceived(FGameplayEventData Payload)
{
	// 블루프린트에서 Break Payload 하던 것 = 여기서 Payload 멤버를 직접 사용
	const AActor* Attacker = Payload.Instigator;
	FGameplayEffectContextHandle ContextHandle = Payload.ContextHandle;
	FGameplayAbilityTargetDataHandle DataHandle = Payload.TargetData;
	
	if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(DataHandle,0))
	{
		const FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(DataHandle,0);
		DesideCombat(Attacker->GetOwner(), HitResult, DamageEffect, Payload);
	}
	

}

FGameplayTag UGA_AttackMelee::GetAttackEventTag()
{
	return MyTags::Abilities::Enemy::Trace;
}


// void ::DesideCombat(AActor* InAttacker, const FHitResult& HitActorToCheck, FGameplayTag EventTag,
// 					FGameplayEventData EventData, TSubclassOf<UGameplayEffect> DamageEffects)
// {
// 	
// 	UAbilitySystemComponent* HitASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActorToCheck.GetActor());
// 	UAbilitySystemComponent* InstASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InAttacker);
// 	
//
// 	bool bIsValidBlock = false; 
// 	const bool bIsCharacterBlocking = UCAbilitySystemStatics::NativeDoseActorHaveTag(HitActorToCheck.GetActor(), MyTags::Status::Guarding);
// 	const bool bIsMyAttackUnBlockalbe = false;
//
// 	if (bIsCharacterBlocking && !bIsMyAttackUnBlockalbe)
// 	{
// 		bIsValidBlock = IsValidBlock(InAttacker, HitActorToCheck.GetActor());
// 	}
// 	//
// 	// FGameplayEffectContextHandle Context = InstASC->MakeEffectContext();
// 	// Context.AddHitResult(HitActorToCheck);
// 	//
// 	//
// 	if (bIsValidBlock)
// 	{
// 		// To Do : HandleSuccessful Block
// 		GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Blue, TEXT("Successful Block!"));
// 		return;
// 	}
//
// 	else
// 	{
// 		if (InstASC && HitASC)
// 		{
// 			// HitResult 생성 및 위치 정보 설정
// 			// FHitResult HitResult;
// 			// HitResult.Location = Cast<AActor>(EventData.Target)->GetActorLocation();
// 			
// 			FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DamageEffects, /*Lvl*/1.f, EventData.ContextHandle);
// 			if (Spec.IsValid())
// 			{
// 				HitASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
// 			}
// 			
// 		}
// 		ACCharacter* HitActor = Cast<ACCharacter>(HitActorToCheck.GetActor());
// 		if (HitActor)
// 		{
// 			HitActor->Multicast_SendGameplayEventToActor(HitActorToCheck.GetActor(), EventTag, EventData);
// 		}
// 		
// 	}
// }