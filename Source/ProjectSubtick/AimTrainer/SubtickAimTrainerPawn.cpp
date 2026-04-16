// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/SubtickAimTrainerPawn.h"
#include "AimTrainer/SubtickAimTrainerSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

ASubtickAimTrainerPawn::ASubtickAimTrainerPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootScene);
	Camera->SetRelativeLocation(FVector::ZeroVector);

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	TargetMesh->SetupAttachment(RootScene);
	TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TargetMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		TargetMesh->SetStaticMesh(SphereMesh.Object);
	}
	TargetMesh->SetRelativeLocation(FVector(400.f, 0.f, 40.f));
	TargetMesh->SetRelativeScale3D(FVector(0.35f));
}

void ASubtickAimTrainerPawn::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetViewTarget(this);
	}

	LocalStats = FSubtickDrillSessionStats();
}

void ASubtickAimTrainerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("SubtickYaw"), this, &ASubtickAimTrainerPawn::AxisYaw);
	PlayerInputComponent->BindAxis(TEXT("SubtickPitch"), this, &ASubtickAimTrainerPawn::AxisPitch);
	PlayerInputComponent->BindAction(TEXT("SubtickPrimaryFire"), IE_Pressed, this,
		&ASubtickAimTrainerPawn::PrimaryFire);
}

void ASubtickAimTrainerPawn::AxisYaw(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		ApplyLookDelta(Value, 0.f);
	}
}

void ASubtickAimTrainerPawn::AxisPitch(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		ApplyLookDelta(0.f, Value);
	}
}

void ASubtickAimTrainerPawn::ApplyLookDelta(float RawX, float RawY)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return;
	}
	USubtickAimTrainerSubsystem* Sub = GI->GetSubsystem<USubtickAimTrainerSubsystem>();
	if (!Sub)
	{
		return;
	}

	const float Sens = Sub->GetEffectiveSensitivity();
	const float YawK = Sub->GetYawConstant();
	YawDegrees += RawX * Sens * YawK;
	PitchDegrees = FMath::Clamp(PitchDegrees - RawY * Sens * YawK, -89.f, 89.f);

	SetActorRotation(FRotator(0.f, YawDegrees, 0.f));
	Camera->SetRelativeRotation(FRotator(PitchDegrees, 0.f, 0.f));
}

void ASubtickAimTrainerPawn::PrimaryFire()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Camera)
	{
		return;
	}

	FVector Origin;
	FRotator Rot;
	PC->GetPlayerViewPoint(Origin, Rot);

	const FVector End = Origin + Rot.Vector() * 50000.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SubtickAimFire), true, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Params))
	{
		LocalStats.Misses++;
		return;
	}

	if (Hit.GetComponent() == TargetMesh)
	{
		LocalStats.Hits++;
	}
	else
	{
		LocalStats.Misses++;
	}
}
