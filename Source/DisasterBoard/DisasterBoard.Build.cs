using UnrealBuildTool;

public class DisasterBoard : ModuleRules
{
	public DisasterBoard(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
		// SlateCore: HUD'da fontu dosyadan kurmak icin (FFontData; JamKit 04 §5)
		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore" });
	}
}
