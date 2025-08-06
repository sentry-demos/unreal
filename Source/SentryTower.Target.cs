// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SentryTowerTarget : TargetRules
{
	public SentryTowerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("SentryTower");

		MacPlatform.bUseDSYMFiles = true;
		IOSPlatform.bGeneratedSYM = true;
	}
}
