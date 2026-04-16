// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/SubtickAimTrainerSubsystem.h"
#include "AimTrainer/Cs2ConfigImporter.h"

void USubtickAimTrainerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TryAutoImportFromCs2Linux();
}

void USubtickAimTrainerSubsystem::TryAutoImportFromCs2Linux()
{
	LastImportWarnings.Reset();
	FSubtickCs2ImportResult Result;

	TArray<FString> UserdataRoots;
	FSubtickCs2ConfigImporter::DiscoverSteamUserdataRoots(UserdataRoots);
	TArray<FString> Files;
	FSubtickCs2ConfigImporter::FindCandidateCfgFiles(UserdataRoots, Files);

	if (Files.Num() == 0)
	{
		LastImportWarnings.Add(TEXT("No CS2 cfg files found under Steam userdata / game paths."));
		ImportedSensitivity = 1.f;
		YawConstant = 0.022f;
		return;
	}

	FSubtickCs2ConfigImporter::ParseAndMerge(Files, Result);
	LastImportWarnings = Result.Warnings;

	ImportedSensitivity = FSubtickCs2ConfigImporter::GetFloatCvar(Result, TEXT("sensitivity"), 1.f);

	float Myaw = FSubtickCs2ConfigImporter::GetFloatCvar(Result, TEXT("m_yaw"), 0.022f);
	if (Myaw <= 0.f)
	{
		LastImportWarnings.Add(TEXT("Invalid m_yaw; using 0.022."));
		Myaw = 0.022f;
	}
	YawConstant = Myaw;

	// zoom ratio stored for future scoped drills; not used in hipfire MVP
	(void)FSubtickCs2ConfigImporter::GetFloatCvar(Result, TEXT("zoom_sensitivity_ratio_mouse"),
		FSubtickCs2ConfigImporter::GetFloatCvar(Result, TEXT("zoom_sensitivity_ratio"), 1.f));
}

void USubtickAimTrainerSubsystem::SetFineTuneMultiplier(float InMultiplier)
{
	FineTuneMultiplier = FMath::Clamp(InMultiplier, 0.5f, 2.f);
}

void USubtickAimTrainerSubsystem::SetAssumedDpi(float InDpi)
{
	AssumedDpi = FMath::Max(1.f, InDpi);
}

float USubtickAimTrainerSubsystem::GetCmPer360Hipfire() const
{
	const float Sens = GetEffectiveSensitivity();
	const float Product = AssumedDpi * Sens * YawConstant;
	if (Product <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return (2.54f * 360.f) / Product;
}
