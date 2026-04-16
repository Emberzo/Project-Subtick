// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerCrosshairHUD.h"
#include "Engine/Canvas.h"

void AAimTrainerCrosshairHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const float L = CrosshairHalfLength;
	const float G = CrosshairGap * 0.5f;
	const float T = LineThickness;

	const FLinearColor Outline(0.f, 0.f, 0.f, 0.65f);
	const FLinearColor Fill(1.f, 1.f, 1.f, 0.92f);

	auto DrawCross = [&](const FLinearColor& Color, float Thickness)
	{
		DrawLine(CX - L - G, CY, CX - G, CY, Color, Thickness);
		DrawLine(CX + G, CY, CX + L + G, CY, Color, Thickness);
		DrawLine(CX, CY - L - G, CX, CY - G, Color, Thickness);
		DrawLine(CX, CY + G, CX, CY + L + G, Color, Thickness);
	};

	DrawCross(Outline, T + 1.25f);
	DrawCross(Fill, T);
}
