using UnrealBuildTool;

public class UnderdogSimulation : ModuleRules
{
    public UnderdogSimulation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "UnderdogCore" });
    }
}
