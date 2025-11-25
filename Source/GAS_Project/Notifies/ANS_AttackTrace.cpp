// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_AttackTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"


static FORCEINLINE FVector GetSocketWS(USkeletalMeshComponent* Mesh, const FName& Socket)
{
	return Mesh && Mesh->DoesSocketExist(Socket) ? Mesh->GetSocketLocation(Socket) : Mesh->GetComponentLocation();
}


UANS_AttackTrace::FTraceState& UANS_AttackTrace::GetState(USkeletalMeshComponent* MeshComp)
{
	FTraceState& State = StateMap.FindOrAdd(MeshComp);
	return State;
	
}

USkeletalMeshComponent* UANS_AttackTrace::ResolveTraceMesh(USkeletalMeshComponent* DefaultMesh) const
{
	if (!bUseWeaponMeshSockets || !DefaultMesh) return DefaultMesh;

	AActor* Owner = DefaultMesh->GetOwner();
	if (!Owner) return DefaultMesh;

	// 무기 컴포넌트 → 현재 무기 → 무기 메쉬
	if (UCWeaponComponent* WeaponComp = Owner->FindComponentByClass<UCWeaponComponent>())
	{
		if (ACWeapon* Weapon = WeaponComp->GetCharacterCurrentEquippedWeapon())
		{
			if (USkeletalMeshComponent* WeaponMesh = Weapon->FindComponentByClass<USkeletalMeshComponent>())
			{
				return WeaponMesh;
			}
		}
	}

	// 실패 시 캐릭터 메쉬 사용
	return DefaultMesh;
}

void UANS_AttackTrace::DoSweepAndApply(USkeletalMeshComponent* MeshComp, FTraceState& State, const FVector& From,
	const FVector& To, UWorld* World)
{
	if (!World || !MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ANS_AttackTrace), false, Owner);
	FCollisionResponseParams RespParams;
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(Hits, From, To, FQuat::Identity, TraceChannel, Shape, Params, RespParams);

	if (bDrawDebug)
	{
		DrawDebugCapsule(World,
			(From + To) * 0.5f,
			(From - To).Size() * 0.5f,
			Radius,
			FQuat::FindBetweenNormals(FVector::UpVector, (To-From).GetSafeNormal()),
			bHit ? FColor::Red : FColor::Cyan,
			false, 0.06f);
	}

	if (!bHit) return;

	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other || Other == Owner) continue;

		// 한 번만 맞게 옵션
		if (bSingleHitPerActor && State.AlreadyHitActors.Contains(Other)) continue;

		// 적용 권한 체크(기본: 서버만)
		if (ShouldApplyOnThisMachine(Owner))
		{
			SendGasEventOrApplyGE(Owner, Other, Hit);
		}

		State.AlreadyHitActors.Add(Other);
	}
}

 void UANS_AttackTrace::SendGasEventOrApplyGE(AActor* InstigatorActor, AActor* TargetActor,
	const FHitResult& Hit) const
{
	if (!InstigatorActor || !TargetActor) return;

	// 1) GAS 이벤트 전송 (이벤트 태그가 있으면 우선)
	if (HitTagEvent.IsValid())
	{
		FGameplayEventData Ev;
		Ev.EventTag  = HitTagEvent;
		Ev.Instigator = InstigatorActor;
		Ev.Target     = TargetActor;

		// TargetData에 Hit 포함
		FGameplayAbilityTargetDataHandle Handle;
		FGameplayAbilityTargetData_SingleTargetHit* NewData =
			new FGameplayAbilityTargetData_SingleTargetHit(Hit);
		Handle.Add(NewData);
		Ev.TargetData = Handle;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InstigatorActor, HitTagEvent, Ev);
		return;
	}

	// 2) 바로 GE 적용(선택)
	if (DamageEffect)
	{
		UAbilitySystemComponent* InstASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
		UAbilitySystemComponent* TgtASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

		if (InstASC && TgtASC)
		{
			FGameplayEffectContextHandle Ctx = InstASC->MakeEffectContext();
			Ctx.AddHitResult(Hit);
			FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DamageEffect, 1.f, Ctx);
			if (Spec.IsValid())
			{
				TgtASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
			}
		}
	}
	
}

bool UANS_AttackTrace::ShouldApplyOnThisMachine(AActor* Owner) const
{
	if (!Owner) return false;

	if (bServerOnly)
	{
		return Owner->HasAuthority(); // 서버에서만 판정/적용
	}
	// 서버/클라 모두(로컬 예측 포함) 판정
	return true;
}

void UANS_AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	//Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	FTraceState& State = GetState(MeshComp);
	State.Reset();

	// 어떤 메쉬의 소켓을 사용할지 결정
	State.TraceMesh = ResolveTraceMesh(MeshComp);

	if (!State.TraceMesh.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_AttackTrace] TraceMesh invalid at Begin."));
		return;
	}

	// 소켓 존재 여부 사전 체크
	if (!State.TraceMesh->DoesSocketExist(StartSocket) || !State.TraceMesh->DoesSocketExist(EndSocket))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ANS_AttackTrace] Socket missing. Start=%s End=%s Mesh=%s"),
			*StartSocket.ToString(), *EndSocket.ToString(),
			*GetNameSafe(State.TraceMesh.Get()));
		return;
	}

	State.PreStart = State.TraceMesh->GetSocketLocation(StartSocket);
	State.PreEnd   = State.TraceMesh->GetSocketLocation(EndSocket);
	State.bInitaillzed = true;

	if (bDrawDebug)
	{
		if (UWorld* World = MeshComp->GetWorld())
		{
			DrawDebugSphere(World, State.PreStart, Radius, 12, FColor::Green, false, 1.0f);
			DrawDebugSphere(World, State.PreEnd,   Radius, 12, FColor::Green, false, 1.0f);
			DrawDebugLine(World, State.PreStart, State.PreEnd, FColor::Green, false, 1.0f, 0, 1.5f);
		}
	}
	
}

void UANS_AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	//Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;
	// 실행이 끝났으면 상태 정리
	if (FTraceState* State = StateMap.Find(MeshComp))
	{
		State->Reset();
	}
}

void UANS_AttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	//Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	FTraceState* StatePtr = StateMap.Find(MeshComp);
	if (!StatePtr) return;
	
	FTraceState& State = *StatePtr;

	if (!State.bInitaillzed || !State.TraceMesh.IsValid())
		return;

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	// 현재 프레임 소켓 위치
	const FVector CurStart = State.TraceMesh->GetSocketLocation(StartSocket);
	const FVector CurEnd   = State.TraceMesh->GetSocketLocation(EndSocket);
	
	// 1) 양 끝 이동 경로 보정(빠른 스윙 중 프레임 사이 누락 방지)
	DoSweepAndApply(MeshComp, State, State.PreStart, CurStart, World);
	DoSweepAndApply(MeshComp, State, State.PreEnd,   CurEnd,   World);

	// 2) 현재 칼날 라인 자체 스윕(칼의 두께/넓이 커버)
	DoSweepAndApply(MeshComp, State, CurStart, CurEnd, World);

	// 다음 프레임 대비
	State.PreStart = CurStart;
	State.PreEnd   = CurEnd;

	if (bDrawDebug)
	{
		DrawDebugLine(World, CurStart, CurEnd, FColor::Yellow, false, 0.05f, 0, 1.0f);
	}
	
	// AActor* Owner = MeshComp->GetOwner();
	// if (!ShouldRunOnThisMachine(Owner)) return;
	//
	// FTraceState* S = States.Find(MeshComp);
	// if (!S || !S->bInitaillzed) return;
	//
	// // 트레이스
	// DoTrace(MeshComp, FrameDeltaTime);
	//
	// // 프레임 업데이트
	// S->PreStart = GetSocketWS(MeshComp, StartSocket);
	// S->PreEnd   = GetSocketWS(MeshComp, EndSocket);
}

void UANS_AttackTrace::FTraceState::Reset()
{
	TraceMesh.Reset();
	PreStart = PreEnd = FVector::ZeroVector;
	AlreadyHitActors.Empty();
	bInitaillzed = false;		}

void UANS_AttackTrace::DoTrace(USkeletalMeshComponent* MeshComp, float DeltaTime) const
{
	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	FTraceState* State = const_cast<FTraceState*>(States.Find(MeshComp));
	if (!State) return;

	const FVector CurrStart = GetSocketWS(MeshComp, StartSocket);
	const FVector CurrEnd   = GetSocketWS(MeshComp, EndSocket);

	AActor* Owner = MeshComp->GetOwner();
	TArray<AActor*> Ignore; Ignore.Add(Owner);
	// 무기 액터가 따로 있으면 Add
	// Ignore.Add(WeaponActor);

	TArray<FHitResult> Hits;

	// 1) 블레이드 현재 선분 검사
	CollectHits(World, CurrStart, CurrEnd, Radius, TraceChannel, Ignore, Hits);

	// 2) 이전 → 현재 이동 보간(빠른 휘두름 보완)
	CollectHits(World, State->PreStart, CurrStart, Radius, TraceChannel, Ignore, Hits);
	CollectHits(World, State->PreEnd,   CurrEnd,   Radius, TraceChannel, Ignore, Hits);

	if (bDrawDebug)
	{
		DrawDebugLine(World, CurrStart, CurrEnd, FColor::Red, false, 0.06f, 0, 1.5f);
		DrawDebugSphere(World, CurrStart, Radius, 12, FColor::Green, false, 0.06f);
		DrawDebugSphere(World, CurrEnd,   Radius, 12, FColor::Green, false, 0.06f);
	}

	for (const FHitResult& HR : Hits)
	{
		HandleHit(MeshComp, HR, *State);
	}
}

void UANS_AttackTrace::CollectHits(UWorld* World, const FVector& A, const FVector& B, float InRadius,
	ECollisionChannel Channel, const TArray<AActor*>& Ignore, TArray<FHitResult>& OutHits) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AttackTrace), /*bTraceComplex*/ false);
	Params.bReturnPhysicalMaterial = false;
	Params.AddIgnoredActors(Ignore);

	TArray<FHitResult> LocalHits;
	World->SweepMultiByChannel(LocalHits, A, B, FQuat::Identity, Channel, FCollisionShape::MakeSphere(InRadius), Params);

	for (const FHitResult& H : LocalHits)
	{
		OutHits.Emplace(H);
	}
}

void UANS_AttackTrace::HandleHit(USkeletalMeshComponent* MeshComp, const FHitResult& Hit, FTraceState& State) const
{
	AActor* Owner = MeshComp->GetOwner();
	AActor* Other = Hit.GetActor();
	if (!Owner || !Other || Owner == Other) return;

	// 중복 히트 방지
	if (bSingleHitPerActor && State.AlreadyHitActors.Contains(Other)) return;
	
	FGameplayEventData EventData;
	EventData.Instigator = Owner;
	EventData.Target = Other;

	FGameplayAbilityTargetDataHandle DataHandle;
	DataHandle.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));
	EventData.TargetData = DataHandle;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, MyTags::Events::Hit::Hit, EventData);

	const FGameplayTag SendTag = HitTagEvent.IsValid() ? HitTagEvent : MyTags::Events::Hit::Hit;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, SendTag, EventData);

	// 2) 바로 데미지 효과 적용(선택): 서버만 실제 적용
	if (DamageEffect && Owner->HasAuthority())
	{
		const IAbilitySystemInterface* InstASI = Cast<IAbilitySystemInterface>(Owner);
		const IAbilitySystemInterface* TgtASI  = Cast<IAbilitySystemInterface>(Other);
		if (InstASI && TgtASI)
		{
			UAbilitySystemComponent* InstASC = InstASI->GetAbilitySystemComponent();
			UAbilitySystemComponent* TgtASC  = TgtASI->GetAbilitySystemComponent();
			if (InstASC && TgtASC)
			{
				FGameplayEffectContextHandle Ctx = InstASC->MakeEffectContext();
				Ctx.AddInstigator(Owner, Owner);
				FGameplayEffectSpecHandle Spec = InstASC->MakeOutgoingSpec(DamageEffect, /*Lvl*/1.f, Ctx);
				if (Spec.IsValid())
				{
					TgtASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}
	}

	State.AlreadyHitActors.Add(Other);
}

bool UANS_AttackTrace::ShouldRunOnThisMachine(const AActor* Owner) const
{
	if (!Owner) return false;

	// 서버는 항상 수행
	if (Owner->HasAuthority()) return true;

	// 클라 예측 허용 여부
	if (bClientAlsoTrace)
	{
		// 자신의 소유 폰만 (Remote 클라에서 중복 트레이스 방지)
		const APawn* Pawn = Cast<APawn>(Owner);
		return Pawn && Pawn->IsLocallyControlled();
	}
	return false;
}
