using UnrealBuildTool;

public class UnderdogManagerEditorTarget : TargetRules
{
    public UnderdogManagerEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "UnderdogGame", "UnderdogTests" });
    }
}
