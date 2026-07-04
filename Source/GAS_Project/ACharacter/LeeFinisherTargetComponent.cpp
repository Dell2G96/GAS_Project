// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinisherTargetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

ULeeFinisherTargetComponent::ULeeFinisherTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 콜리전에 의존하지 않는다 (Player 콜리전 프리셋과 무관하게 동작해야 함) — 순수 거리 계산 방식.
	// 물리적으로 아무것도 막지 않도록 콜리전을 완전히 끈다.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);

	InitSphereRadius(PromptRadius);
}

void ULeeFinisherTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	SetSphereRadius(PromptRadius);

	// 콜리전 오버랩 이벤트에 의존하지 않고 상시 주기적으로 직접 판정 (Player 콜리전 프리셋 영향 없음)
	GetWorld()->GetTimerManager().SetTimer(EvalTimerHandle, this, &ThisClass::EvaluatePrompt, EvalInterval, /*bLoop*/true);

	// ASC/AttributeSet 초기화가 오너 BeginPlay 이후일 수 있으므로 다음 틱에 바인딩
	if (GetOwnerRole() == ROLE_Authority)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::BindToStaminaDelegate);
	}
}

void ULeeFinisherTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvalTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ULeeFinisherTargetComponent::BindToStaminaDelegate()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	const ULeeSoulsStatSet* SoulsSet = ASC ? ASC->GetSet<ULeeSoulsStatSet>() : nullptr;
	if (!SoulsSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeFinisherTargetComponent] %s에서 LeeSoulsStatSet을 찾지 못했습니다. 그로기 자동 진입이 동작하지 않습니다."),
			*GetNameSafe(GetOwner()));
		return;
	}

	SoulsSet->OnOutOfStamina.AddUObject(this, &ThisClass::HandleOutOfStamina);
}

void ULeeFinisherTargetComponent::HandleOutOfStamina(AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/)
{
	// 서버 전용: 스태미나 고갈 → 그로기 GE 적용 (Status.Groggy + Status.Vulnerable.Execution)
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (!GroggyEffect)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeFinisherTargetComponent] GroggyEffect가 설정되지 않음. BP에서 GE_Groggy를 지정해주세요."));
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(GroggyEffect, /*Level*/1.0f, ASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ULeeFinisherTargetComponent::EvaluatePrompt()
{
	// UI는 로컬 플레이어에 대해서만 의미가 있다 — 이 머신의 로컬 플레이어 폰을 직접 찾는다
	// (콜리전 오버랩에 의존하지 않으므로 Player 콜리전 프리셋과 무관하게 동작)
	ELeeFinisherPromptPhase NewPhase = ELeeFinisherPromptPhase::Hidden;
	ELeeFinisherType NewType = LastType;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bool bFoundLocalPlayer = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		if (PlayerPawn && PlayerPawn->IsLocallyControlled())
		{
			bFoundLocalPlayer = true;
			ComputePromptFor(PlayerPawn, NewPhase, NewType);
			break;
		}
	}

	// [임시 진단 로그] 로컬 플레이어 폰 자체를 못 찾는 경우 (서버 전용 인스턴스 등)
	if (!bFoundLocalPlayer && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(int32)GetUniqueID(), 0.3f, FColor::Red,
			FString::Printf(TEXT("[Finisher] %s: 로컬 플레이어 폰을 못 찾음"), *GetNameSafe(GetOwner())));
	}

	BroadcastPrompt(NewPhase, NewType);
}

void ULeeFinisherTargetComponent::ComputePromptFor(
	const APawn* PlayerPawn, ELeeFinisherPromptPhase& OutPhase, ELeeFinisherType& OutType) const
{
	OutPhase = ELeeFinisherPromptPhase::Hidden;

	const AActor* Owner = GetOwner();
	UAbilitySystemComponent* OwnerASC = GetOwnerASC();

	// [임시 진단 로그] 화면에 실시간 상태 표시 — 원인 확정되면 제거할 것
	const float DebugDistSq = (Owner ? FVector::DistSquared2D(Owner->GetActorLocation(), PlayerPawn->GetActorLocation()) : -1.0);
	const bool bDebugGroggy = OwnerASC && OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy);
	const bool bDebugUnaware = OwnerASC && OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(int32)GetUniqueID(), 0.3f, FColor::Yellow,
			FString::Printf(TEXT("[Finisher] Owner=%s ASC=%s Groggy=%d Unaware=%d Dist=%.0f/%d(Prompt) PromptR=%.0f"),
				*GetNameSafe(Owner), OwnerASC ? TEXT("O") : TEXT("X"), bDebugGroggy, bDebugUnaware,
				FMath::Sqrt(FMath::Max(DebugDistSq, 0.0f)), 0, PromptRadius));
	}

	if (!Owner || !OwnerASC)
	{
		return;
	}

	// 사망했거나 이미 피니셔 진행 중이면 숨김
	if (OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Finisher_Victim) ||
		OwnerASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dying) ||
		OwnerASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dead))
	{
		return;
	}

	// PromptRadius 밖이면 더 볼 것도 없이 숨김 (콜리전 오버랩 대신 직접 거리로 판정)
	if (FVector::DistSquared2D(Owner->GetActorLocation(), PlayerPawn->GetActorLocation()) > FMath::Square(PromptRadius))
	{
		return;
	}

	// 그로기(처형)가 미인식(암살)보다 우선 — GA_Finisher::FindFinisherTarget과 동일 규칙
	bool bConditionMet = false;
	if (OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
	{
		OutType = ELeeFinisherType::Execution;
		bConditionMet = true;
	}
	else if (OwnerASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
	{
		// 후방 원뿔 판정: Dot(오너 전방, 오너→플레이어) <= -cos(반각)
		const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
		const FVector ToPlayer = (PlayerPawn->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		const float BehindDotThreshold = -FMath::Cos(FMath::DegreesToRadians(BehindAngleDeg * 0.5f));
		const float ActualDot = FVector::DotProduct(OwnerForward, ToPlayer);

		// [임시 진단 로그] 각도 판정 상세
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				(int32)GetUniqueID() + 1, 0.3f, FColor::Cyan,
				FString::Printf(TEXT("[Finisher-Angle] %s Dot=%.2f Threshold=%.2f (Dot<=Threshold 이면 통과) BehindAngleDeg=%.0f"),
					*GetNameSafe(Owner), ActualDot, BehindDotThreshold, BehindAngleDeg));
		}

		if (FVector::DotProduct(OwnerForward, ToPlayer) <= BehindDotThreshold)
		{
			OutType = ELeeFinisherType::Assassination;
			bConditionMet = true;
		}
	}

	if (!bConditionMet)
	{
		return;
	}

	// 오버랩 안 = PromptRadius 이내 보장. 거리로 표시/실행 단계 구분
	const float DistSq = FVector::DistSquared2D(Owner->GetActorLocation(), PlayerPawn->GetActorLocation());
	OutPhase = (DistSq <= FMath::Square(ExecuteRadius))
		? ELeeFinisherPromptPhase::Executable
		: ELeeFinisherPromptPhase::Visible;
}

void ULeeFinisherTargetComponent::BroadcastPrompt(ELeeFinisherPromptPhase Phase, ELeeFinisherType Type)
{
	// 단계가 바뀔 때만 발행
	if (Phase == LastPhase && (Phase == ELeeFinisherPromptPhase::Hidden || Type == LastType))
	{
		return;
	}

	LastPhase = Phase;
	LastType = Type;

	// [임시 진단 로그] 실제 메시지 발행 시점
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			(int32)GetUniqueID() + 2, 3.0f, FColor::Green,
			FString::Printf(TEXT("[Finisher-Broadcast] %s Phase=%d Type=%d 메시지 발행됨"),
				*GetNameSafe(GetOwner()), (int32)Phase, (int32)Type));
	}

	FLeeFinisherPromptMessage Message;
	Message.Target = GetOwner();
	Message.Type = Type;
	Message.Phase = Phase;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(MyTags::Souls::Message_Finisher_Prompt, Message);
}

UAbilitySystemComponent* ULeeFinisherTargetComponent::GetOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}
