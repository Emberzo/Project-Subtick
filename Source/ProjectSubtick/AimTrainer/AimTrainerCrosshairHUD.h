// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AimTrainerCrosshairHUD.generated.h"

/** Simple screen-center crosshair for the aim trainer. */
UCLASS()
class PROJECTSUBTICK_API AAimTrainerCrosshairHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Subtick|Crosshair")
	float CrosshairHalfLength = 8.f;

	UPROPERTY(EditAnywhere, Category = "Subtick|Crosshair")
	float CrosshairGap = 4.f;

	UPROPERTY(EditAnywhere, Category = "Subtick|Crosshair")
	float LineThickness = 2.f;
};
