// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/SubtickAimTrainerGameMode.h"
#include "AimTrainer/SubtickAimTrainerHUD.h"
#include "AimTrainer/SubtickAimTrainerPawn.h"
#include "AimTrainer/SubtickAimTrainerPlayerController.h"

ASubtickAimTrainerGameMode::ASubtickAimTrainerGameMode()
{
	DefaultPawnClass = ASubtickAimTrainerPawn::StaticClass();
	PlayerControllerClass = ASubtickAimTrainerPlayerController::StaticClass();
	HUDClass = ASubtickAimTrainerHUD::StaticClass();
}
