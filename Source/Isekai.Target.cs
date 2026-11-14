using UnrealBuildTool;

public class IsekaiTarget : TargetRules
{
	public IsekaiTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Isekai");
	}
}
