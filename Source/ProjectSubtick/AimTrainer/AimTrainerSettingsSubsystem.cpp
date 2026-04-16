// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerSettingsSubsystem.h"
#include "AimTrainer/Cs2ConfigImporter.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"

void UAimTrainerSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterConsoleCommands();
}

void UAimTrainerSettingsSubsystem::Deinitialize()
{
	if (ImportCmd)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ImportCmd, false);
		ImportCmd = nullptr;
	}
	Super::Deinitialize();
}

void UAimTrainerSettingsSubsystem::RegisterConsoleCommands()
{
	ImportCmd = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("subtick.ImportCs2"),
		TEXT("Merge CS2 Linux cfg files from default Steam paths and apply cvars"),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			ImportFromDefaultSteamPaths();
			ApplyImportedCvarsToRuntime();
		}),
		ECVF_Default);
}

void UAimTrainerSettingsSubsystem::ImportFromDefaultSteamPaths()
{
	TArray<FString> Roots;
	FSubtickCs2ConfigImporter::DiscoverSteamUserdataRoots(Roots);
	TArray<FString> Files;
	FSubtickCs2ConfigImporter::FindCandidateCfgFiles(Roots, Files);
	LastImport = FSubtickCs2ImportResult();
	if (Files.Num() == 0)
	{
		LastImport.bSuccess = false;
		LastImport.Warnings.Add(TEXT("No CS2 cfg files found under default Steam userdata paths."));
		return;
	}
	FSubtickCs2ConfigImporter::ParseAndMerge(Files, LastImport);
}

void UAimTrainerSettingsSubsystem::ApplyImportedCvarsToRuntime()
{
	if (!LastImport.bSuccess && LastImport.Entries.Num() == 0)
	{
		ImportFromDefaultSteamPaths();
	}
	const float Sens = FSubtickCs2ConfigImporter::GetFloatCvar(LastImport, TEXT("sensitivity"), Sensitivity);
	const float Yaw = FSubtickCs2ConfigImporter::GetFloatCvar(LastImport, TEXT("m_yaw"), MYaw);
	const float Pitch = FSubtickCs2ConfigImporter::GetFloatCvar(LastImport, TEXT("m_pitch"), MPitch);
	const float Zoom = FSubtickCs2ConfigImporter::GetFloatCvar(LastImport, TEXT("zoom_sensitivity_ratio_mouse"), ZoomSensitivityRatio);
	Sensitivity = Sens > 0.f ? Sens : Sensitivity;
	MYaw = Yaw > 0.f ? Yaw : MYaw;
	MPitch = Pitch > 0.f ? Pitch : MPitch;
	ZoomSensitivityRatio = Zoom > 0.f ? Zoom : ZoomSensitivityRatio;
}

float UAimTrainerSettingsSubsystem::GetExpectedCmPer360() const
{
	const float Dpi = FMath::Max(MouseDpi, 1.f);
	const float Sens = FMath::Max(GetEffectiveSensitivity(), 0.0001f);
	const float Yaw = FMath::Max(MYaw, 0.00001f);
	return (2.54f * 360.f) / (Dpi * Sens * Yaw);
}

float UAimTrainerSettingsSubsystem::GetSuggestedFineTuneForMeasuredCm360(float MeasuredCm360) const
{
	const float Expected = GetExpectedCmPer360();
	if (MeasuredCm360 <= KINDA_SMALL_NUMBER || Expected <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	return Expected / MeasuredCm360;
}

float UAimTrainerSettingsSubsystem::GetCalibrationErrorPercent(float MeasuredCm360) const
{
	const float Expected = GetExpectedCmPer360();
	if (Expected <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Abs(MeasuredCm360 - Expected) / Expected * 100.f;
}
