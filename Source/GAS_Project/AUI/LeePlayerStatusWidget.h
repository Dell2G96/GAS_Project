// [26.10.30 빌드 검증 완료 — 학습용 주석 처리] 직접 타이핑하며 학습 후 각 줄 앞의 "//" 를 풀어 사용
#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeePlayerStatusWidget.generated.h"

class UImage;
class UTextBlock;
class ULeeHealthComponent;


// HP/스태미나 바는 BP에서 기존 W_HealthBar/W_StaminaBar를 자식으로 배치하고,
// 이 클래스는 에스트(회복약) 아이콘 + 남은 횟수 숫자 표시만 담당한다 (0이면 아이콘을 어둡게)
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeePlayerStatusWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	// 사용 시 횟수 변경 
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Estus")
	void OnEstusChargesUpdated(int32 OldCharges, int32 NewCharges);

private:
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleEstusChargesChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	void BindToPawn(APawn* Pawn);
	void UnbindFromHealthComponent();
	void RefreshEstus(int32 Charges);

protected:
	// 아이콘 
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_EstusIcon;

	// 남은 횟수 숫자
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_EstusCount;

	// 남은 횟수가 있을 때 아이콘 색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Estus")
	FLinearColor AvailableIconColor = FLinearColor::White;

	// 남은 횟수가 0일 때 아이콘 색 (어둡게)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Estus")
	FLinearColor EmptyIconColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

private:
	// 현재 바인딩 중인 HealthComponent (리스폰 시 교체)
	TWeakObjectPtr<ULeeHealthComponent> BoundHealthComponent;
};
