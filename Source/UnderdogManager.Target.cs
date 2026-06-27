using UnrealBuildTool;

public class UnderdogManagerTarget : TargetRules
{
    public UnderdogManagerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UnderdogGame");
    }
}
