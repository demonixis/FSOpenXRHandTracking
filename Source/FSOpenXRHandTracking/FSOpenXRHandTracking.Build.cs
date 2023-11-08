// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FSOpenXRHandTracking : ModuleRules
{
	public FSOpenXRHandTracking(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new []
		{
			"Core",
			"EnhancedInput",
			"HeadMountedDisplay",
			"XRBase",
			"XRVisualization"
		});
					
		PrivateDependencyModuleNames.AddRange(new [] {
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore"
		});
	}
}
