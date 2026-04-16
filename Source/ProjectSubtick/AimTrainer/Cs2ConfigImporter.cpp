// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/Cs2ConfigImporter.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

void FSubtickCs2ConfigImporter::DiscoverSteamUserdataRoots(TArray<FString>& OutRoots)
{
	const FString Home = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	if (Home.IsEmpty())
	{
		return;
	}
	OutRoots.Add(FPaths::Combine(Home, TEXT(".local/share/Steam/userdata")));
	OutRoots.Add(FPaths::Combine(Home, TEXT(".var/app/com.valvesoftware.Steam/.local/share/Steam/userdata")));
}

void FSubtickCs2ConfigImporter::FindCandidateCfgFiles(const TArray<FString>& UserdataRoots, TArray<FString>& OutFilesOrdered)
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> UserConvars;
	TArray<FString> MachineConvars;
	TArray<FString> AutoExecGame;

	for (const FString& Root : UserdataRoots)
	{
		if (!PF.DirectoryExists(*Root))
		{
			continue;
		}
		TArray<FString> SteamIdNames;
		IFileManager::Get().FindFiles(SteamIdNames, *Root, false, true);
		for (const FString& Name : SteamIdNames)
		{
			const FString SteamIdDir = FPaths::Combine(Root, Name);
			const FString CfgDir = FPaths::Combine(SteamIdDir, TEXT("730/local/cfg"));
			const FString UserFile = FPaths::Combine(CfgDir, TEXT("cs2_user_convars_0_slot0.vcfg"));
			const FString MachineFile = FPaths::Combine(CfgDir, TEXT("cs2_machine_convars.vcfg"));
			if (PF.FileExists(*UserFile))
			{
				UserConvars.Add(UserFile);
			}
			if (PF.FileExists(*MachineFile))
			{
				MachineConvars.Add(MachineFile);
			}
		}
	}

	const FString Home = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	if (!Home.IsEmpty())
	{
		const FString NativeAuto = FPaths::Combine(Home, TEXT(".local/share/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/cfg/autoexec.cfg"));
		const FString FlatpakAuto = FPaths::Combine(Home, TEXT(".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/cfg/autoexec.cfg"));
		if (PF.FileExists(*NativeAuto))
		{
			AutoExecGame.Add(NativeAuto);
		}
		if (PF.FileExists(*FlatpakAuto))
		{
			AutoExecGame.Add(FlatpakAuto);
		}
	}

	// Merge order: all user_convars (sorted for stability), then machine, then autoexec (later wins in map — we apply in array order with overwrite)
	UserConvars.Sort();
	MachineConvars.Sort();
	AutoExecGame.Sort();

	for (const FString& F : UserConvars)
	{
		OutFilesOrdered.Add(F);
	}
	for (const FString& F : MachineConvars)
	{
		OutFilesOrdered.Add(F);
	}
	for (const FString& F : AutoExecGame)
	{
		OutFilesOrdered.Add(F);
	}
}

void FSubtickCs2ConfigImporter::SkipWs(const FString& S, int32& I)
{
	while (I < S.Len() && FChar::IsWhitespace(S[I]))
	{
		++I;
	}
}

bool FSubtickCs2ConfigImporter::ReadQuotedString(const FString& S, int32& I, FString& Out)
{
	SkipWs(S, I);
	if (I >= S.Len() || S[I] != TEXT('"'))
	{
		return false;
	}
	++I;
	int32 Start = I;
	while (I < S.Len() && S[I] != TEXT('"'))
	{
		if (S[I] == TEXT('\\') && I + 1 < S.Len())
		{
			I += 2;
			continue;
		}
		++I;
	}
	Out = S.Mid(Start, I - Start);
	if (I < S.Len() && S[I] == TEXT('"'))
	{
		++I;
	}
	return true;
}

bool FSubtickCs2ConfigImporter::ReadBareToken(const FString& S, int32& I, FString& Out)
{
	SkipWs(S, I);
	int32 Start = I;
	while (I < S.Len() && !FChar::IsWhitespace(S[I]) && S[I] != TEXT('{') && S[I] != TEXT('}'))
	{
		++I;
	}
	Out = S.Mid(Start, I - Start);
	return Out.Len() > 0;
}

void FSubtickCs2ConfigImporter::ParseVcfgContentRecursive(const FString& SourceLabel, const FString& Content, int32& Index, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings)
{
	while (Index < Content.Len())
	{
		SkipWs(Content, Index);
		if (Index >= Content.Len())
		{
			break;
		}
		const TCHAR C = Content[Index];
		if (C == TEXT('}'))
		{
			++Index;
			return;
		}
		FString Key;
		if (Content[Index] == TEXT('"'))
		{
			if (!ReadQuotedString(Content, Index, Key))
			{
				Warnings.Add(FString::Printf(TEXT("%s: bad string at %d"), *SourceLabel, Index));
				++Index;
				continue;
			}
		}
		else
		{
			if (!ReadBareToken(Content, Index, Key))
			{
				++Index;
				continue;
			}
		}
		SkipWs(Content, Index);
		if (Index < Content.Len() && Content[Index] == TEXT('{'))
		{
			++Index;
			ParseVcfgContentRecursive(SourceLabel, Content, Index, InOutFlat, Warnings);
			continue;
		}
		FString Value;
		if (Index < Content.Len() && Content[Index] == TEXT('"'))
		{
			ReadQuotedString(Content, Index, Value);
		}
		else
		{
			ReadBareToken(Content, Index, Value);
		}
		if (!Key.IsEmpty())
		{
			InOutFlat.Add(Key, Value);
		}
	}
}

void FSubtickCs2ConfigImporter::ParseCfgLines(const FString& SourceLabel, const FString& Content, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings)
{
	TArray<FString> Lines;
	Content.ParseIntoArrayLines(Lines, false);
	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("//")))
		{
			continue;
		}
		// strip inline // comment
		int32 Comment = INDEX_NONE;
		if (Line.FindChar(TEXT('/'), Comment) && Comment + 1 < Line.Len() && Line[Comment + 1] == TEXT('/'))
		{
			Line.LeftInline(Comment, EAllowShrinking::No);
			Line.TrimEndInline();
		}
		if (Line.StartsWith(TEXT("bind")))
		{
			continue;
		}
		TArray<FString> Parts;
		Line.ParseIntoArrayWS(Parts);
		if (Parts.Num() < 2)
		{
			continue;
		}
		const FString Cmd = Parts[0].ToLower();
		if (Cmd == TEXT("exec") || Cmd == TEXT("alias"))
		{
			continue;
		}
		FString Key;
		FString Value;
		if (Cmd == TEXT("set") || Cmd == TEXT("seta") || Cmd == TEXT("setl"))
		{
			if (Parts.Num() < 3)
			{
				continue;
			}
			Key = Parts[1];
			Value = Parts[2];
			for (int32 i = 3; i < Parts.Num(); ++i)
			{
				Value += TEXT(" ") + Parts[i];
			}
		}
		else
		{
			Key = Parts[0];
			Value = Parts[1];
			for (int32 i = 2; i < Parts.Num(); ++i)
			{
				Value += TEXT(" ") + Parts[i];
			}
		}
		Value.TrimStartAndEndInline();
		if (Value.Len() >= 2 && Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")))
		{
			Value = Value.Mid(1, Value.Len() - 2);
		}
		if (!Key.IsEmpty())
		{
			InOutFlat.Add(Key, Value);
		}
	}
}

void FSubtickCs2ConfigImporter::ParseFileContent(const FString& AbsolutePath, const FString& Content, TMap<FString, FString>& InOutFlat, TArray<FString>& Warnings)
{
	const bool bVcfg = AbsolutePath.EndsWith(TEXT(".vcfg"), ESearchCase::IgnoreCase);
	if (bVcfg)
	{
		int32 Idx = 0;
		ParseVcfgContentRecursive(AbsolutePath, Content, Idx, InOutFlat, Warnings);
	}
	else
	{
		ParseCfgLines(AbsolutePath, Content, InOutFlat, Warnings);
	}
}

bool FSubtickCs2ConfigImporter::ParseAndMerge(const TArray<FString>& FilesOrdered, FSubtickCs2ImportResult& OutResult)
{
	TMap<FString, FString> Flat;
	TMap<FString, FString> SourceForKey;
	for (const FString& Path : FilesOrdered)
	{
		FString Data;
		if (!FFileHelper::LoadFileToString(Data, *Path))
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("Could not read: %s"), *Path));
			continue;
		}
		TMap<FString, FString> ThisFileFlat;
		ParseFileContent(Path, Data, ThisFileFlat, OutResult.Warnings);
		for (const TPair<FString, FString>& P : ThisFileFlat)
		{
			Flat.Add(P.Key, P.Value);
			SourceForKey.Add(P.Key, Path);
		}
	}
	OutResult.Entries.Reserve(Flat.Num());
	for (const TPair<FString, FString>& P : Flat)
	{
		FSubtickCs2CvarEntry E;
		E.Key = P.Key;
		E.Value = P.Value;
		E.SourceFile = SourceForKey.FindRef(P.Key);
		if (E.SourceFile.IsEmpty())
		{
			E.SourceFile = TEXT("(unknown)");
		}
		OutResult.Entries.Add(E);
	}
	OutResult.bSuccess = true;
	return true;
}

float FSubtickCs2ConfigImporter::GetFloatCvar(const FSubtickCs2ImportResult& Result, const TCHAR* Key, float DefaultValue)
{
	for (const FSubtickCs2CvarEntry& E : Result.Entries)
	{
		if (E.Key.Equals(Key, ESearchCase::IgnoreCase))
		{
			return FCString::Atof(*E.Value);
		}
	}
	return DefaultValue;
}

FString FSubtickCs2ConfigImporter::GetStringCvar(const FSubtickCs2ImportResult& Result, const TCHAR* Key, const FString& DefaultValue)
{
	for (const FSubtickCs2CvarEntry& E : Result.Entries)
	{
		if (E.Key.Equals(Key, ESearchCase::IgnoreCase))
		{
			return E.Value;
		}
	}
	return DefaultValue;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtickCs2ParseVcfg, "ProjectSubtick.Cs2.ParseVcfgBasic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSubtickCs2ParseVcfg::RunTest(const FString& Parameters)
{
	const FString Sample = TEXT(
		"\"config\"\n"
		"{\n"
		"  \"cvars\"\n"
		"  {\n"
		"    \"sensitivity\" \"2.5\"\n"
		"    \"m_yaw\" \"0.022\"\n"
		"  }\n"
		"}\n");
	TMap<FString, FString> Flat;
	TArray<FString> Warn;
	FSubtickCs2ConfigImporter::ParseFileContent(TEXT("/tmp/SubtickUnitTest.vcfg"), Sample, Flat, Warn);
	TestEqual(TEXT("sensitivity"), Flat.FindRef(TEXT("sensitivity")), FString(TEXT("2.5")));
	TestEqual(TEXT("m_yaw"), Flat.FindRef(TEXT("m_yaw")), FString(TEXT("0.022")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtickCs2ParseCfg, "ProjectSubtick.Cs2.ParseCfgSet", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSubtickCs2ParseCfg::RunTest(const FString& Parameters)
{
	const FString Sample = TEXT("sensitivity 1.25\nset zoom_sensitivity_ratio_mouse 1.0\n// comment\n");
	TMap<FString, FString> Flat;
	TArray<FString> Warn;
	FSubtickCs2ConfigImporter::ParseFileContent(TEXT("/tmp/SubtickUnitTest.cfg"), Sample, Flat, Warn);
	TestEqual(TEXT("sensitivity"), Flat.FindRef(TEXT("sensitivity")), FString(TEXT("1.25")));
	TestEqual(TEXT("zoom_sensitivity_ratio_mouse"), Flat.FindRef(TEXT("zoom_sensitivity_ratio_mouse")), FString(TEXT("1.0")));
	return true;
}
#endif
