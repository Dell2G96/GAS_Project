// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePartyStatusWidget.h"

#include "LeePartyMemberWidget.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/GameModes/LeeBossPartySubsystem.h"

// 생성 직후 한 번 갱신 (자신은 항상 보이게 두고 클릭만 통과시킨다 — 숨기면 Tick이 멈춤)
void ULeePartyStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshParty();
}

// RefreshInterval마다 파티 갱신
void ULeePartyStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceRefresh += InDeltaTime;
	if (TimeSinceRefresh >= RefreshInterval)
	{
		TimeSinceRefresh = 0.0f;
		RefreshParty();
	}
}

// GameState의 플레이어 목록에서 나(소유 플레이어)를 뺀 동료만 모은다
TArray<APlayerState*> ULeePartyStatusWidget::GetTeammates() const
{
	TArray<APlayerState*> Result;

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const APlayerState* MyPlayerState = GetOwningPlayerState();
	if (!GameState)
	{
		return Result;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		// 26.11.04 수정 - 적 AI(봇 컨트롤러)도 PlayerState를 가져 PlayerArray에 들어오므로 봇은 제외한다
		if (PlayerState && PlayerState != MyPlayerState && !PlayerState->IsInactive() && !PlayerState->IsABot())
		{
			Result.Add(PlayerState);
		}
	}
	return Result;
}

// 보스전이 아니면 목록을 숨김. 동료 구성이 바뀌면 줄을 다시 만들고, 각 줄의 HP·스태미나·상태 갱신
void ULeePartyStatusWidget::RefreshParty()
{
	if (!VerticalBox_Members || !MemberWidgetClass)
	{
		return;
	}

	// 26.11.04 수정 - 자기 자신이 아니라 목록(VerticalBox)만 숨긴다.
	// Collapsed 위젯은 NativeTick이 호출되지 않아, 한 번 숨으면(Experience 로딩 전·필드) 보스전이 되어도 다시 켜지지 않았다
	const bool bShow = ULeeBossPartySubsystem::IsBossMultiplayer(this);
	VerticalBox_Members->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShow)
	{
		return;
	}

	const TArray<APlayerState*> Teammates = GetTeammates();

	// 동료 구성(입장·퇴장)이 바뀌었는지 확인
	bool bChanged = (Teammates.Num() != MemberWidgets.Num());
	for (int32 Index = 0; !bChanged && Index < Teammates.Num(); ++Index)
	{
		bChanged = (MemberWidgets[Index]->GetPlayerState() != Teammates[Index]);
	}

	if (bChanged)
	{
		VerticalBox_Members->ClearChildren();
		MemberWidgets.Reset();

		for (APlayerState* PlayerState : Teammates)
		{
			ULeePartyMemberWidget* MemberWidget = CreateWidget<ULeePartyMemberWidget>(this, MemberWidgetClass);
			MemberWidget->SetPlayerState(PlayerState);
			VerticalBox_Members->AddChildToVerticalBox(MemberWidget);
			MemberWidgets.Add(MemberWidget);
		}
	}

	const float TimeSeconds = GetWorld()->GetTimeSeconds();
	for (ULeePartyMemberWidget* MemberWidget : MemberWidgets)
	{
		MemberWidget->Refresh(TimeSeconds);
	}
}
