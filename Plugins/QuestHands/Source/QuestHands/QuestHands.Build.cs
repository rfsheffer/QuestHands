// Copyright(c) 2020 Sheffer Online Services

using UnrealBuildTool;
using System.IO;
using System;

public class QuestHands : ModuleRules
{
	public QuestHands(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		//OptimizeCode = CodeOptimization.Never;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
			});
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"OVRPlugin",
				"OculusHMD",
				"OculusInput",
				"InputDevice",
			});

		PrivateIncludePaths.AddRange(
				new string[] {
					// Oculus's naughty little hack in the plugin... Access the private headers.
					Path.GetFullPath(Path.Combine(EngineDirectory, "Plugins/Runtime/Oculus/OculusVR/Source/OculusHMD/Private")),
				});

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "QuestHands_APL.xml"));
		}
	}
}
