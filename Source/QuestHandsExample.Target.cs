// Copyright(c) 2020 Sheffer Online Services

using UnrealBuildTool;
using System.Collections.Generic;

public class QuestHandsExampleTarget : TargetRules
{
	public QuestHandsExampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "QuestHandsExample" } );
	}
}
