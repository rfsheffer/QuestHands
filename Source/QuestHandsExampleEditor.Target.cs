// Copyright(c) 2020 Sheffer Online Services

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
