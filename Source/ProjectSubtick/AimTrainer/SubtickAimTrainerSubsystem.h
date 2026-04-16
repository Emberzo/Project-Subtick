// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SubtickAimTrainerSubsystem.generated.h"

/**
 * Holds imported CS2 sensitivity values, fine-tune multiplier, and session stats for drills.
 */
UCLASS()
class PROJECTSUBTICK_API USubtickAimTrainerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Subtick|Aim")
	void TryAutoImportFromCs2Linux();

	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetImportedSensitivity() const { return ImportedSensitivity; }

	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetYawConstant() const { return YawConstant; }

	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetFineTuneMultiplier() const { return FineTuneMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Subtick|Aim")
	void SetFineTuneMultiplier(float InMultiplier);

	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetEffectiveSensitivity() const { return ImportedSensitivity * FineTuneMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetAssumedDpi() const { return AssumedDpi; }

	UFUNCTION(BlueprintCallable, Category = "Subtick|Aim")
	void SetAssumedDpi(float InDpi);

	/** cm/360 for hipfire using Source-style formula (DPI * sens * m_yaw counts per 360°). */
	UFUNCTION(BlueprintPure, Category = "Subtick|Aim")
	float GetCmPer360Hipfire() const;

	const TArray<FString>& GetLastImportWarnings() const { return LastImportWarnings; }

private:
	UPROPERTY()
	float ImportedSensitivity = 1.f;

	UPROPERTY()
	float YawConstant = 0.022f;

	UPROPERTY()
	float FineTuneMultiplier = 1.f;

	UPROPERTY()
	float AssumedDpi = 800.f;

	UPROPERTY()
	TArray<FString> LastImportWarnings;
};
