// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_OrientToTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GAS_Project/MyTags.h"
#include "Kismet/KismetMathLibrary.h"

UBTService_OrientToTarget::UBTService_OrientToTarget()
{
	NodeName = TEXT("OrientToTarget");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	RotationInterpSpeed = 5.f;
	Interval = 0.f;
	RandomDeviation = 0.f;

	InTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetKey), AActor::StaticClass());
}

void UBTService_OrientToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InTargetKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_OrientToTarget::GetStaticDescription() const
{
	const FString KeyDescription = InTargetKey.SelectedKeyName.ToString();
	return FString::Printf(TEXT("Orient rotation to %s Key %s"), *KeyDescription, *GetStaticServiceDescription());
}

void UBTService_OrientToTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	UObject* ActorObject = OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(ActorObject);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	//
	// APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();
	// if (OwningPawn && TargetActor)
	// {
	// 	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(OwningPawn->GetActorLocation(), TargetActor->GetActorLocation());
	// 	const FRotator TargetRot = FMath::RInterpTo(OwningPawn->GetActorRotation(), LookAtRot, DeltaSeconds, RotationInterpSpeed);
	//
	// 	OwningPawn->SetActorRotation(TargetRot);
	// }


	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!TargetASC)
	{
		return;
	}
	if (TargetASC->HasMatchingGameplayTag(MyTags::Status::Knockdown))
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		OwnerComp.GetBlackboardComponent()->ClearValue(InTargetKey.SelectedKeyName);
		return ;
	}

	if (!TargetActor)
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}
	if (!AIC || !Pawn)
	{
		
		return;
	}

	// 1) 포커스를 타겟으로 고정 (스트레이프의 기준)
	AIC->SetFocus(TargetActor);


	// 2) (선택) 컨트롤러 회전을 직접 보간해서 부드럽게
	if (TargetActor)
	{
		const FVector PawnLoc = Pawn->GetActorLocation();
		const FVector TargetLoc = TargetActor->GetActorLocation();
		FRotator Desired = (TargetLoc - PawnLoc).Rotation();
		Desired.Pitch = 0.f;
		Desired.Roll  = 0.f;
		const FRotator Current = AIC->GetControlRotation();
		const FRotator NewRot = FMath::RInterpTo(Current, Desired, DeltaSeconds, 10.f /*회전속도*/);
		AIC->SetControlRotation(NewRot);
	}
	
}
