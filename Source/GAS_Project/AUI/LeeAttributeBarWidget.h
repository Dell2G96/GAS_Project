#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeAttributeBarWidget.generated.h"

class UCommonNumericTextBlock;
class ULeeHealthComponent;

// 바가 표시할 속성 종류
UENUM(BlueprintType)
enum class ELeeAttributeBarType : uint8
{
	Health,
	Stamina
};

/**
 * HUD의 HP/스태미나 바 공용 부모 위젯.
 * 내 Pawn의 HealthComponent에 바인딩(리스폰 시 재바인딩)하고, 바인딩 즉시 현재 값으로 숫자를 맞춘다.
 * 바 머티리얼 연출은 BP가 OnAttributeBarUpdated 이벤트를 받아 처리한다.
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeAttributeBarWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 현재 값 / 최대 값 (0~1). 바인딩 전이면 0
	UFUNCTION(BlueprintPure, Category = "Lee|AttributeBar")
	float GetNormalizedValue() const;

protected:
	/**
	 * 바 표시 갱신 이벤트 (BP에서 바 머티리얼 파라미터를 갱신).
	 * bInstant = true : 바인딩·리스폰·최대값 변경 → 애니메이션 없이 바로 맞춤
	 * bInstant = false: 피격·회복 등 실제 변화 → 기존 감소/증가 연출 재생
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|AttributeBar")
	void OnAttributeBarUpdated(float OldValue, float NewValue, float OldNormalized, float NewNormalized, bool bInstant);

	// 이 위젯이 보여줄 속성 (HP 바 = Health, 스태미나 바 = Stamina)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttributeBar")
	ELeeAttributeBarType AttributeType = ELeeAttributeBarType::Health;

	// 숫자가 새 값으로 굴러가는 최대 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttributeBar", meta = (ClampMin = "0.0"))
	float NumberInterpolationDuration = 0.5f;

	// 숫자 표시 (기존 BP의 위젯 이름 그대로 사용). 기존 BP 그래프의 값 변경 연출이 읽을 수 있게 BlueprintReadOnly
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonNumericTextBlock> HealthNumber;

private:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	UFUNCTION()
	void HandleMaxValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	void BindToPawn(APawn* Pawn);
	void UnbindFromHealthComponent();

	// 현재 값으로 숫자와 바를 즉시 맞춘다
	void RefreshInstant();

	float GetCurrentValue() const;
	float GetMaxValue() const;

	TWeakObjectPtr<ULeeHealthComponent> BoundHealthComponent;
};
