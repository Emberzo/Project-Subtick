// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerGameMode.h"
#include "AimTrainer/AimTrainerCrosshairHUD.h"
#include "AimTrainer/AimTrainerProceduralWorld.h"
#include "AimTrainer/AimTrainerTarget.h"
#include "AimTrainer/AimTrainerCharacter.h"
#include "AimTrainer/AimTrainerPlayerController.h"
#include "AimTrainer/AimTrainerSettingsSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AAimTrainerGameMode::AAimTrainerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AAimTrainerCharacter::StaticClass();
	PlayerControllerClass = AAimTrainerPlayerController::StaticClass();
	HUDClass = AAimTrainerCrosshairHUD::StaticClass();
	TargetClass = AAimTrainerTarget::StaticClass();
}

void AAimTrainerGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (AimTrainerDefaultPawnClass)
	{
		DefaultPawnClass = AimTrainerDefaultPawnClass;
	}
}

void AAimTrainerGameMode::StartPlay()
{
	Super::StartPlay();
	AimTrainerProceduralWorld::EnsureArena(GetWorld());
	SetDrill(ESubtickAimDrill::StaticClick);
}

FVector AAimTrainerGameMode::ViewDir(APawn* Pawn) const
{
	if (!Pawn)
	{
		return FVector::ForwardVector;
	}
	return Pawn->GetControlRotation().Vector();
}

void AAimTrainerGameMode::ClearTargets()
{
	if (ActiveTarget)
	{
		ActiveTarget->Destroy();
		ActiveTarget = nullptr;
	}
	for (AAimTrainerTarget* T : SwitchTargets)
	{
		if (IsValid(T))
		{
			T->Destroy();
		}
	}
	SwitchTargets.Reset();
}

void AAimTrainerGameMode::SpawnOrbAt(APawn* Pawn, const FVector& WorldLocation, TObjectPtr<AAimTrainerTarget>& Slot)
{
	if (!TargetClass || !GetWorld())
	{
		return;
	}
	if (Slot)
	{
		Slot->Destroy();
		Slot = nullptr;
	}
	FActorSpawnParameters Sp;
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Slot = GetWorld()->SpawnActor<AAimTrainerTarget>(TargetClass, WorldLocation, FRotator::ZeroRotator, Sp);
}

void AAimTrainerGameMode::SpawnStaticOrMicroTarget(bool bMicro)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	const FVector Eye = Pawn->GetPawnViewLocation();
	FVector Dir;
	if (bMicro)
	{
		const float YawJ = FMath::FRandRange(-6.f, 6.f);
		const float PitchJ = FMath::FRandRange(-4.f, 4.f);
		const FRotator R(Pawn->GetControlRotation().Pitch + PitchJ, Pawn->GetControlRotation().Yaw + YawJ, 0.f);
		Dir = R.Vector();
	}
	else
	{
		const float YawJ = FMath::FRandRange(-35.f, 35.f);
		const float PitchJ = FMath::FRandRange(-18.f, 18.f);
		const FRotator R(Pawn->GetControlRotation().Pitch + PitchJ, Pawn->GetControlRotation().Yaw + YawJ, 0.f);
		Dir = R.Vector();
	}
	const FVector Loc = Eye + Dir * TargetDistance;
	SpawnOrbAt(Pawn, Loc, ActiveTarget);
	LastSpawnWorldTime = GetWorld()->GetTimeSeconds();
}

void AAimTrainerGameMode::SetupTargetSwitch()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || !TargetClass)
	{
		return;
	}
	ClearTargets();
	const FVector Eye = Pawn->GetPawnViewLocation();
	const FRotator Base = Pawn->GetControlRotation();
	const TArray<float> Yaws = { -22.f, 0.f, 24.f };
	const TArray<float> Pitches = { -8.f, 4.f, -12.f };
	for (int32 i = 0; i < 3; ++i)
	{
		const FRotator Off(Base.Pitch + Pitches[i], Base.Yaw + Yaws[i], 0.f);
		const FVector Dir = Off.Vector();
		const FVector Loc = Eye + Dir * TargetDistance;
		TObjectPtr<AAimTrainerTarget> T = nullptr;
		SpawnOrbAt(Pawn, Loc, T);
		if (T)
		{
			SwitchTargets.Add(T);
		}
	}
	SwitchCursor = 0;
	AdvanceSwitchHighlight();
	LastSpawnWorldTime = GetWorld()->GetTimeSeconds();
}

void AAimTrainerGameMode::AdvanceSwitchHighlight()
{
	for (int32 i = 0; i < SwitchTargets.Num(); ++i)
	{
		if (AAimTrainerTarget* T = SwitchTargets[i].Get())
		{
			T->SetHighlight(i == SwitchCursor);
		}
	}
}

void AAimTrainerGameMode::SetDrill(ESubtickAimDrill NewDrill)
{
	CurrentDrill = NewDrill;
	Stats = FSubtickDrillSessionStats();
	DrillTimeAccumulator = 0.f;
	ClearTargets();
	if (!GetWorld())
	{
		return;
	}
	switch (CurrentDrill)
	{
	case ESubtickAimDrill::StaticClick:
		SpawnStaticOrMicroTarget(false);
		break;
	case ESubtickAimDrill::MicroFlick:
		SpawnStaticOrMicroTarget(true);
		break;
	case ESubtickAimDrill::TargetSwitch:
		SetupTargetSwitch();
		break;
	case ESubtickAimDrill::Calibration:
		// No targets: player uses C toggle + yaw accumulation in Tick (see CalibrationToggleSegment).
		break;
	default:
		break;
	}
}

void AAimTrainerGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CurrentDrill != ESubtickAimDrill::None && CurrentDrill != ESubtickAimDrill::Calibration)
	{
		DrillTimeAccumulator += DeltaSeconds;
		Stats.TotalTimeSeconds = DrillTimeAccumulator;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}
	if (!bCalibrationRecording)
	{
		LastControlYaw = PC->GetControlRotation().Yaw;
		return;
	}
	const float Yaw = PC->GetControlRotation().Yaw;
	float D = Yaw - LastControlYaw;
	if (D > 180.f)
	{
		D -= 360.f;
	}
	if (D < -180.f)
	{
		D += 360.f;
	}
	CalibrationYawThisSegment += D;
	LastControlYaw = Yaw;
}

void AAimTrainerGameMode::CalibrationToggleSegment()
{
	if (!bCalibrationRecording)
	{
		bCalibrationRecording = true;
		CalibrationYawThisSegment = 0.f;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			LastControlYaw = PC->GetControlRotation().Yaw;
		}
		return;
	}
	bCalibrationRecording = false;
	const float AbsYaw = FMath::Abs(CalibrationYawThisSegment);
	if (AbsYaw > 1.f && GetGameInstance())
	{
		if (UAimTrainerSettingsSubsystem* Sub = GetGameInstance()->GetSubsystem<UAimTrainerSettingsSubsystem>())
		{
			const float Factor = 360.f / AbsYaw;
			Sub->FineTuneMultiplier *= Factor;
			LastSuggestedFineTune = Factor;
			CalibrationMeasuredCmSamples.Add(Factor);
		}
	}
}

void AAimTrainerGameMode::RegisterTargetHit(AAimTrainerTarget* Target)
{
	if (!Target || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const float TTH = Now - LastSpawnWorldTime;
	if (CurrentDrill == ESubtickAimDrill::TargetSwitch)
	{
		if (SwitchTargets.IsValidIndex(SwitchCursor) && SwitchTargets[SwitchCursor].Get() == Target)
		{
			Stats.Hits++;
			Stats.SumTimeToHitSeconds += TTH;
			SwitchCursor = (SwitchCursor + 1) % FMath::Max(SwitchTargets.Num(), 1);
			AdvanceSwitchHighlight();
			LastSpawnWorldTime = Now;
		}
		else
		{
			Stats.Misses++;
		}
		return;
	}
	if (Target == ActiveTarget.Get())
	{
		Stats.Hits++;
		Stats.SumTimeToHitSeconds += TTH;
		if (CurrentDrill == ESubtickAimDrill::StaticClick)
		{
			SpawnStaticOrMicroTarget(false);
		}
		else if (CurrentDrill == ESubtickAimDrill::MicroFlick)
		{
			SpawnStaticOrMicroTarget(true);
		}
		LastSpawnWorldTime = GetWorld()->GetTimeSeconds();
	}
}

void AAimTrainerGameMode::RegisterMiss()
{
	Stats.Misses++;
}
