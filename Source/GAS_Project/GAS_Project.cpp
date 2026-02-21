// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS_Project.h"

#include "LeeLogChannels.h"
#include "Modules/ModuleManager.h"

class FGAS_ProjectMoudle : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

void FGAS_ProjectMoudle::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
	UE_LOG(LogLee, Warning, TEXT("LeeGameModule Startup"));
}

void FGAS_ProjectMoudle::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
}

IMPLEMENT_PRIMARY_GAME_MODULE(FGAS_ProjectMoudle, GAS_Project, "GAS_Project");



// IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, GAS_Project, "GAS_Project" );
