// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SubtickAimTrainerHUD.generated.h"

UCLASS()
class PROJECTSUBTICK_API ASubtickAimTrainerHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
