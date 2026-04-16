// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AimTrainer/AimTrainerTypes.h"

/**
 * Discovers CS2 Linux cfg paths and parses .vcfg + .cfg into a flat merged map.
 * Precedence (later wins): user_convars -> machine_convars -> autoexec.cfg
 */
class PROJECTSUBTICK_API FSubtickCs2ConfigImporter
{
public:
	static void DiscoverSteamUserdataRoots(TArray<FString>& OutRoots);
	static void FindCandidateCfgFiles(const TArray<FString>& UserdataRoots, TArray<FString>& OutFilesOrdered);
	static bool ParseAndMerge(const TArray<FString>& FilesOrdered, FSubtickCs2ImportResult& OutResult);

	/** Unit-test / automation helper: parse one file's text (vcfg or cfg). */
	static void ParseFileContent(const FString& AbsolutePath, const FString& Content, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings);

	static float GetFloatCvar(const FSubtickCs2ImportResult& Result, const TCHAR* Key, float DefaultValue);
	static FString GetStringCvar(const FSubtickCs2ImportResult& Result, const TCHAR* Key, const FString& DefaultValue);

private:
	static void ParseVcfgContentRecursive(const FString& SourceLabel, const FString& Content, int32& Index, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings);
	static void ParseCfgLines(const FString& SourceLabel, const FString& Content, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings);
	static void SkipWs(const FString& S, int32& I);
	static bool ReadQuotedString(const FString& S, int32& I, FString& Out);
	static bool ReadBareToken(const FString& S, int32& I, FString& Out);
};
