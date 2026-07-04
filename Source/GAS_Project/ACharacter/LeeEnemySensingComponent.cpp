// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeEnemySensingComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "GAS_Project/MyTags.h"

ULeeEnemySensingComponent::ULeeEnemySensingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULeeEnemySensingComponent::BeginPlay()
{
	Super::BeginPlay();

	// 판정은 서버 전용. 태그는 리플리케이트로 클라이언트에 전달된다.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// 스폰 시 미인식 상태로 시작 (ASC 초기화 순서 때문에 다음 틱에 적용)
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetUnaware(true);
		}));

	GetWorld()->GetTimerManager().SetTimer(SenseTimerHandle, this, &ThisClass::SenseTick, SenseInterval, /*bLoop*/true);
}

void ULeeEnemySensingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SenseTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ULeeEnemySensingComponent::SenseTick()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 피니셔 당하는 중이거나 사망 시 인식 상태를 건드리지 않는다
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Finisher_Victim) ||
		ASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dying) ||
		ASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dead))
	{
		return;
	}

	bool bPerceivedAny = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (PlayerPawn && CanPerceive(PlayerPawn))
		{
			bPerceivedAny = true;
			break;
		}
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (bPerceivedAny)
	{
		LastPerceivedTime = Now;
		if (bUnaware)
		{
			// 인식 성공 → 즉시 암살 불가 상태로 전환 (UI는 다음 판정 주기에 숨겨짐)
			SetUnaware(false);
		}
	}
	else if (!bUnaware && (Now - LastPerceivedTime) > LoseSightDelay)
	{
		// 전투 이탈 → 다시 암살 가능 상태로 복귀
		SetUnaware(true);
	}
}

bool ULeeEnemySensingComponent::CanPerceive(const APawn* PlayerPawn) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !PlayerPawn)
	{
		return false;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// ① 거리
	if (FVector::DistSquared(OwnerLocation, PlayerLocation) > FMath::Square(SightRadius))
	{
		return false;
	}

	// ② 전방 시야각 (원뿔 전체 각 SightAngleDeg → 반각과 비교)
	const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToPlayer = (PlayerLocation - OwnerLocation).GetSafeNormal2D();
	const float FrontDotThreshold = FMath::Cos(FMath::DegreesToRadians(SightAngleDeg * 0.5f));
	if (FVector::DotProduct(OwnerForward, ToPlayer) < FrontDotThreshold)
	{
		return false;
	}

	// ③ 차폐 검사 (눈높이 → 플레이어 중심)
	if (bCheckLineOfSight)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeEnemySensing), /*bTraceComplex*/false, Owner);
		QueryParams.AddIgnoredActor(PlayerPawn);

		FHitResult Hit;
		const FVector EyeLocation = OwnerLocation + FVector(0.0f, 0.0f, EyeHeightOffset);
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, PlayerLocation, ECC_Visibility, QueryParams))
		{
			return false;
		}
	}

	return true;
}

void ULeeEnemySensingComponent::SetUnaware(bool bNewUnaware)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	bUnaware = bNewUnaware;

	// 서버 로컬 카운트 + 리플리케이트 양쪽 반영 — 클라이언트의 프롬프트 판정에서도 태그가 보여야 함
	if (bNewUnaware)
	{
		if (!ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
		{
			ASC->AddLooseGameplayTag(MyTags::Souls::Status_Unaware);
			ASC->AddReplicatedLooseGameplayTag(MyTags::Souls::Status_Unaware);
		}
	}
	else
	{
		if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
		{
			ASC->RemoveLooseGameplayTag(MyTags::Souls::Status_Unaware);
			ASC->RemoveReplicatedLooseGameplayTag(MyTags::Souls::Status_Unaware);
		}
	}
}

UAbilitySystemComponent* ULeeEnemySensingComponent::GetOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
