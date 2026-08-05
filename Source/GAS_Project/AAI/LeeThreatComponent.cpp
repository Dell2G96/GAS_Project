// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeThreatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/AMessage/LeeVerbMessageHelpers.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

// 위협도 컴포넌트 생성자. 판정은 외부(TargetSelection 등)가 주기적으로 호출하므로 자체 틱은 쓰지 않는다
ULeeThreatComponent::ULeeThreatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 기본 감시 대상은 Souls 체력. Enemy가 다른 AttributeSet을 쓰면 BP에서 교체한다
	HealthAttribute = ULeeSoulsStatSet::GetHealthAttribute();
}

// 서버에서만 체력 감소 델리게이트를 구독한다 (§6 서버 권위 정책 / 회수 경로와 무관한 기록 경로)
void ULeeThreatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ensureMsgf(ThreatHalfLife > 0.f, TEXT("ULeeThreatComponent: ThreatHalfLife는 0보다 커야 합니다.")))
	{
		ThreatHalfLife = 1.f;
	}

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bAutoTrackHealthDamage)
	{
		return;
	}

	// ALeeCharacter 계열은 ASC가 PawnExtensionComponent를 통해 BeginPlay 이후에 초기화된다.
	// 그래서 직접 조회하지 않고, 이미 초기화됐으면 즉시·아직이면 초기화 시점에 호출되는 델리게이트를 쓴다
	// (ALeeCharacter 자신이 HealthComponent를 붙일 때 쓰는 것과 동일한 패턴).
	if (ULeePawnExtensionComponent* PawnExt = ULeePawnExtensionComponent::FindPawnExtensionComponent(Owner))
	{
		PawnExt->OnAbilitySystemInitialized_RegistedAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::BindHealthAttributeDelegate));
		return;
	}

	BindHealthAttributeDelegate();
}

// ASC가 준비된 시점에 체력 변경 델리게이트를 구독. 재초기화로 두 번 불려도 안전하게 동작한다
void ULeeThreatComponent::BindHealthAttributeDelegate()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bAutoTrackHealthDamage)
	{
		return;
	}

	if (!HealthAttribute.IsValid())
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeThreatComponent] %s: HealthAttribute가 지정되지 않아 위협도가 자동 누적되지 않습니다."), *GetNameSafe(Owner));
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeThreatComponent] %s: ASC를 찾지 못해 위협도가 자동 누적되지 않습니다."), *GetNameSafe(Owner));
		return;
	}

	// 이미 같은 ASC에 구독돼 있으면 아무것도 하지 않는다
	if (BoundASC.Get() == ASC && HealthChangedDelegateHandle.IsValid())
	{
		return;
	}

	// 다른 ASC에 물려 있었다면 먼저 해제
	if (UAbilitySystemComponent* OldASC = BoundASC.Get())
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			OldASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(HealthChangedDelegateHandle);
		}
	}

	BoundASC = ASC;
	HealthChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute)
		.AddUObject(this, &ThisClass::HandleHealthAttributeChanged);
}

// 구독 해제. ASC가 컴포넌트보다 오래 살아남는 경우를 대비해 반드시 명시적으로 푼다
void ULeeThreatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		if (HealthChangedDelegateHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(HealthChangedDelegateHandle);
		}
	}
	BoundASC.Reset();
	HealthChangedDelegateHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

// 체력이 줄어든 만큼을 위협도로 환산해 누적. 가해자는 GE 컨텍스트의 OriginalInstigator에서 얻는다
void ULeeThreatComponent::HandleHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	// 감소분만 인정 (회복은 위협도에 기여하지 않음 — Q9 결정안)
	const float Delta = Data.OldValue - Data.NewValue;
	if (Delta <= 0.f)
	{
		return;
	}

	// GEModData는 GameplayEffect로 인한 변경일 때만 채워진다. 직접 SetNumericAttribute 호출 등은 가해자를 알 수 없어 무시한다
	if (!Data.GEModData)
	{
		return;
	}

	AActor* RawInstigator = Data.GEModData->EffectSpec.GetContext().GetOriginalInstigator();
	if (!RawInstigator)
	{
		RawInstigator = Data.GEModData->EffectSpec.GetContext().GetEffectCauser();
	}

	// AddThreat 내부에서 DamageToThreatScale을 곱하므로 여기서는 원시 감소량만 넘긴다
	AddThreat(RawInstigator, Delta);
}

// 현재 시각까지 반감기 감쇠를 적용한 위협도 계산
float ULeeThreatComponent::ComputeDecayedThreat(const FLeeThreatEntry& Entry, double Now) const
{
	const double Elapsed = Now - Entry.LastUpdateTime;
	if (Elapsed <= 0.0)
	{
		return Entry.Threat;
	}

	const double DecayFactor = FMath::Pow(0.5, Elapsed / static_cast<double>(ThreatHalfLife));
	return static_cast<float>(Entry.Threat * DecayFactor);
}

// RawInstigator를 Player Pawn으로 정규화해 위협도를 누적한다 (Controller/PlayerState/Pawn 무엇이 들어와도 동작)
void ULeeThreatComponent::AddThreat(AActor* RawInstigator, float Amount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (Amount <= 0.f)
	{
		return;
	}

	APlayerState* PS = ULeeVerbMessageHelpers::GetPlayerStateFromObject(RawInstigator);
	APawn* NormalizedInstigator = PS ? PS->GetPawn() : Cast<APawn>(RawInstigator);
	if (!NormalizedInstigator)
	{
		UE_LOG(LogLee, Warning, TEXT("ULeeThreatComponent: RawInstigator(%s)를 Player Pawn으로 정규화하지 못했습니다."), *GetNameSafe(RawInstigator));
		return;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FLeeThreatEntry& Entry = ThreatTable.FindOrAdd(NormalizedInstigator);
	Entry.Threat = ComputeDecayedThreat(Entry, Now) + Amount * DamageToThreatScale;
	Entry.LastUpdateTime = Now;
}

// 조회 시점까지 감쇠를 적용한 현재 위협도를 반환 (lazy decay, 테이블은 변경하지 않음)
float ULeeThreatComponent::GetThreat(const AActor* Actor) const
{
	if (!Actor)
	{
		return 0.f;
	}

	const FLeeThreatEntry* Entry = ThreatTable.Find(Actor);
	if (!Entry)
	{
		return 0.f;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	return ComputeDecayedThreat(*Entry, Now);
}

// 무효화된 대상 및 MinThreatThreshold 미만으로 감쇠한 엔트리를 정리한다
void ULeeThreatComponent::PruneExpiredThreats()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	for (auto It = ThreatTable.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Key.Get();
		if (!Actor)
		{
			It.RemoveCurrent();
			continue;
		}

		if (ComputeDecayedThreat(It->Value, Now) < MinThreatThreshold)
		{
			It.RemoveCurrent();
		}
	}
}

// 모든 위협도 기록을 초기화한다 (리스폰 시 등 — Q7 결정안: 승계하지 않고 초기화)
void ULeeThreatComponent::ResetThreat()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	ThreatTable.Empty();
}
