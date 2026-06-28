using UnrealBuildTool;

public class UnderdogManagerTarget : TargetRules
{
    public UnderdogManagerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UnderdogGame");
    }
}
