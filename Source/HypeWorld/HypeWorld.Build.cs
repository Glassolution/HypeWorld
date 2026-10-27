// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HypeWorld : ModuleRules
{
	public HypeWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimationCore", "SlateCore", "Landscape", "AnimationWarpingRuntime", "AnimGraphRuntime" });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");   // ferramentas de editor (NinjaEditorTools: Blocking Volume)
			PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraph", "BlueprintGraph", "AssetTools", "NiagaraEditor" });   // NinjaEditorTools: Anim Blueprint de retarget
		}

		PublicIncludePaths.AddRange(new string[] {
			"HypeWorld",
			"HypeWorld/Ninja",
			"HypeWorld/UI",
			"HypeWorld/Combat",
			"HypeWorld/Editor",
			"HypeWorld/Variant_Platforming",
			"HypeWorld/Variant_Platforming/Animation",
			"HypeWorld/Variant_Combat",
			"HypeWorld/Variant_Combat/AI",
			"HypeWorld/Variant_Combat/Animation",
			"HypeWorld/Variant_Combat/Gameplay",
			"HypeWorld/Variant_Combat/Interfaces",
			"HypeWorld/Variant_Combat/UI",
			"HypeWorld/Variant_SideScrolling",
			"HypeWorld/Variant_SideScrolling/AI",
			"HypeWorld/Variant_SideScrolling/Gameplay",
			"HypeWorld/Variant_SideScrolling/Interfaces",
			"HypeWorld/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
