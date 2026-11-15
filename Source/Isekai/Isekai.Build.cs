using UnrealBuildTool;

public class Isekai : ModuleRules
{
	public Isekai(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 폴더를 기준으로 "Player/IsekaiCharacter.h"처럼 include할 수 있게 한다.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Slate",
			"SlateCore"
		});

		// 에디터에서만 쓰는 기능: 소리 파일 자동 가져오기.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools"
			});
		}
	}
}
