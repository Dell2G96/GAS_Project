using UnrealBuildTool;

// 에디터 전용 모듈: 레벨 에디터 툴바에 Slomo(배속) 버튼을 추가한다
public class DebuggerEditor : ModuleRules
{
	public DebuggerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"LevelEditor",
			}
			);
	}
}
