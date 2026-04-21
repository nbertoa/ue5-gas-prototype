using UnrealBuildTool;

public class GASPrototype : ModuleRules
{
	public GASPrototype(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public dependencies: available to both this module's headers and consumers.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput", // Enhanced Input System (Input Actions, Mapping Contexts)
			"GameplayAbilities", // Gameplay Ability System (ASC, GEs, Abilities)
			"GameplayTasks", // Async tasks used by GAS (WaitTargetData, PlayMontageAndWait)
			"GameplayTags", // Native Gameplay Tags (GPTags.h)
			"UMG" // Slate/UMG widgets (AttributeWidget, WidgetComponent)
		});

		// Private dependencies: only needed by .cpp files, not exposed in headers.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule" // AAIController for enemy death logic (StopMovement)
		});
	}
}