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
			"CommonInput",
			"AnimGraphRuntime",
			// GAS
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"AnimGraphRuntime",
			"MotionWarping",
			"UMG",
			
			"AIModule",
			// StateTree — Enemy 근접 공격 C++ Task(FLeeStateTreeTask_MeleeAttack)용
			"StateTreeModule",
			"GameplayStateTreeModule",
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
			//CommonUI
			"CommonUI",
			//Debugger
			"Debugger",
			
			//AsyncMixin
			"AsyncMixin",
			
			"DataRegistry",
			"ReplicationGraph",
			"SignificanceManager",
			"Hotfix",
			"GameplayMessageRuntime",
			"NetCore",
			"CommonLoadingScreen",
			"ControlFlows",

			
			
		});
		
		//PrivateIncludePaths.Add("GAS_Project");


		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraphRuntime",
			"AnimGraphRuntime",
			"CommonGame", 
			"GameplayMessageRuntime",
			"CommonLoadingScreen", "ControlFlows",
			
		});

		#region 26.09.22 추가 - StateTree 에셋 자동 저작용 에디터 전용 의존성
		// ULeeStateTreeAuthoringLibrary가 StateTreeEditorModule의 저작 API를 호출하기 위한 의존성.
		// 이 모듈들은 에디터 전용이라 bBuildEditor로 감싸지 않으면 패키징 빌드가 깨진다.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"StateTreeEditorModule",   // UStateTreeEditorData / UStateTreeState / FStateTreeCompiler
				"UnrealEd",                // UStateTreeEditingSubsystem (컴파일 + 검증 + 해시 갱신)
				"AssetRegistry",           // FAssetRegistryModule::AssetCreated
			});
		}
		#endregion

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
