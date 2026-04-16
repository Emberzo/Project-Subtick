// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AimTrainerTypes.generated.h"

/** Single merged CS2 cvar value from import. */
USTRUCT(BlueprintType)
struct PROJECTSUBTICK_API FSubtickCs2CvarEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Key;

	UPROPERTY(BlueprintReadOnly)
	FString Value;

	UPROPERTY(BlueprintReadOnly)
	FString SourceFile;
};

/** Result of scanning + parsing CS2 config files on Linux. */
USTRUCT(BlueprintType)
struct PROJECTSUBTICK_API FSubtickCs2ImportResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FSubtickCs2CvarEntry> Entries;

	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Warnings;

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;
};

UENUM(BlueprintType)
enum class ESubtickAimDrill : uint8
{
	None UMETA(DisplayName = "None"),
	StaticClick UMETA(DisplayName = "Static Click"),
	MicroFlick UMETA(DisplayName = "Micro Flick"),
	TargetSwitch UMETA(DisplayName = "Target Switch"),
	Calibration UMETA(DisplayName = "Calibration"),
};

/** Per-session drill metrics (MVP). */
USTRUCT(BlueprintType)
struct PROJECTSUBTICK_API FSubtickDrillSessionStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Hits = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Misses = 0;

	UPROPERTY(BlueprintReadOnly)
	float TotalTimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float SumTimeToHitSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 Overshoots = 0;

	float GetHitRate() const { const int32 T = Hits + Misses; return T > 0 ? float(Hits) / float(T) : 0.f; }
	float GetAvgTimeToHit() const { return Hits > 0 ? SumTimeToHitSeconds / float(Hits) : 0.f; }
};
