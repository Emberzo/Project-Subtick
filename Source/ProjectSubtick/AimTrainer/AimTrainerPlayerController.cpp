// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerPlayerController.h"
#include "AimTrainer/AimTrainerGameMode.h"
#include "AimTrainer/AimTrainerTarget.h"
#include "AimTrainer/AimTrainerSettingsSubsystem.h"
#include "ProjectSubtickCharacter.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

AAimTrainerPlayerController::AAimTrainerPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAimTrainerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);
}

void AAimTrainerPlayerController::OnMouseTurn(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	if (AProjectSubtickCharacter* Ch = Cast<AProjectSubtickCharacter>(GetPawn()))
	{
		Ch->DoAim(Value, 0.f);
	}
}

void AAimTrainerPlayerController::OnMouseLookUp(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	if (AProjectSubtickCharacter* Ch = Cast<AProjectSubtickCharacter>(GetPawn()))
	{
		Ch->DoAim(0.f, Value);
	}
}

void AAimTrainerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Mouse look: Enhanced Input only works when mapping contexts are assigned on this controller.
	// Many aim-trainer setups leave those arrays empty — fall back to legacy Turn / LookUp axes.
	bool bHasAnyIMC = false;
	for (UInputMappingContext* C : DefaultMappingContexts)
	{
		if (C)
		{
			bHasAnyIMC = true;
			break;
		}
	}
	if (!bHasAnyIMC)
	{
		for (UInputMappingContext* C : MobileExcludedMappingContexts)
		{
			if (C)
			{
				bHasAnyIMC = true;
				break;
			}
		}
	}
	if (!bHasAnyIMC)
	{
		InputComponent->BindAxis(TEXT("Turn"), this, &AAimTrainerPlayerController::OnMouseTurn);
		InputComponent->BindAxis(TEXT("LookUp"), this, &AAimTrainerPlayerController::OnMouseLookUp);
	}

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAimTrainerPlayerController::PrimaryFire);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AAimTrainerPlayerController::KeyOne);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AAimTrainerPlayerController::KeyTwo);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AAimTrainerPlayerController::KeyThree);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AAimTrainerPlayerController::KeyCalibrateToggle);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AAimTrainerPlayerController::ToggleSettingsHud);
}

void AAimTrainerPlayerController::ToggleSettingsHud()
{
	bSettingsHud = !bSettingsHud;
}

void AAimTrainerPlayerController::PrimaryFire()
{
	TraceCenterView();
}

void AAimTrainerPlayerController::KeyOne()
{
	if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
	{
		GM->SetDrill(ESubtickAimDrill::StaticClick);
	}
}

void AAimTrainerPlayerController::KeyTwo()
{
	if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
	{
		GM->SetDrill(ESubtickAimDrill::MicroFlick);
	}
}

void AAimTrainerPlayerController::KeyThree()
{
	if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
	{
		GM->SetDrill(ESubtickAimDrill::TargetSwitch);
	}
}

void AAimTrainerPlayerController::KeyCalibrateToggle()
{
	if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
	{
		GM->CalibrationToggleSegment();
	}
}

void AAimTrainerPlayerController::TraceCenterView()
{
	FHitResult Hit;
	FVector Start;
	FRotator Rot;
	GetPlayerViewPoint(Start, Rot);
	const FVector Dir = Rot.Vector();
	const FVector End = Start + Dir * 50000.f;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(SubtickTrace), true, GetPawn());
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q))
	{
		if (AAimTrainerTarget* T = Cast<AAimTrainerTarget>(Hit.GetActor()))
		{
			if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
			{
				GM->RegisterTargetHit(T);
			}
		}
		else if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
		{
			GM->RegisterMiss();
		}
	}
	else if (AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>())
	{
		GM->RegisterMiss();
	}
}

void AAimTrainerPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HudAccum += DeltaSeconds;
	if (HudAccum < 0.1f)
	{
		return;
	}
	HudAccum = 0.f;
	if (!GetWorld())
	{
		return;
	}
	UAimTrainerSettingsSubsystem* Sub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAimTrainerSettingsSubsystem>() : nullptr;
	AAimTrainerGameMode* GM = GetWorld()->GetAuthGameMode<AAimTrainerGameMode>();
	auto DrillToString = [](ESubtickAimDrill D) -> FString
	{
		switch (D)
		{
		case ESubtickAimDrill::StaticClick: return TEXT("Static Click");
		case ESubtickAimDrill::MicroFlick: return TEXT("Micro Flick");
		case ESubtickAimDrill::TargetSwitch: return TEXT("Target Switch");
		case ESubtickAimDrill::Calibration: return TEXT("Calibration");
		default: return TEXT("—");
		}
	};
	const FString DrillName = GM ? DrillToString(GM->GetCurrentDrill()) : TEXT("—");
	const float HitRate = GM ? GM->GetSessionStats().GetHitRate() * 100.f : 0.f;
	const float ExpCm = Sub ? Sub->GetExpectedCmPer360() : 0.f;
	GEngine->AddOnScreenDebugMessage(1, 0.2f, FColor::Green, FString::Printf(TEXT("Subtick | %s | Hit%% %.1f | cm/360 ~ %.2f | 1/2/3 drills | C cal | Tab settings | LMB shoot"), *DrillName, HitRate, ExpCm));
	if (bSettingsHud && Sub)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.2f, FColor::Yellow, FString::Printf(TEXT("Sens %.4f  m_yaw %.5f  FineTune %.4f  DPI %.0f"), Sub->Sensitivity, Sub->MYaw, Sub->FineTuneMultiplier, Sub->MouseDpi));
		GEngine->AddOnScreenDebugMessage(3, 0.2f, FColor::Yellow, FString::Printf(TEXT("Import entries: %d  warnings: %d"), Sub->LastImport.Entries.Num(), Sub->LastImport.Warnings.Num()));
	}
}
