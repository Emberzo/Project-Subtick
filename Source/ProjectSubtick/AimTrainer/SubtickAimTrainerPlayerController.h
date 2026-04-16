// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SubtickAimTrainerPlayerController.generated.h"

UCLASS()
class PROJECTSUBTICK_API ASubtickAimTrainerPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
