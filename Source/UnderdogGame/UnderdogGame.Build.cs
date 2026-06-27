using UnrealBuildTool;

public class UnderdogGame : ModuleRules
{
    public UnderdogGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "CommonUI",
            "UnderdogCore", "UnderdogSimulation"
        });
    }
}
