// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class tracy : ModuleRules
{
	public tracy(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string TracyPath = Target.UEThirdPartySourceDirectory + "tracy";

		PublicSystemIncludePaths.Add(TracyPath + "/public");
		PublicSystemIncludePaths.Add(TracyPath + "/public/tracy");
	}
}