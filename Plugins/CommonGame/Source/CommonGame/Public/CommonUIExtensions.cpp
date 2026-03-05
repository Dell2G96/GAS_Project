// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonUIExtensions.h"

#include "CommonLocalPlayer.h"
#include "CommonActivatableWidget.h"
#include "GameUIManagerSubsystem.h"
#include "GameUIPolicy.h"

class UCommonActivatableWidget* UCommonUIExtensions::PushContentToLayer_ForPlayer(const class ULocalPlayer* LocalPlayer,
                                                                                  FGameplayTag LayerName, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	// 로컬플레이어를 통해 게임UIManagerSubsystem을 가져오기
	if (UGameUIManagerSubsystem* UIManager = LocalPlayer->GetGameInstance()->GetSubsystem<UGameUIManagerSubsystem>())
	{
		// UIManager에서 현재 활성화된 UIPolicy 가져오기
		if (UGameUIPolicy* Policy = UIManager->GetCurrentUIPolicy())
		{
			// Policy에서 Localplayer에 맞는 primaryGamelayout을 가져오기
			if (UPrimaryGameLayout* RootLayout = Policy->GetRootLayout(CastChecked<UCommonLocalPlayer>(LocalPlayer)))
			{
				// PrimaryGamelayout , W_OverallUILayout의 LayerName에 Stack으로 Widgetclass을 넣어준다
				return RootLayout->PushWidgetToLayerStack(LayerName, WidgetClass);
			}
		}
	}
	return nullptr;
}
