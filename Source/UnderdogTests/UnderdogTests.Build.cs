using UnrealBuildTool;

public class UnderdogTests : ModuleRules
{
    public UnderdogTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnderdogCore", "UnderdogSimulation", "UnderdogGame"
        });
    }
}
