// Fill out your copyright notice in the Description page of Project Settings.


#include "CGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAP_Launched.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

void UCGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == AbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo && !Spec.IsActive())
		{
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void UCGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (HasAuthority(&ActivationInfo))
	{
		if (ActivationPolicy == AbilityActivationPolicy::OnGiven)
		{
			if (ActorInfo)
			{
				ActorInfo->AbilitySystemComponent->ClearAbility(Handle);
			}
		}	
	}
}

class UAnimInstance* UCGameplayAbility::GetOwnerAnimInstance() const
{
	USkeletalMeshComponent* OwnerSkeletalMeshComp = GetOwningComponentFromActorInfo();
	if (OwnerSkeletalMeshComp)
	{
		return OwnerSkeletalMeshComp->GetAnimInstance();
	}
	return nullptr;
}

TArray<FHitResult> UCGameplayAbility::GetHitResultFromSweepLocationTargetData(
	const FGameplayAbilityTargetDataHandle& TargetDataHandle, float SphereSweepRadius,/* ETeamAttitude::Type TargetTeam,*/
	bool bDrawDebug, bool bIgnoreSelf) const
{
	TArray<FHitResult> OutResults;
	TSet<AActor*> HitActors;
	
	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
	
	for (const TSharedPtr<FGameplayAbilityTargetData>& TargetData : TargetDataHandle.Data)
	{
		// 시작 및 끝점 위치
		FVector StartLoc = TargetData->GetOrigin().GetTranslation();
		FVector EndLoc = TargetData->GetEndPoint();
    
		// 추척할 오브젝트 객체
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		
		EDrawDebugTrace::Type DrawDebugTrace = bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
    
		//무시할 오브젝트 객체
		TArray<AActor*> ActorsToIgnore;
		if (bIgnoreSelf)
		{
			ActorsToIgnore.Add(GetAvatarActorFromActorInfo());
		}
    
		TArray<FHitResult> Results;
		UKismetSystemLibrary::SphereTraceMultiForObjects(
		  this,
		  StartLoc,
		  EndLoc,
		  SphereSweepRadius,
		  ObjectTypes,
		  false,
		  ActorsToIgnore,
		  DrawDebugTrace,
		  Results,
		  false);

		for (const FHitResult& Result : Results)
		{
			if (HitActors.Contains(Result.GetActor()))
			{
				continue;
			}

			// if (OwnerTeamInterface)
			// {
			// 	ETeamAttitude::Type OtherActorTeamAttribute = OwnerTeamInterface->GetTeamAttitudeTowards(*Result.GetActor());
			// 	if (OtherActorTeamAttribute != TargetTeam)
			// 	{
			// 		continue;
			// 	}
			// }

			HitActors.Add(Result.GetActor());
			OutResults.Add(Result);
		}
    
	}
  
	return OutResults;
}

FGameplayEffectSpecHandle UCGameplayAbility::MakeEnemyDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass,
	const FScalableFloat& InDamageScalableFloat)
{
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	ContextHandle.SetAbility(this);
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle EffectSpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);

	EffectSpecHandle.Data->SetSetByCallerMagnitude(MyTags::SetByCaller::Projectile, InDamageScalableFloat.GetValueAtLevel(GetAbilityLevel()));

	return EffectSpecHandle;
	
}

void UCGameplayAbility::PushSelf(const FVector& PushVel)
{
	ACharacter* OwningAvatarCharacter = GetOwningAvatarCharacter();
	if (OwningAvatarCharacter)
	{
		OwningAvatarCharacter->LaunchCharacter(PushVel, true, true);
	}
}

void UCGameplayAbility::PushTarget(AActor* Target, const FVector& PushVel)
{
	if (!Target)
	{
		return;
	}
	FGameplayEventData EventData;
	EventData.Instigator = GetAvatarActorFromActorInfo();

	FGameplayAbilityTargetData_SingleTargetHit* HitData = new FGameplayAbilityTargetData_SingleTargetHit();
	FHitResult HitResult;
	HitResult.ImpactNormal = PushVel;
	HitData->HitResult = HitResult;
	EventData.TargetData.Add(HitData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,UGAP_Launched::GetLaunchedAbilityActiationTag(), EventData);
	
}

void UCGameplayAbility::PushTargets(const TArray<AActor*>& Targets, const FVector& PushVel)
{
	for (AActor* Target : Targets)
	{
		PushTarget(Target, PushVel); 
	} 
}

void UCGameplayAbility::PushTargets(const FGameplayAbilityTargetDataHandle& TargetDataHandle, const FVector& PushVel)
{
	TArray<AActor*> Targets  = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);
	PushTargets(Targets, PushVel);
}

void UCGameplayAbility::PlayMontageLocally(UAnimMontage* MontageToPlay)
{
	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();

	if (OwnerAnimInst && !OwnerAnimInst->Montage_IsPlaying(MontageToPlay))
	{
		OwnerAnimInst->Montage_Play(MontageToPlay);
	}
}

void UCGameplayAbility::StopMontageAfterCurrentSection(UAnimMontage* MontageToStop)
{
	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (OwnerAnimInst)
	{
		FName CurrentSectionName = OwnerAnimInst->Montage_GetCurrentSection(MontageToStop);
		OwnerAnimInst->Montage_SetNextSection(CurrentSectionName,NAME_None, MontageToStop);
	}
}

class UCWeaponComponent* UCGameplayAbility::GetWeaponComponent() const
{
	return GetAvatarActorFromActorInfo()->FindComponentByClass<UCWeaponComponent>();
}

FGenericTeamId UCGameplayAbility::GetOwnerTeamID() const
{
	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
	if (OwnerTeamInterface)
	{
		return OwnerTeamInterface->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

ACharacter* UCGameplayAbility::GetOwningAvatarCharacter()
{
	if (!AvatarCharacter)
	{
		AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	}
	return AvatarCharacter;
}

void UCGameplayAbility::ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult,
TSubclassOf<UGameplayEffect> GameplayEffect, int Level)
{
	FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingGameplayEffectSpec(GameplayEffect, Level);

	FGameplayEffectContextHandle EffectContext = MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
	EffectContext.AddHitResult(HitResult);

	EffectSpecHandle.Data->SetContext(EffectContext);
	ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, EffectSpecHandle, UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitResult.GetActor()));
}

void UCGameplayAbility::SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (OwnerASC)
	{
		OwnerASC->HandleGameplayEvent(EventTag, &EventData);
	}
}

bool UCGameplayAbility::IsEnemyActor(AActor* MyActor, AActor* OtherActor) const
{
	if (!MyActor || !OtherActor) return false;
    
	IGenericTeamAgentInterface* MyTeamInterface = Cast<IGenericTeamAgentInterface>(MyActor);
	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
    
	if (!MyTeamInterface || !OtherTeamInterface) return false;
    
	FGenericTeamId MyTeamID = MyTeamInterface->GetGenericTeamId();
	FGenericTeamId OtherTeamID = OtherTeamInterface->GetGenericTeamId();
	
	return MyTeamID.GetId() != OtherTeamID.GetId();
	
	// IGenericTeamAgentInterface* MyTeamInterface = Cast<IGenericTeamAgentInterface>(MyActor);
	// IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	//
	// FGenericTeamId MyTeamID =  MyTeamInterface->GetGenericTeamId();
	// FGenericTeamId OtherTeamID = OtherTeamInterface->GetGenericTeamId();
	//
	// return MyTeamID != OtherTeamID;
}


void UCGameplayAbility::DesideCombat(AActor* InAttacker, const FHitResult& HitActorToCheck,
                                     TSubclassOf<UGameplayEffect> DamageEffects, FGameplayEventData Payload)
{
	
	UAbilitySystemComponent* InstASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* HitASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActorToCheck.GetActor());
	

	bool bIsValidBlock = false; 
	const bool bIsCharacterBlocking = UCAbilitySystemStatics::NativeDoseActorHaveTag(HitActorToCheck.GetActor(), MyTags::Status::Guarding);
	const bool bIsMyAttackUnBlockalbe = false;

	if (bIsCharacterBlocking && !bIsMyAttackUnBlockalbe)
	{
		bIsValidBlock = UCAbilitySystemStatics::IsValidBlock(InAttacker, HitActorToCheck.GetActor());
	}
	
	if (bIsValidBlock)
	{
		// To Do : HandleSuccessful Block
		GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Blue, TEXT("Successful Block!"));
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActorToCheck.GetActor(), MyTags::Events::Block_Hit, Payload);
		return;
	}
	else
	{
		if (InstASC && HitASC)
		{
			// HitResult 생성 및 위치 정보 설정
			// FHitResult HitResult;
			// HitResult.Location = Cast<AActor>(EventData.Target)->GetActorLocation();
			
			FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DamageEffects, /*Lvl*/1.f, Payload.ContextHandle);
			if (Spec.IsValid())
			{
				HitASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
			
		}
		ACCharacter* HitActor = Cast<ACCharacter>(HitActorToCheck.GetActor());
		if (HitActor)
		{
			HitActor->Multicast_SendGameplayEventToActor(HitActorToCheck.GetActor(), HitActorTag, Payload);
		}
	}
	
}

class ACPlayerController* UCGameplayAbility::GetAvatarController() const
{
	if (!GetAvatarActorFromActorInfo())
	{
		return nullptr;
	}
	ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	ACPlayerController* OwnerController = Cast<ACPlayerController>(CurrentActorInfo->PlayerController);
	return OwnerController;
}

const IGenericTeamAgentInterface* UCGameplayAbility::GetTeamAgentFromActor(const AActor* InActor) const
{
	if (!InActor)
	{
		return nullptr;
	}

	// 1) Actor itself
	if (const IGenericTeamAgentInterface* Team = Cast<IGenericTeamAgentInterface>(InActor))
	{
		return Team;
	}

	// 2) Pawn -> Controller
	if (const APawn* Pawn = Cast<APawn>(InActor))
	{
		if (const AController* Ctrl = Pawn->GetController())
		{
			if (const IGenericTeamAgentInterface* Team = Cast<IGenericTeamAgentInterface>(Ctrl))
			{
				return Team;
			}
		}

		// 3) Pawn -> PlayerState (player ASC on PlayerState case)
		if (const APlayerState* PS = Pawn->GetPlayerState())
		{
			if (const IGenericTeamAgentInterface* Team = Cast<IGenericTeamAgentInterface>(PS))
			{
				return Team;
			}
		}
	}

	return nullptr;
}

bool UCGameplayAbility::IsEnemyByTeamId(const AActor* InTarget) const
{
	// [ADDED] TeamId-based enemy-only filtering.
	// Strict rule:
	// - If owner team is known, target MUST have team and must be different.
	// - If owner team unknown, we do not block target lock (keeps system usable).
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !InTarget)
	{
		return false;
	}

	const IGenericTeamAgentInterface* OwnerTeam = GetTeamAgentFromActor(Avatar);
	if (!OwnerTeam)
	{
		return true; // can't judge -> don't block
	}

	const IGenericTeamAgentInterface* TargetTeam = GetTeamAgentFromActor(InTarget);
	if (!TargetTeam)
	{
		return false; // strict enemy-only
	}

	const FGenericTeamId OwnerId = OwnerTeam->GetGenericTeamId();
	const FGenericTeamId TargetId = TargetTeam->GetGenericTeamId();

	// If you use NoTeam, this will reject (strict). Adjust here if you want "NoTeam is hostile".
	if (OwnerId == TargetId)
	{
		return false;
	}

	// If GetTeamAttitudeTowards is implemented, keep it consistent as well.
	const ETeamAttitude::Type Attitude = OwnerTeam->GetTeamAttitudeTowards(*InTarget);
	if (Attitude != ETeamAttitude::Hostile && Attitude != ETeamAttitude::Neutral)
	{
		// Friendly should be rejected.
		return false;
	}

	return true;
}