// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeePartyStatusWidget.generated.h"

class ULeePartyMemberWidget;
class UVerticalBox;

// 26.11.04 추가 - 화면 오른쪽 중앙의 파티 HUD (W_PartyStatus)
// 보스전(로비·보스맵)에서만 보이며, 나를 뺀 동료들의 W_PartyMember를 세로로 나열하고 주기적으로 갱신한다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeePartyStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 동료 목록이 바뀌었으면 줄을 다시 만들고, 모든 줄을 갱신
	void RefreshParty();

	// 나를 뺀 동료 PlayerState 목록
	TArray<APlayerState*> GetTeammates() const;

	// 동료 줄을 담는 세로 박스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_Members;

	// 동료 한 명 위젯 클래스 (W_PartyMember)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Party")
	TSubclassOf<ULeePartyMemberWidget> MemberWidgetClass;

	// 갱신 주기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Party", meta = (ClampMin = "0.02"))
	float RefreshInterval = 0.1f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULeePartyMemberWidget>> MemberWidgets;

	float TimeSinceRefresh = 0.0f;
};
