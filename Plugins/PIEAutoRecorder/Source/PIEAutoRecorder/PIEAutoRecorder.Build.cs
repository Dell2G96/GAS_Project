using UnrealBuildTool;

public class PIEAutoRecorder : ModuleRules
{
	public PIEAutoRecorder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",       // 설정 UObject
			"Engine",            // 기본 엔진 타입
			"UnrealEd",          // FEditorDelegates
			"DeveloperSettings", // UDeveloperSettings
			"WebSockets",        // IWebSocket
			"Json",              // FJsonSerializer
			"Slate",             // 저장 확인 창, 알림
			"SlateCore",
			"DesktopPlatform",   // 폴더 선택 다이얼로그
			"OpenSSL",           // SHA256
		});
	}
}
