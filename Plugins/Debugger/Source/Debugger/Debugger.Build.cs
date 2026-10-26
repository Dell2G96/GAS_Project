// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Debugger : ModuleRules
{
	public Debugger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayTags",
				// ... add private dependencies that you statically link with here ...
			}
			);

		// GameplayDebugger 모듈 의존성 + WITH_GAMEPLAY_DEBUGGER 매크로 정의 (Shipping 등에서는 자동으로 0)
		SetupGameplayDebuggerSupport(Target);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
