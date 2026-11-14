using UnrealBuildTool;

public class IsekaiEditorTarget : TargetRules
{
	public IsekaiEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Isekai");
	}
}
