#include "LeeGameplayTags.h"

#include "Runtime/GameplayTags/Classes/GameplayTagsManager.h"


void FLeeGameplayTags::InitializeNativeTags()
{
	// UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	// GameplayTags.AddAllTags(Manager);
	
}

void AddTag(class FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment)
{
	// OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TagName), FName(TagComment));
}

void FLeeGameplayTags::AddAllTags(class UGameplayTagsManager& Manager)
{
	
}
