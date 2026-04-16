// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/SubtickAimTrainerHUD.h"
#include "AimTrainer/SubtickAimTrainerPawn.h"
#include "AimTrainer/SubtickAimTrainerSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void ASubtickAimTrainerHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	UWorld* World = PC->GetWorld();
	USubtickAimTrainerSubsystem* Sub = nullptr;
	if (World && World->GetGameInstance())
	{
		Sub = World->GetGameInstance()->GetSubsystem<USubtickAimTrainerSubsystem>();
	}

	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	float Y = 20.f;
	const float LineStep = 18.f;

	auto DrawLine = [&](const FString& Text, const FLinearColor& Color)
	{
		DrawText(Text, Color, 20.f, Y, Font, 1.f, false);
		Y += LineStep;
	};

	DrawLine(TEXT("Project Subtick — static aim (MVP)"), FLinearColor(0.4f, 1.f, 0.5f));

	if (Sub)
	{
		DrawLine(FString::Printf(TEXT("CS2 sens: %.4f  effective: %.4f  m_yaw: %.5f"),
				Sub->GetImportedSensitivity(), Sub->GetEffectiveSensitivity(), Sub->GetYawConstant()),
			FLinearColor::White);
		DrawLine(FString::Printf(TEXT("Hipfire cm/360 (approx): %.2f @ %.0f DPI (change DPI in subsystem later)"),
				Sub->GetCmPer360Hipfire(), Sub->GetAssumedDpi()),
			FLinearColor(0.85f, 0.85f, 0.85f));
		if (Sub->GetLastImportWarnings().Num() > 0)
		{
			DrawLine(FString::Printf(TEXT("Import warnings: %d (see log)"), Sub->GetLastImportWarnings().Num()),
				FLinearColor(1.f, 0.65f, 0.2f));
		}
	}

	if (const ASubtickAimTrainerPawn* AimPawn = Cast<ASubtickAimTrainerPawn>(PC->GetPawn()))
	{
		const FSubtickDrillSessionStats& S = AimPawn->GetDrillStats();
		DrawLine(FString::Printf(TEXT("Clicks — hits: %d  misses: %d  hit%%: %.1f"),
				S.Hits, S.Misses, S.GetHitRate() * 100.f),
			FLinearColor(0.7f, 0.85f, 1.f));
		DrawLine(TEXT("LMB: shoot sphere | Mouse: yaw/pitch (CS2-style counts * sens * m_yaw)"),
			FLinearColor(0.6f, 0.6f, 0.65f));
	}
}
