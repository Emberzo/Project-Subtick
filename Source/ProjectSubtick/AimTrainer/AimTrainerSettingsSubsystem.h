// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AimTrainer/AimTrainerTypes.h"
#include "AimTrainerSettingsSubsystem.generated.h"

class IConsoleObject;

/**
 * Persistent CS2-parity aim settings + last import. GameInstance-scoped.
 */
UCLASS()
class PROJECTSUBTICK_API UAimTrainerSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Mouse DPI used for cm/360 display (user-entered). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity")
	float MouseDpi = 800.f;

	/** Imported or manual CS2 sensitivity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity")
	float Sensitivity = 1.f;

	/** CS2 m_yaw default 0.022 (degrees per count * sens scaling in our pipeline). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity")
	float MYaw = 0.022f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity")
	float MPitch = 0.022f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity")
	float ZoomSensitivityRatio = 1.f;

	/** User fine-tune after calibration (multiply imported sens). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick|Sensitivity", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float FineTuneMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Subtick|Import")
	FSubtickCs2ImportResult LastImport;

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void ImportFromDefaultSteamPaths();

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void ApplyImportedCvarsToRuntime();

	UFUNCTION(BlueprintPure, Category = "Subtick")
	float GetEffectiveSensitivity() const { return Sensitivity * FineTuneMultiplier; }

	/** cm/360 from current DPI/sens/yaw (CS-style formula). */
	UFUNCTION(BlueprintPure, Category = "Subtick")
	float GetExpectedCmPer360() const;

	/** Suggested fine-tune given a measured cm/360 from calibration turns. */
	UFUNCTION(BlueprintPure, Category = "Subtick")
	float GetSuggestedFineTuneForMeasuredCm360(float MeasuredCm360) const;

	UFUNCTION(BlueprintPure, Category = "Subtick")
	float GetCalibrationErrorPercent(float MeasuredCm360) const;

protected:
	void RegisterConsoleCommands();
	IConsoleObject* ImportCmd = nullptr;
};
