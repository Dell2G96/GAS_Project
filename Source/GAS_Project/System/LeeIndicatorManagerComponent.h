// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "LeeIndicatorManagerComponent.generated.h"


class AController;
class UIndicatorDescriptor;
class UObject;
struct FFrame;

UCLASS(BlueprintType, Blueprintable)
class GAS_PROJECT_API ULeeIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	ULeeIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer);

	static ULeeIndicatorManagerComponent* GetComponent(AController* Controller);


	UFUNCTION(BlueprintCallable, Category= Indicator)
	void AddIndicator(UIndicatorDescriptor* IndicatorDescriptor);

	UFUNCTION(BlueprintCallable, Category= Indicator)
	void RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor);

	DECLARE_EVENT_OneParam(ULeeIndicatorManagerComponent, FIndicatorEvent,  UIndicatorDescriptor* Indicator);

	FIndicatorEvent OnIndicatorAdded;
	FIndicatorEvent OnIndicatorRemoved;

	const TArray<UIndicatorDescriptor*>& GetIndicators() const {return Indicators;}

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// ---------------------------------------------------------------------
	// 26.11.08 추가 - 적 머리 위 HP 바(Nameplate) 표시 규칙
	// 플레이어 캐릭터로부터 일정 거리 안에 있고, 화면 안에 있으며 벽에 가리지 않으면 보인다
	// ---------------------------------------------------------------------

	/** 위젯 클래스 이름이 이 글자로 시작하는 인디케이터를 적 HP 바로 본다 (W_Nameplate, W_Nameplate_Souls) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Nameplate")
	FString NameplateWidgetPrefix = TEXT("W_Nameplate");

	/** HP 바가 보이는 최대 거리(cm) — 플레이어 캐릭터 기준 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Nameplate", meta = (ClampMin = "0.0"))
	float NameplateMaxDistance = 1500.0f;

	/** 표시 판정 주기(초) — 매 프레임 할 필요가 없어 짧은 타이머로 돈다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Nameplate", meta = (ClampMin = "0.02"))
	float NameplateUpdateInterval = 0.1f;

private:
	UPROPERTY()
	TArray<TObjectPtr<class UIndicatorDescriptor>> Indicators;

	FTimerHandle NameplateTimer;

	/** 모든 적 HP 바 인디케이터의 표시 여부를 갱신한다 (로컬 플레이어 컨트롤러에서만) */
	void UpdateNameplateVisibility();

	/** 이 인디케이터가 적 HP 바인가 (위젯 클래스 이름으로 판단) */
	bool IsNameplateIndicator(const UIndicatorDescriptor* Indicator) const;
};
