#pragma once

#include "CoreMinimal.h"
#include "Textures/SlateIcon.h"


class FDell2gPluginStyle
{
public:
	static void Regist();
	static TSharedPtr<FDell2gPluginStyle> Get();
	static void Unregist();
	
private:
	static TSharedPtr<FDell2gPluginStyle> Instance;
	
public:
	FDell2gPluginStyle();
	~FDell2gPluginStyle();
	
private:
	void RegisterIcon(const FString& InName , const FString& InPath, const FVector2D& InInconSize, FSlateIcon& OutSlateIcon);
	
private:
	static const FName StyleSetName;
	
private:
	TSharedPtr<class FSlateStyleSet> StyleSet;
	
public:
	FSlateIcon Slomo_Up;
	FSlateIcon Slomo_Down;
	FSlateIcon Slomo_0_1;
	FSlateIcon Slomo_1_0;
	
};
