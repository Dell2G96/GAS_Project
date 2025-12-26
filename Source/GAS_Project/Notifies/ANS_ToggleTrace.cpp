// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_ToggleTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"

void UANS_ToggleTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
                                   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	FMeleeTraceRuntimeData& Data = RuntimeMap.FindOrAdd(MeshComp);
	OwnerCharacter = Cast<ACCharacter>(MeshComp->GetOwner());

	// Begin 시점에 Prev 초기화
	if (IsHandEnabled(EMeleeTraceHand::Left))
	{
		FVector L;
		if (GetSocketLocationSafe(MeshComp, LeftSocketName, L))
		{
			Data.PrevLeft = L;
			Data.bHasPrevLeft = true;
		}
		Data.HitActorsLeft.Reset();
	}

	if (IsHandEnabled(EMeleeTraceHand::Right))
	{
		FVector R;
		if (GetSocketLocationSafe(MeshComp, RightSocketName, R))
		{
			Data.PrevRight = R;
			Data.bHasPrevRight = true;
		}
		Data.HitActorsRight.Reset();
	}
	
}

void UANS_ToggleTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	// 해당 MeshComp 런타임 데이터 제거(메모리 정리)
	RuntimeMap.Remove(MeshComp);

}

void UANS_ToggleTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;
	if (!ShouldRunTrace(MeshComp)) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	FMeleeTraceRuntimeData* DataPtr = RuntimeMap.Find(MeshComp);
	if (!DataPtr) return;

	// 왼손 트레이스
	if (IsHandEnabled(EMeleeTraceHand::Left))
	{
		DoHandTrace(
			MeshComp,
			OwnerActor,
			LeftSocketName,
			DataPtr->PrevLeft,
			DataPtr->bHasPrevLeft,
			DataPtr->HitActorsLeft,
			true
		);
	}

	// 오른손 트레이스
	if (IsHandEnabled(EMeleeTraceHand::Right))
	{
		DoHandTrace(
			MeshComp,
			OwnerActor,
			RightSocketName,
			DataPtr->PrevRight,
			DataPtr->bHasPrevRight,
			DataPtr->HitActorsRight,
			false
		);
	}
}

bool UANS_ToggleTrace::ShouldRunTrace(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) return false;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return false;

	if (!bServerOnly) return true;

	// 서버 권한에서만 확정 판정 (멀티 권장)
	return OwnerActor->HasAuthority();
}

bool UANS_ToggleTrace::IsHandEnabled(EMeleeTraceHand HandToCheck) const
{
	if (TraceHand == EMeleeTraceHand::Both) return true;
	return TraceHand == HandToCheck;
}

bool UANS_ToggleTrace::GetSocketLocationSafe(USkeletalMeshComponent* MeshComp, const FName& SocketName,
	FVector& OutLocation) const
{
	if (!MeshComp) return false;
	if (SocketName.IsNone()) return false;
	if (!MeshComp->DoesSocketExist(SocketName)) return false;

	OutLocation = MeshComp->GetSocketLocation(SocketName);
	return true;
}

void UANS_ToggleTrace::DoHandTrace(USkeletalMeshComponent* MeshComp, AActor* OwnerActor, const FName& SocketName,
	FVector& InOutPrevPos, bool& InOutHasPrev, TSet<TWeakObjectPtr<AActor>>& InOutHitActorsThisHand, bool bLeftHand)
 {
	
	FVector CurrPos;
	if (!GetSocketLocationSafe(MeshComp, SocketName, CurrPos))
	{
		// 소켓이 없으면 이번 틱은 건너뜀
		InOutHasPrev = false;
		return;
	}

	// Prev가 없으면 현재 위치로 갱신하고 종료
	if (!InOutHasPrev)
	{
		InOutPrevPos = CurrPos;
		InOutHasPrev = true;
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeSocketTrace), false);
	if (bIgnoreOwner)
	{
		Params.AddIgnoredActor(OwnerActor);
	}

	// 같은 액터를 여러 번 맞추지 않도록 이미 히트한 액터는 무시
	for (const TWeakObjectPtr<AActor>& HitActorPtr : InOutHitActorsThisHand)
	{
		if (AActor* HitActor = HitActorPtr.Get())
		{
			Params.AddIgnoredActor(HitActor);
		}
	}

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		InOutPrevPos,
		CurrPos,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius),
		Params
	);

	if (bDrawDebug)
	{
		DrawDebugLine(World, InOutPrevPos, CurrPos, FColor::Cyan, false, 0.05f, 0, 1.0f);
		DrawDebugSphere(World, CurrPos, TraceRadius, 12, bHit ? FColor::Red : FColor::Green, false, 0.05f);
	}

	if (bHit)
	{
		for (const FHitResult& HR : Hits)
		{
			AActor* HitActor = HR.GetActor();
			if (!HitActor) continue;

			// 여기서 “적”인지 판정은 프로젝트 룰에 맞게 커스터마이즈 해야 함
			// 일단 Pawn/Character/CombatActor 등만 처리하고 싶으면 필터 추가 가능

			// 중복 히트 방지 등록
			InOutHitActorsThisHand.Add(HitActor);

			HandleHit(MeshComp, OwnerActor, HR, bLeftHand);
		}
	}

	// 다음 틱을 위해 이전 위치 갱신
	InOutPrevPos = CurrPos;
	InOutHasPrev = true;
}

void UANS_ToggleTrace::HandleHit(USkeletalMeshComponent* MeshComp, AActor* OwnerActor,
	const FHitResult& HitResult, bool bLeftHand) const
{
	UE_LOG(LogTemp,Warning,TEXT("Melee Hit Detected: %s"),*GetNameSafe(HitResult.GetActor()));
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(12345,2.f,FColor::Yellow,FString::Printf(TEXT("Melee Hit Detected: %s hand=%s"),*GetNameSafe(HitResult.GetActor()),bLeftHand ? TEXT("Left") : TEXT("Right")));
	}

	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(OwnerActor);

	
	if (OwnerTeamInterface)
	{
		if (OwnerTeamInterface->GetTeamAttitudeTowards(*HitResult.GetActor()) != TargetTeam)
		{
			UE_LOG(LogTemp,Warning,TEXT("%s : is Not My Team"),*HitResult.GetActor()->GetName());
		}
	}
		
	// FVector ImpactPoint;
	// ImpactPoint = HitResult.ImpactPoint;
			
	FGameplayEffectContextHandle ContextHandle = OwnerCharacter->GetAbilitySystemComponent()->MakeEffectContext();
	if (ContextHandle.IsValid())
	{
		ContextHandle.AddHitResult(HitResult);	
	}

	FGameplayEventData Payload;
	Payload.Instigator = OwnerActor;
	Payload.Target = HitResult.GetActor();
	Payload.ContextHandle = ContextHandle;
		
	if (OwnerActor->HasAuthority())
	{
		// 내(공격자) 가져오기
		OwnerActor = Cast<ACCharacter>(OwnerActor);
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitResult.GetActor(), MyTags::Events::Hit::LightHit,Payload);
		
		DoDamageNew(Payload);
		
		UE_LOG(LogTemp,Warning,TEXT("%s : is My Team"),*HitResult.GetActor()->GetName());
		
		GEngine->AddOnScreenDebugMessage(12345,2.f,FColor::Blue,FString::Printf(TEXT("Melee Hit Detected: %s hand=%s"),*GetNameSafe(HitResult.GetActor()),bLeftHand ? TEXT("Left") : TEXT("Right")));
	}
		
		
		

}
	// else
	// {
	// 	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitResult.GetActor(), MyTags::Events::Hit::LightHit,Payload);
	// }
	
	// 여기서 “탐지/피격 처리”를 프로젝트에 맞게 연결하면 됨.
	// 1) GAS를 쓰면 GameplayEvent(또는 Cue)로 Ability에 전달
	// 2) 아니면 ApplyPointDamage / 인터페이스 호출 등

	// 예시: 로그
	// UE_LOG(LogTemp, Log, TEXT("Melee hit: %s hand=%s"),
	// 	*GetNameSafe(HitResult.GetActor()),
	// 	bLeftHand ? TEXT("Left") : TEXT("Right"));

	// 가장 안전한 기본은 “서버에서만 확정”이므로,
	// bServerOnly=true일 때는 여기 호출이 서버에서만 일어남.

	// GAS 연결을 원하면:
	// - OwnerActor(공격자)의 ASC에서 이벤트를 보내거나
	// - HitResult.GetActor()(피격자)의 ASC에 GE 적용
	//
	// 지금은 프레임워크/프로젝트마다 코드가 달라서,
	// 여기 함수를 너 프로젝트 방식으로 이어붙이는 형태가 맞음.


void UANS_ToggleTrace::DoDamageNew(struct FGameplayEventData Data) const
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
			ContextHandle.AddInstigator(OwnerCharacter, OwnerCharacter);
			ContextHandle.AddHitResult(HitResult);
			FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DefaultDamageEffect, /*Lvl*/1.f, ContextHandle);
			if (Spec.IsValid())
			{
				TgtASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}


//
//
// USkeletalMeshComponent* UANS_ToggleTrace::IsUsingWeaponMesh(USkeletalMeshComponent* MeshComp)
// {
// 	ACCharacter* Ownercharacter =  Cast<ACCharacter>(MeshComp->GetOwner());
// 	// 어떤 메쉬의 소켓을 사용할지 결정
// 	if (IsWeapon)
// 	{
// 		if (UCWeaponComponent* WeaponComp = Ownercharacter->FindComponentByClass<UCWeaponComponent>())
// 		{
// 			if (ACWeapon* Weapon = WeaponComp->GetCharacterCurrentEquippedWeapon())
// 			{
// 				if (USkeletalMeshComponent* WeaponMesh = Weapon->FindComponentByClass<USkeletalMeshComponent>())
// 				{
// 					return WeaponMesh;
// 				}
// 			}
// 		}
// 	}
// 	return MeshComp;
// }
//
// TArray<class AActor*> UANS_ToggleTrace::HitBoxTrace()
// {
// 	
// 	TArray<AActor*> OutActors;
//
// 	USkeletalMeshComponent* UseMeshComp; 
// 	if (IsWeapon)
// 	{
// 		UseMeshComp = IsUsingWeaponMesh(OwnerActor->GetMesh());
// 		
// 	}
// 	
// 	else
// 	{
// 		UseMeshComp = OwnerActor->GetMesh();
// 	}
// 	
// 	UWorld* World = GetWorld();
// 	
// 	if (!World)
// 	{
// 		return OutActors;
// 	}
// 	
// 	// 소켓 체크
// 	if (!UseMeshComp->DoesSocketExist(StartTraceSocket) || !UseMeshComp->DoesSocketExist(EndTraceSocket))
// 	{
// 		return OutActors;
// 	}
//
// 	const FVector Start = UseMeshComp->GetSocketLocation(StartTraceSocket);
// 	const FVector End   = UseMeshComp->GetSocketLocation(EndTraceSocket);
//
// 	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComboHitBoxTrace), false, OwnerActor);
// 	QueryParams.AddIgnoredActor(OwnerActor);
//
// 	FCollisionResponseParams ResponseParams;
// 	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
// 	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
//
// 	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
//
// 	TArray<FHitResult> HitResults;
//
// 	const bool bHit = World->SweepMultiByChannel(HitResults,Start,End,FQuat::Identity,ECC_Visibility,Sphere,QueryParams,ResponseParams);
// 	
// 	if (!bHit) return OutActors;
// 	
// 	OutActors.Reserve(HitResults.Num());
//
// 	for (const FHitResult& Result : HitResults)
// 	{
// 		AActor* HitActor = Result.GetActor();
// 		if (!IsValid(HitActor))
// 		{
// 			continue;
// 		}
// 	
// 		OutActors.AddUnique(HitActor);
// 		//UE_LOG(LogTemp,Warning,TEXT("HitActor : %s"),*HitActor->GetName());
// 	}
//
// 	if (bDebugDrawing)
// 	{
// 		DrawDebugSphere(GetWorld(), Start, Radius, 12, FColor::Green, false, 1.0f);
// 		DrawDebugSphere(GetWorld(), End,   Radius, 12, FColor::Green, false, 1.0f);
// 		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f, 0, 10.f);
//
// 		for (const FHitResult& Result : HitResults)
// 		{
// 			if (IsValid(Result.GetActor()))
// 			{
// 				// 표면 충돌 지점
// 				FVector HitPoint = Result.ImpactPoint;
//
// 				// 혹시 ImpactPoint 가 비어있는 경우를 대비해 Location fallback
// 				if (HitPoint.IsNearlyZero())
// 				{
// 					HitPoint = Result.Location;
// 				}
// 				DrawDebugSphere(GetWorld(),HitPoint,Radius,10,FColor::Red,false,1.f);
// 			}
// 		}
// 	}
// 	return OutActors;
// }
//
