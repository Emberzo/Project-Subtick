// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/SubtickAimTrainerPlayerController.h"

void ASubtickAimTrainerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}
