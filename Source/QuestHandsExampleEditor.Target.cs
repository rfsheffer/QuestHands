// Copyright 2020 Sheffer Online Services. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class QuestHandsExampleEditorTarget : TargetRules
{
	public QuestHandsExampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "QuestHandsExample" } );
	}
}
