// Copyright Big Brother AI Simulator. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class BigBrotherSimTarget : TargetRules
{
	public BigBrotherSimTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("BigBrotherSim");
	}
}
