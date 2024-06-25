// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FSOpenXRHandTracking : ModuleRules
{
	private const bool bMetaXREnabled = false;
	
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

		bool bMetaXRSupported = (Target.Platform == UnrealTargetPlatform.Android ||
		                         Target.Platform == UnrealTargetPlatform.Win64) &&
		                        bMetaXREnabled;

		if (bMetaXRSupported)
		{
			PrivateDependencyModuleNames.Add("OculusXRInput");
		}
		
		PublicDefinitions.Add($"WITH_METAXR={(bMetaXRSupported ? 1 : 0)}");
	}
}
