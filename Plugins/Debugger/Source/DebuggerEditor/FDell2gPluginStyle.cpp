#include "FDell2gPluginStyle.h"

#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

const FName FDell2gPluginStyle::StyleSetName = "Dell2gStyle";
TSharedPtr<FDell2gPluginStyle> FDell2gPluginStyle::Instance = nullptr;

void FDell2gPluginStyle::Regist()
{
	Instance = MakeShareable(new FDell2gPluginStyle);
}

TSharedPtr<FDell2gPluginStyle> FDell2gPluginStyle::Get()
{
	ensure(Instance != nullptr);
	
	return Instance;
}

void FDell2gPluginStyle::Unregist()
{
	if(Instance.IsValid())
		Instance.Reset();
}

FDell2gPluginStyle::FDell2gPluginStyle()
{
	StyleSet = MakeShareable(new FSlateStyleSet(StyleSetName));
	
	FString path = FPaths::ProjectPluginsDir() / "Debugger" / "Resources";
	StyleSet->SetContentRoot(path);
	
	RegisterIcon("Slomo_Up", path / "Icon_Slomo_Up.png", FVector2d(40,40),Slomo_Up);
	RegisterIcon("Slomo_Down", path / "Icon_Slomo_Down.png",FVector2d(40,40),Slomo_Down);
	RegisterIcon("Slomo_0_1", path / "Icon_Slomo_0_1.png",FVector2d(40,40),Slomo_0_1);
	RegisterIcon("Slomo_1_0", path / "Icon_Slomo_1_0.png",FVector2d(40,40),Slomo_1_0);
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

FDell2gPluginStyle::~FDell2gPluginStyle()
{
	if (StyleSet.IsValid() == false)
		return;
	
	FSlateStyleRegistry::UnRegisterSlateStyle(StyleSetName);
	StyleSet.Reset();
}

void FDell2gPluginStyle::RegisterIcon(const FString& InName, const FString& InPath, const FVector2D& InInconSize,
	FSlateIcon& OutSlateIcon)
{
	FSlateImageBrush* brush = new FSlateImageBrush(InPath, InInconSize);
	
	FString name = StyleSetName.ToString() + "." + InName;
	StyleSet->Set(FName(name), brush);
	
	OutSlateIcon = FSlateIcon(StyleSetName, FName(name));
}













