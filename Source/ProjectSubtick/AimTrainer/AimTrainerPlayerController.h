// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSubtickPlayerController.h"
#include "AimTrainerPlayerController.generated.h"

class AAimTrainerGameMode;

/**
 * Aim trainer PC: center ray hits, debug HUD, drill hotkeys.
 */
UCLASS()
class PROJECTSUBTICK_API AAimTrainerPlayerController : public AProjectSubtickPlayerController
{
	GENERATED_BODY()

public:
	AAimTrainerPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	void PrimaryFire();
	void KeyOne();
	void KeyTwo();
	void KeyThree();
	void KeyCalibrateToggle();

	void TraceCenterView();
	void ToggleSettingsHud();

	/** Mouse look via legacy axis (IMC is often unset on aim-trainer-only PCs). */
	void OnMouseTurn(float Value);
	void OnMouseLookUp(float Value);

	float HudAccum = 0.f;
	bool bSettingsHud = false;
};
