// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeePartyMemberWidget.generated.h"

class APlayerState;
class UImage;
class UProgressBar;
class UTextBlock;

// 26.11.04 추가 - 파티 HUD의 동료 한 명 표시 (W_PartyMember)
// 이름·HP바·스태미나바를 보여주고, 넉다운이면 테두리를 깜빡이고, 사망이면 전체를 회색으로 칠한다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeePartyMemberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 동료의 PlayerState 지정
	void SetPlayerState(APlayerState* InPlayerState);

	APlayerState* GetPlayerState() const { return PlayerState.Get(); }

	// 현재 HP·스태미나·상태로 화면 갱신 (TimeSeconds는 깜빡임 계산용)
	void Refresh(float TimeSeconds);

protected:
	// 26.11.04 추가 - 넉다운 깜빡임을 매 프레임 부드럽게 갱신 (Refresh는 0.1초 주기라 끊겨 보임)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Health;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Stamina;

	// 넉다운일 때 깜빡이는 테두리/발광 이미지 (없으면 깜빡임 생략)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_DownedGlow;

	// 깜빡임 속도 (초당 왕복 횟수에 비례)
	UPROPERTY(EditAnywhere, Category = "Lee|Party")
	float DownedBlinkSpeed = 6.0f;

	// 26.11.04 추가 - 넉다운 발광 색 (위급함을 나타내는 빨강)
	UPROPERTY(EditAnywhere, Category = "Lee|Party")
	FLinearColor DownedGlowColor = FLinearColor(1.0f, 0.05f, 0.03f, 1.0f);

	// 26.11.04 추가 - 넉다운 중 줄 전체가 깜빡일 때 가장 붉은 순간의 색 (흰색이면 줄 전체 깜빡임 없음)
	UPROPERTY(EditAnywhere, Category = "Lee|Party")
	FLinearColor DownedRowTint = FLinearColor(1.0f, 0.45f, 0.45f, 1.0f);

	// 사망한 동료를 칠할 색 (회색)
	UPROPERTY(EditAnywhere, Category = "Lee|Party")
	FLinearColor DeadTint = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

private:
	TWeakObjectPtr<APlayerState> PlayerState;

	// 26.11.04 추가 - 마지막 Refresh에서 판정한 상태 (NativeTick의 깜빡임 연출용)
	bool bIsDowned = false;
	bool bIsDead = false;
	float BlinkTime = 0.0f;
};
