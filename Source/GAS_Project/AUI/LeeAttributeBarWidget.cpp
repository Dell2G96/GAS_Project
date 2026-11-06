#include "LeeAttributeBarWidget.h"

#include "CommonNumericTextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"

// 위젯 생성 시 Pawn 변경(리스폰)을 구독하고 현재 Pawn에 바로 바인딩
void ULeeAttributeBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
		BindToPawn(PlayerController->GetPawn());
	}
}

// 위젯 제거 시 모든 바인딩 해제
void ULeeAttributeBarWidget::NativeDestruct()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	UnbindFromHealthComponent();

	Super::NativeDestruct();
}

// 새 Pawn에 빙의(리스폰)하면 새 HealthComponent로 다시 바인딩
void ULeeAttributeBarWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

// Pawn의 HealthComponent에서 이 바의 값/최대값 변경 이벤트를 구독하고, 현재 값으로 즉시 갱신
void ULeeAttributeBarWidget::BindToPawn(APawn* Pawn)
{
	UnbindFromHealthComponent();

	ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Pawn);
	if (!HealthComponent)
	{
		return;
	}

	if (AttributeType == ELeeAttributeBarType::Health)
	{
		HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleValueChanged);
		HealthComponent->OnMaxHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleMaxValueChanged);
	}
	else
	{
		HealthComponent->OnStaminaChanged.AddUniqueDynamic(this, &ThisClass::HandleValueChanged);
		HealthComponent->OnMaxStaminaChanged.AddUniqueDynamic(this, &ThisClass::HandleMaxValueChanged);
	}
	BoundHealthComponent = HealthComponent;

	// 핵심: 이벤트를 기다리지 않고 지금 값으로 맞춘다 (초기화 브로드캐스트가 바인딩보다 먼저 지나간 경우 대비)
	RefreshInstant();
}

// 이전 HealthComponent 구독 해제
void ULeeAttributeBarWidget::UnbindFromHealthComponent()
{
	if (ULeeHealthComponent* HealthComponent = BoundHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleValueChanged);
		HealthComponent->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::HandleMaxValueChanged);
		HealthComponent->OnStaminaChanged.RemoveDynamic(this, &ThisClass::HandleValueChanged);
		HealthComponent->OnMaxStaminaChanged.RemoveDynamic(this, &ThisClass::HandleMaxValueChanged);
	}
	BoundHealthComponent.Reset();
}

// 값 변경 수신 — 실제로 바뀌었으면 숫자를 굴리고 바 연출, 같은 값(초기화 브로드캐스트)이면 즉시 맞춤
void ULeeAttributeBarWidget::HandleValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (FMath::IsNearlyEqual(OldValue, NewValue))
	{
		RefreshInstant();
		return;
	}

	if (HealthNumber)
	{
		HealthNumber->InterpolateToValue(NewValue, NumberInterpolationDuration);
	}

	const float MaxValue = GetMaxValue();
	const float OldNormalized = (MaxValue > 0.0f) ? FMath::Clamp(OldValue / MaxValue, 0.0f, 1.0f) : 0.0f;
	OnAttributeBarUpdated(OldValue, NewValue, OldNormalized, GetNormalizedValue(), /*bInstant*/false);
}

// 최대값 변경 수신 — 비율이 바뀌므로 즉시 맞춤
void ULeeAttributeBarWidget::HandleMaxValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	RefreshInstant();
}

// 현재 값으로 숫자와 바를 애니메이션 없이 맞춘다
void ULeeAttributeBarWidget::RefreshInstant()
{
	const float CurrentValue = GetCurrentValue();
	if (HealthNumber)
	{
		HealthNumber->SetCurrentValue(CurrentValue);
	}

	const float Normalized = GetNormalizedValue();
	OnAttributeBarUpdated(CurrentValue, CurrentValue, Normalized, Normalized, /*bInstant*/true);
}

// 바인딩된 HealthComponent에서 이 바의 현재 값을 읽는다
float ULeeAttributeBarWidget::GetCurrentValue() const
{
	const ULeeHealthComponent* HealthComponent = BoundHealthComponent.Get();
	if (!HealthComponent)
	{
		return 0.0f;
	}
	return (AttributeType == ELeeAttributeBarType::Health) ? HealthComponent->GetHealth() : HealthComponent->GetStamina();
}

// 바인딩된 HealthComponent에서 이 바의 최대 값을 읽는다
float ULeeAttributeBarWidget::GetMaxValue() const
{
	const ULeeHealthComponent* HealthComponent = BoundHealthComponent.Get();
	if (!HealthComponent)
	{
		return 0.0f;
	}
	return (AttributeType == ELeeAttributeBarType::Health) ? HealthComponent->GetMaxHealth() : HealthComponent->GetMaxStamina();
}

// 현재 값 / 최대 값 (0~1)
float ULeeAttributeBarWidget::GetNormalizedValue() const
{
	const float MaxValue = GetMaxValue();
	return (MaxValue > 0.0f) ? FMath::Clamp(GetCurrentValue() / MaxValue, 0.0f, 1.0f) : 0.0f;
}
