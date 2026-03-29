// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeDeveloperSettings.h"

#include "Misc/App.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(LeeDeveloperSettings)

ULeeDeveloperSettings::ULeeDeveloperSettings()
{
}

FName ULeeDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void ULeeDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplySettings();
}

void ULeeDeveloperSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	ApplySettings();
}

void ULeeDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();

	ApplySettings();
}

void ULeeDeveloperSettings::ApplySettings()
{
}

void ULeeDeveloperSettings::OnPlayInEditorStarted() const
{
	// Show a notification toast to remind the user that there's an experience override set
	if (ExperienceOverride.IsValid())
	{
		// FNotificationInfo Info(FText::Format(
		// 	LOCTEXT("ExperienceOverrideActive", "Developer Settings Override\nExperience {0}"),
		// 	FText::FromName(ExperienceOverride.PrimaryAssetName)
		// ));
		// Info.ExpireDuration = 2.0f;
		// FSlateNotificationManager::Get().AddNotification(Info);
	}
}
#endif

#undef LOCTEXT_NAMESPACE

