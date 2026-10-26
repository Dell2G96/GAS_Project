#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"

class FToolBarButton : public TCommands<FToolBarButton>
{
public:
	FToolBarButton();
	~FToolBarButton();
	
public:
	virtual void RegisterCommands() override;
	
public:
	// 명령이 들어올 때 어떤 명령을 실행할지 저장
	TSharedPtr<FUICommandList> Command_Slomo;
	
public:
	// 어떤 작업을 입력받을지
	TSharedPtr<FUICommandInfo> btnCommandInfo1;
	TSharedPtr<FUICommandInfo> btnCommandInfo2;
	TSharedPtr<FUICommandInfo> btnCommandInfo3;
	TSharedPtr<FUICommandInfo> btnCommandInfo4;
	
public:
	static void OnPIEStarted(bool bIsSimulating);
	
private:
	void SlowSlomo();
	void FastSlomo();
	void SlowMin();
	void SlowDefault();
	
private:
	float GameplaySpeed = 1.0f;
	
	
};
