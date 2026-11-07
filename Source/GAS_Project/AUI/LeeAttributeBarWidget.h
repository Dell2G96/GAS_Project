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

	// 26.11.07 추가 - 지정한 액터(보스 등)의 HealthComponent에 바인딩. nullptr이면 바인딩 해제
	UFUNCTION(BlueprintCallable, Category = "Lee|AttributeBar")
	void BindToActor(AActor* Actor);

	// 26.11.07 추가 - 지금 바인딩된 HealthComponent (없으면 nullptr). BP 연출 이벤트에 넘겨 줄 때 사용
	UFUNCTION(BlueprintPure, Category = "Lee|AttributeBar")
	ULeeHealthComponent* GetBoundHealthComponent() const;

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

	// 26.11.07 추가 - true면 내 Pawn에 자동 바인딩(플레이어 HUD). 보스 바처럼 다른 액터를 보여줄 땐 false로 두고 BindToActor를 호출
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttributeBar")
	bool bBindToOwningPawn = true;

	// 숫자가 새 값으로 굴러가는 최대 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttributeBar", meta = (ClampMin = "0.0"))
	float NumberInterpolationDuration = 0.5f;

	// 숫자 표시 (기존 BP의 위젯 이름 그대로 사용). 기존 BP 그래프의 값 변경 연출이 읽을 수 있게 BlueprintReadOnly
	// 26.11.07 수정 - 보스 바처럼 숫자가 없는 바도 쓰도록 BindWidgetOptional로 변경
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonNumericTextBlock> HealthNumber;

private:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	UFUNCTION()
	void HandleMaxValueChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	void UnbindFromHealthComponent();

	// 현재 값으로 숫자와 바를 즉시 맞춘다
	void RefreshInstant();

	float GetCurrentValue() const;
	float GetMaxValue() const;

	TWeakObjectPtr<ULeeHealthComponent> BoundHealthComponent;
};
