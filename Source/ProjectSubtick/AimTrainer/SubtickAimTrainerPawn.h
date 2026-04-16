// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AimTrainer/AimTrainerTypes.h"
#include "SubtickAimTrainerPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Static aim trainer pawn: no movement, CS2-style yaw/pitch from mouse counts * sens * m_yaw.
 */
UCLASS()
class PROJECTSUBTICK_API ASubtickAimTrainerPawn : public APawn
{
	GENERATED_BODY()

public:
	ASubtickAimTrainerPawn();

	const FSubtickDrillSessionStats& GetDrillStats() const { return LocalStats; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void AxisYaw(float Value);
	void AxisPitch(float Value);
	void PrimaryFire();

	void ApplyLookDelta(float RawX, float RawY);

	UPROPERTY(VisibleAnywhere, Category = "Subtick|Aim")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, Category = "Subtick|Aim")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Subtick|Aim")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Subtick|Aim")
	float YawDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Subtick|Aim")
	float PitchDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Subtick|Aim", meta = (AllowPrivateAccess = "true"))
	FSubtickDrillSessionStats LocalStats;
};
