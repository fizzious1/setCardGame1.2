// Copyright Big Brother AI Simulator. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BigBrotherSimEditorTarget : TargetRules
{
	public BigBrotherSimEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("BigBrotherSim");
	}
}
