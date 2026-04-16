// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AimTrainer/AimTrainerTypes.h"
#include "AimTrainerGameMode.generated.h"

class AAimTrainerTarget;

/**
 * Aim trainer flow: calibration + three MVP drills, abstract sphere targets.
 */
UCLASS(Config = Engine)
class PROJECTSUBTICK_API AAimTrainerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAimTrainerGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void SetDrill(ESubtickAimDrill NewDrill);

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void RegisterTargetHit(AAimTrainerTarget* Target);

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void RegisterMiss();

	/** Mark start/end of a physical 360° calibration segment (toggle). */
	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void CalibrationToggleSegment();

	UFUNCTION(BlueprintPure, Category = "Subtick")
	ESubtickAimDrill GetCurrentDrill() const { return CurrentDrill; }

	UFUNCTION(BlueprintPure, Category = "Subtick")
	const FSubtickDrillSessionStats& GetSessionStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "Subtick")
	float GetLastSuggestedFineTune() const { return LastSuggestedFineTune; }

protected:
	/**
	 * Optional pawn class (e.g. Blueprint with Gun_USP). If unset, uses AAimTrainerCharacter.
	 * Set in DefaultEngine.ini: [/Script/ProjectSubtick.AimTrainerGameMode] AimTrainerDefaultPawnClass=/Game/YourPath/usp.usp_C
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Subtick")
	TSubclassOf<APawn> AimTrainerDefaultPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Subtick")
	TSubclassOf<AAimTrainerTarget> TargetClass;

	UPROPERTY(EditAnywhere, Category = "Subtick")
	float TargetDistance = 900.f;

	UPROPERTY()
	TObjectPtr<AAimTrainerTarget> ActiveTarget;

	UPROPERTY()
	TArray<TObjectPtr<AAimTrainerTarget>> SwitchTargets;

	ESubtickAimDrill CurrentDrill = ESubtickAimDrill::StaticClick;
	FSubtickDrillSessionStats Stats;
	float DrillTimeAccumulator = 0.f;
	float LastSpawnWorldTime = 0.f;
	int32 SwitchCursor = 0;
	float LastControlYaw = 0.f;
	bool bCalibrationRecording = false;
	float CalibrationYawThisSegment = 0.f;
	TArray<float> CalibrationMeasuredCmSamples;
	float LastSuggestedFineTune = 1.f;

	void ClearTargets();
	void SpawnStaticOrMicroTarget(bool bMicro);
	void SetupTargetSwitch();
	void AdvanceSwitchHighlight();
	FVector ViewDir(APawn* Pawn) const;
	void SpawnOrbAt(APawn* Pawn, const FVector& WorldLocation, TObjectPtr<AAimTrainerTarget>& Slot);
};
