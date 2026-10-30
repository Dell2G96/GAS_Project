#include "LeePlayerStatusWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"

// 위젯 생성 시 PlayerController의 Pawn 변경(리스폰)을 구독 후, 현재 Pawn에 바로 바인딩
void ULeePlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
		BindToPawn(PlayerController->GetPawn());
	}
}

// 위젯 제거 시 모든 델리게이트 바인딩 해제
void ULeePlayerStatusWidget::NativeDestruct()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	UnbindFromHealthComponent();

	Super::NativeDestruct();
}

// 새 Pawn에 빙의(리스폰)하면 새로운 HealthComponent로 다시 바인딩
void ULeePlayerStatusWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

// Pawn의 HealthComponent를 찾아 에스트 횟수 변경 이벤트를 구독하고 현재 값으로 갱신
void ULeePlayerStatusWidget::BindToPawn(APawn* Pawn)
{
	UnbindFromHealthComponent();

	ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Pawn);
	if (!HealthComponent)
	{
		RefreshEstus(0);
		return;
	}

	HealthComponent->OnEstusChargesChanged.AddUniqueDynamic(this, &ThisClass::HandleEstusChargesChanged);
	BoundHealthComponent = HealthComponent;

	// ASC 초기화 전이면 0이 나오지만, InitializeWithAbilitySystem이 초기값을 한 번 브로드캐스트해 준다
	RefreshEstus(FMath::RoundToInt(HealthComponent->GetEstusCharges()));
}

// 이전 HealthComponent 구독 해제
void ULeePlayerStatusWidget::UnbindFromHealthComponent()
{
	if (ULeeHealthComponent* HealthComponent = BoundHealthComponent.Get())
	{
		HealthComponent->OnEstusChargesChanged.RemoveDynamic(this, &ThisClass::HandleEstusChargesChanged);
	}
	BoundHealthComponent.Reset();
}

// 에스트 횟수 변경 수신 (마시기 / 휴식 / 부활) → 표시 갱신 + BP 연출 이벤트 호출
void ULeePlayerStatusWidget::HandleEstusChargesChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	const int32 OldCharges = FMath::RoundToInt(OldValue);
	const int32 NewCharges = FMath::RoundToInt(NewValue);

	RefreshEstus(NewCharges);
	OnEstusChargesUpdated(OldCharges, NewCharges);
}

// 숫자 텍스트와 아이콘 색(0이면 어둡게)을 갱신
void ULeePlayerStatusWidget::RefreshEstus(int32 Charges)
{
	if (Text_EstusCount)
	{
		Text_EstusCount->SetText(FText::AsNumber(Charges));
	}

	if (Image_EstusIcon)
	{
		Image_EstusIcon->SetColorAndOpacity(Charges > 0 ? AvailableIconColor : EmptyIconColor);
	}
}
