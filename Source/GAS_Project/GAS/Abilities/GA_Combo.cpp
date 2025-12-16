

#include "GA_Combo.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagsManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"
#include "Kismet/GameplayStatics.h"

UGA_Combo::UGA_Combo()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	SetAssetTags(AssetTags);
	// 자기 자신 블럭(선택)
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
}

void UGA_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility(); 
		return;
	}

	// 1) 캐시 셋업
	CachedOwnerCharacter = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (CachedOwnerCharacter)
	{
		CachedWeaponComp = CachedOwnerCharacter->FindComponentByClass<UCWeaponComponent>();
		if (CachedWeaponComp)
		{
			CachedWeapon = CachedWeaponComp->GetCharacterCurrentEquippedWeapon();
			if (CachedWeapon)
			{
				CachedWeaponMesh = CachedWeapon->GetWeaponMesh();
			}
		}
	}

	// 무기나 메쉬가 없으면 바로 종료
	if (!CachedWeaponMesh)
	{
		K2_EndAbility();
		return;
	}

	AlreadyHitActors.Empty();
	
	// 팩토리 패턴

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayComboMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontage);
		PlayComboMontageTask->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitComboChangeEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboChangeEventTag(), nullptr, false, false);
		WaitComboChangeEventTask->EventReceived.AddDynamic(this, &UGA_Combo::ComboChangedEventRecevied);
		WaitComboChangeEventTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* HitScanStartTask  = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MyTags::Events::Trace::TraceStart, nullptr, false, false);
		HitScanStartTask->EventReceived.AddDynamic(this,&UGA_Combo::OnHitScanStartEvent);
		HitScanStartTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* HitScanEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MyTags::Events::Trace::TraceEnd, nullptr, false, false);
		HitScanEndTask->EventReceived.AddDynamic(this, &UGA_Combo::OnHitScanEndEvent);
		HitScanEndTask->ReadyForActivation();
	}

	if (K2_HasAuthority())
	{
		UAbilityTask_WaitGameplayEvent* WaitTargetEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,GetComboTargetEventTag());
		WaitTargetEventTask->EventReceived.AddDynamic(this, &UGA_Combo::DoDamageNew);
		WaitTargetEventTask->ReadyForActivation();
	}
	SetupWaitComboInputPress();
}

FGameplayTag UGA_Combo::GetComboChangeEventTag()
{
	return MyTags::Abilities::ComboChange;
	//FGameplayTag::RequestGameplayTag("ability.combo.change");
}

FGameplayTag UGA_Combo::GetComboChangeEventEndTag()
{
	return MyTags::Abilities::ComboChangeEnd;
	//FGameplayTag::RequestGameplayTag("ability.combo.change.end");
}

FGameplayTag UGA_Combo::GetComboTargetEventTag()
{
	return MyTags::Abilities::ComboDamage;
	// FGameplayTag::RequestGameplayTag("ability.combo.damage");
}

void UGA_Combo::OnHitScanStartEvent(FGameplayEventData Payload)
{
	HitScanStart();
}

void UGA_Combo::OnHitScanEndEvent(FGameplayEventData Payload)
{
	HitScanEnd();
}

void UGA_Combo::SetupWaitComboInputPress()
{
	UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputPress->OnPress.AddDynamic(this, &UGA_Combo::HandleInputPress);
	WaitInputPress->ReadyForActivation();
}

void UGA_Combo::HandleInputPress(float TimeWaited)
{
	SetupWaitComboInputPress();
	TryCommitCombo();	
}

void UGA_Combo::TryCommitCombo()
{
	if (NextComboName == NAME_None)
	{
		return;
	}
	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (!OwnerAnimInst )
	{
		return;
	}

	OwnerAnimInst->Montage_SetNextSection(OwnerAnimInst->Montage_GetCurrentSection(ComboMontage), NextComboName, ComboMontage);
	
}

TSubclassOf<UGameplayEffect> UGA_Combo::GetDamageEffectForCurrentCombo() const
{
	UAnimInstance* OwenrAnimInst = GetOwnerAnimInstance();
	if (OwenrAnimInst )
	{
		FName CurrentSectionName = OwenrAnimInst->Montage_GetCurrentSection(ComboMontage);
		const TSubclassOf<UGameplayEffect>* FoundEffectPtr = DamageEffectMap.Find(CurrentSectionName);
		if (FoundEffectPtr)
		{
			return* FoundEffectPtr;
		}
	}
	return DefaultDamageEffect;
}

void UGA_Combo::ComboChangedEventRecevied(FGameplayEventData Data)
{
	FGameplayTag EventTag = Data.EventTag;

	if (EventTag == GetComboChangeEventEndTag())
	{
		NextComboName = NAME_None;
		return;
	}

	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, TagNames);
	NextComboName = TagNames.Last();
}
void UGA_Combo::DoDamage(FGameplayEventData Data)
{
	int HitResultCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(Data.TargetData);

	for (int i = 0; i < HitResultCount; i++)
	{
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data.TargetData, i);
		TSubclassOf<UGameplayEffect> GameplayEffect = GetDamageEffectForCurrentCombo();
		ApplyGameplayEffectToHitResultActor(HitResult, GameplayEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
	}
}



void UGA_Combo::DoDamageNew(FGameplayEventData Data)
{
	const IAbilitySystemInterface* InstASI = Cast<IAbilitySystemInterface>(Data.Instigator);
	const IAbilitySystemInterface* TgtASI  = Cast<IAbilitySystemInterface>(Data.Target);
	if (InstASI && TgtASI)
	{
		// HitResult 생성 및 위치 정보 설정
		FHitResult HitResult;
		HitResult.Location = Cast<AActor>(Data.Target)->GetActorLocation();
		HitResult.ImpactPoint = HitResult.Location;
		
		UAbilitySystemComponent* InstASC = InstASI->GetAbilitySystemComponent();
		UAbilitySystemComponent* TgtASC  = TgtASI->GetAbilitySystemComponent();
		
		if (InstASC && TgtASC)
		{
			FGameplayEffectContextHandle ContextHandle = InstASC->MakeEffectContext();
			ContextHandle.AddInstigator(CachedOwnerCharacter, CachedOwnerCharacter);
			ContextHandle.AddHitResult(HitResult);
			FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DefaultDamageEffect, /*Lvl*/1.f, ContextHandle);
			if (Spec.IsValid())
			{
				TgtASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}


// void UGA_Combo::SendHitReactEventToActors(const TArray<class AActor*>& HitActors)
// {
// 	for (AActor* HitActor : HitActors)
// 	{
// 		FGameplayEventData Payload;
// 		Payload.Instigator = GetAvatarActorFromActorInfo();
// 		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, MyTags::Events::Hit::LightHit,Payload);
// 	}
// }

TArray<class AActor*> UGA_Combo::HitBoxTrace()
{ TArray<AActor*> OutActors;


	
	UWorld* World = GetWorld();
	if (!World || !CachedWeaponMesh || !CachedOwnerCharacter)
	{
		return OutActors;
	}

	AActor* AvatarActor = CachedOwnerCharacter;

	// 소켓 체크
	if (!CachedWeaponMesh->DoesSocketExist(StartSocket) ||
		!CachedWeaponMesh->DoesSocketExist(EndSocket))
	{
		return OutActors;
	}

	const FVector Start = CachedWeaponMesh->GetSocketLocation(StartSocket);
	const FVector End   = CachedWeaponMesh->GetSocketLocation(EndSocket);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComboHitBoxTrace), false, AvatarActor);
	QueryParams.AddIgnoredActor(AvatarActor);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	const FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	TArray<FHitResult> HitResults;

	const bool bHit = World->SweepMultiByChannel(HitResults,Start,End,FQuat::Identity,ECC_Visibility,Sphere,QueryParams,ResponseParams);
	
	if (!bHit) return OutActors;
	

	OutActors.Reserve(HitResults.Num());

	for (const FHitResult& Result : HitResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!IsValid(HitActor) || HitActor == AvatarActor)
		{
			continue;
		}
	
		OutActors.AddUnique(HitActor);
		//UE_LOG(LogTemp,Warning,TEXT("HitActor : %s"),*HitActor->GetName());
	}

	if (bShouldDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Start, HitBoxRadius, 12, FColor::Green, false, 1.0f);
		DrawDebugSphere(GetWorld(), End,   HitBoxRadius, 12, FColor::Green, false, 1.0f);
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f, 0, 10.f);

		for (const FHitResult& Result : HitResults)
		{
			if (IsValid(Result.GetActor()))
			{
				// 표면 충돌 지점
				FVector HitPoint = Result.ImpactPoint;

				// 혹시 ImpactPoint 가 비어있는 경우를 대비해 Location fallback
				if (HitPoint.IsNearlyZero())
				{
					HitPoint = Result.Location;
				}
				DrawDebugSphere(GetWorld(),HitPoint,HitBoxRadius,10,FColor::Red,false,1.f);
			}
		}
	}
	return OutActors;
}

void UGA_Combo::HitScanStart()
{
	UWorld* World = GetWorld();
	if (!World) return;

	AlreadyHitActors.Empty(); // 새 공격 창 시작

	if (World->GetTimerManager().IsTimerActive(HitBoxTraceTimerHandle))
	{
		World->GetTimerManager().ClearTimer(HitBoxTraceTimerHandle);
	}

	const float TraceInterval = 0.016f; // 프레임 레벨

	World->GetTimerManager().SetTimer(HitBoxTraceTimerHandle,this,&UGA_Combo::HitScanTick,TraceInterval,true);
}

void UGA_Combo::HitScanEnd()
{
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(HitBoxTraceTimerHandle);
}

void UGA_Combo::HitScanTick()
{
	TArray<AActor*> HitActors = HitBoxTrace();

	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(CachedOwnerCharacter);

	for (AActor* HitActor : HitActors)
	{
		if (!IsValid(HitActor)) continue;
		
		if (AlreadyHitActors.Contains(HitActor)) continue;

		AlreadyHitActors.Add(HitActor);

		if (OwnerTeamInterface)
		{
			if (OwnerTeamInterface->GetTeamAttitudeTowards(*HitActor) != TargetTeam)
			{
				UE_LOG(LogTemp,Warning,TEXT("%s : is My Team"),*HitActor->GetName());
				continue;
			}
		}
		
		if (K2_HasAuthority())
		{
			// 내(공격자) 가져오기
			FGameplayEventData Payload;
			Payload.Instigator = CachedOwnerCharacter;
			Payload.Target = HitActor;

		
			if (CachedOwnerCharacter)
			{
				CachedOwnerCharacter->Multicast_SendGameplayEventToActor(HitActor, MyTags::Events::Hit::LightHit, Payload);
				DoDamageNew(Payload);
			}
		}
		else
		{
			FGameplayEventData Payload;
			Payload.Instigator = CachedOwnerCharacter;
			Payload.Target = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, MyTags::Events::Hit::LightHit,Payload);
		}
	}
}

void UGA_Combo::SendEventsToActors(class AActor* Owner, const TArray<FHitResult>& Hits) const
{
	for (const FHitResult& Hit : Hits)
	{
		ACPlayerCharacter* PlayerCharacter = Cast<ACPlayerCharacter>(Hit.GetActor());
		if (!IsValid(PlayerCharacter)) continue;
		if (!PlayerCharacter->IsAlive()) continue;
		UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
		if (!IsValid(ASC)) continue;

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddHitResult(Hit);

		FGameplayEventData Payload;
		Payload.Target = PlayerCharacter;
		Payload.ContextHandle = ContextHandle;
		Payload.Instigator = Owner;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, MyTags::Events::Player::HitReact, Payload);
	}
}
