// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GAS_Project : ModuleRules
{
	public GAS_Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreOnline",
			"CoreUObject",
			"ApplicationCore",

			"Engine",
			"PhysicsCore",

			// Input
			"InputCore",
			"EnhancedInput",
			"AnimGraphRuntime",
			// GAS
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"AnimGraphRuntime",
			"MotionWarping",
			"UMG",
			
			"AIModule",
			"SlateCore",
			"Slate",
			"Niagara",
			"NavigationSystem",
			"ModularGameplay",
			"GameFeatures",
			//"ModularGameplayActors",
			"ModularGamePlayActors",
			//commonUser
			"CommonUser",
			// CommonUI
			"CommonUI",
			// CommonGame
			"CommonGame",
			//UIExtension
			"UIExtension",
			//SoulsCore
			
			"DataRegistry",
			"ReplicationGraph",
			"SignificanceManager",
			"Hotfix",
			"GameplayMessageRuntime",
			"NetCore",



			
		});
		
		//PrivateIncludePaths.Add("GAS_Project");


		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime", "AnimGraphRuntime", "CommonGame", "GameplayMessageRuntime" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
