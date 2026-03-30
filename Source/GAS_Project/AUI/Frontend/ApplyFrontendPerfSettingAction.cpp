// Fill out your copyright notice in the Description page of Project Settings.


#include "ApplyFrontendPerfSettingAction.h"


//////////////////////////////////////////////////////////////////////
// UApplyFrontendPerfSettingsAction

// 게임 사용자 설정(그리고 이를 통해 구동되는 엔진 성능/확장성 설정)은
// 글로벌이므로 멀티플레이어 PIE에서 월드별로 추적할 필요 없음:
// 메뉴 상태인 PIE 월드가 있으면 적용만 하면 됨.
//
// 하지만 기본적으로는 개발자 설정 ApplyFrontEndPerformanceOptionsInPIE가
// 활성화되지 않으면 에디터에서 프론트엔드 성능 설정을 적용하지 않음
void UApplyFrontendPerfSettingAction::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	// ApplicationCounter++;
	// if (ApplicationCounter == 1)
	// {
	// 	ULeeSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(true);
	// }

}

void UApplyFrontendPerfSettingAction::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	// ApplicationCounter--;
	// check(ApplicationCounter >= 0);
	//
	// if (ApplicationCounter == 0)
	// {
	// 	ULeeSettingsLocal::Get()->SetShouldUseFrontendPerformanceSettings(false);
	// }
}
