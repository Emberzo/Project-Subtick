// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerProceduralWorld.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
/** Engine cube mesh is 100×100×100 uu; scales multiply that. */
static constexpr float CubeUnit = 100.f;

static UStaticMesh* LoadEngineCubeMesh()
{
	static UStaticMesh* Cached = nullptr;
	if (Cached)
	{
		return Cached;
	}
	Cached = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	return Cached;
}

static UMaterialInterface* LoadBasicShapeMaterial()
{
	static UMaterialInterface* Cached = nullptr;
	if (Cached)
	{
		return Cached;
	}
	Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	return Cached;
}

static UMaterialInterface* LoadDefaultLitMaterial()
{
	static UMaterialInterface* Cached = nullptr;
	if (Cached)
	{
		return Cached;
	}
	Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	return Cached;
}

static void ApplyWhiteRoomMaterial(UStaticMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}
	if (UMaterialInterface* Base = LoadBasicShapeMaterial())
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, Mesh->GetOwner()))
		{
			// BasicShapeMaterial uses "BaseColor" on most engine versions; MID still works if the param is ignored.
			Mid->SetVectorParameterValue(TEXT("BaseColor"), FVector(0.94f, 0.94f, 0.96f));
			Mesh->SetMaterial(0, Mid);
			return;
		}
	}
	if (UMaterialInterface* Lit = LoadDefaultLitMaterial())
	{
		Mesh->SetMaterial(0, Lit);
	}
}

static bool AnyEnvActorsExist(UWorld* World)
{
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FName("SubtickAimTrainerEnv")))
		{
			return true;
		}
	}
	return false;
}

static bool AnyPlayerStartExists(UWorld* World)
{
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		return true;
	}
	return false;
}

static void SpawnRoomBoxPanel(UWorld* World, UStaticMesh* CubeMesh, const FVector& CenterWorld, const FVector& ScaleXYZ,
	const FString& Label, const FName& EnvTag, FActorSpawnParameters& Params)
{
	AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), CenterWorld, FRotator::ZeroRotator, Params);
	if (!A)
	{
		return;
	}
	A->Tags.Add(EnvTag);
	A->SetActorLabel(Label);
	if (UStaticMeshComponent* Mesh = A->GetStaticMeshComponent())
	{
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetWorldScale3D(ScaleXYZ);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Mesh->SetMobility(EComponentMobility::Static);
		Mesh->SetCastShadow(false);
		ApplyWhiteRoomMaterial(Mesh);
	}
}
} // namespace

void AimTrainerProceduralWorld::EnsureArena(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (AnyEnvActorsExist(World))
	{
		return;
	}

	UStaticMesh* CubeMesh = LoadEngineCubeMesh();
	if (!CubeMesh)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags = RF_Transient;

	const FName EnvTag(TEXT("SubtickAimTrainerEnv"));

	// Inner playable volume ≈ 500×500×260 uu (small white box room).
	const float HalfW = 250.f;
	const float HalfD = 250.f;
	const float RoomH = 260.f;
	const float WallT = 20.f;
	const float HalfWallT = WallT * 0.5f;

	// Floor — top at z=0
	SpawnRoomBoxPanel(World, CubeMesh, FVector(0.f, 0.f, -HalfWallT), FVector((HalfW + HalfWallT) * 2.f / CubeUnit, (HalfD + HalfWallT) * 2.f / CubeUnit, WallT / CubeUnit),
		TEXT("SubtickAimTrainerRoomFloor"), EnvTag, Params);

	// Ceiling — bottom at z=RoomH
	SpawnRoomBoxPanel(World, CubeMesh, FVector(0.f, 0.f, RoomH + HalfWallT),
		FVector((HalfW + HalfWallT) * 2.f / CubeUnit, (HalfD + HalfWallT) * 2.f / CubeUnit, WallT / CubeUnit), TEXT("SubtickAimTrainerRoomCeiling"), EnvTag, Params);

	const float WallXScale = WallT / CubeUnit;
	const float WallYSpan = (HalfD + HalfWallT) * 2.f / CubeUnit;
	const float WallZScale = RoomH / CubeUnit;
	const float WallYScale = WallT / CubeUnit;
	const float WallXSpan = (HalfW + HalfWallT) * 2.f / CubeUnit;
	const float WallCenterZ = RoomH * 0.5f;

	// ±X walls
	SpawnRoomBoxPanel(World, CubeMesh, FVector(HalfW + HalfWallT, 0.f, WallCenterZ), FVector(WallXScale, WallYSpan, WallZScale), TEXT("SubtickAimTrainerWallPosX"), EnvTag, Params);
	SpawnRoomBoxPanel(World, CubeMesh, FVector(-HalfW - HalfWallT, 0.f, WallCenterZ), FVector(WallXScale, WallYSpan, WallZScale), TEXT("SubtickAimTrainerWallNegX"), EnvTag, Params);
	// ±Y walls
	SpawnRoomBoxPanel(World, CubeMesh, FVector(0.f, HalfD + HalfWallT, WallCenterZ), FVector(WallXSpan, WallYScale, WallZScale), TEXT("SubtickAimTrainerWallPosY"), EnvTag, Params);
	SpawnRoomBoxPanel(World, CubeMesh, FVector(0.f, -HalfD - HalfWallT, WallCenterZ), FVector(WallXSpan, WallYScale, WallZScale), TEXT("SubtickAimTrainerWallNegY"), EnvTag, Params);

	// Soft interior fill (no outdoor sky — reads as a white box).
	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.f, 0.f, RoomH * 0.5f), FRotator::ZeroRotator, Params))
	{
		Sky->Tags.Add(EnvTag);
		Sky->SetActorLabel(TEXT("SubtickAimTrainerSkyLight"));
		if (USkyLightComponent* SL = Sky->GetLightComponent())
		{
			SL->SetIntensity(1.35f);
			SL->SetLightColor(FLinearColor(1.f, 1.f, 1.f));
			SL->SetMobility(EComponentMobility::Movable);
			SL->bRealTimeCapture = true;
		}
	}

	if (ADirectionalLight* Key = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(-120.f, -80.f, RoomH + 120.f),
			FRotator(-48.f, 32.f, 0.f), Params))
	{
		Key->Tags.Add(EnvTag);
		Key->SetActorLabel(TEXT("SubtickAimTrainerKeyLight"));
		if (UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(Key->GetLightComponent()))
		{
			L->SetIntensity(14.f);
			L->SetLightColor(FLinearColor(1.f, 0.99f, 0.96f));
			L->SetMobility(EComponentMobility::Movable);
			L->SetCastShadows(false);
		}
	}

	// Corner fill lights so the room stays evenly lit without harsh shadows.
	const float CornerXY = 160.f;
	const float CornerZ = RoomH - 40.f;
	const TArray<FVector> Corners = {
		FVector(CornerXY, CornerXY, CornerZ),
		FVector(-CornerXY, CornerXY, CornerZ),
		FVector(CornerXY, -CornerXY, CornerZ),
		FVector(-CornerXY, -CornerXY, CornerZ),
	};
	int32 CornerIdx = 0;
	for (const FVector& P : Corners)
	{
		if (APointLight* PL = World->SpawnActor<APointLight>(APointLight::StaticClass(), P, FRotator::ZeroRotator, Params))
		{
			PL->Tags.Add(EnvTag);
			PL->SetActorLabel(FString::Printf(TEXT("SubtickAimTrainerFillLight_%d"), CornerIdx++));
			if (UPointLightComponent* C = Cast<UPointLightComponent>(PL->GetLightComponent()))
			{
				C->SetIntensity(12000.f);
				C->SetLightColor(FLinearColor(1.f, 1.f, 1.f));
				C->SetAttenuationRadius(620.f);
				C->SetMobility(EComponentMobility::Movable);
				C->bUseInverseSquaredFalloff = false;
			}
		}
	}

	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params))
	{
		Fog->Tags.Add(EnvTag);
		Fog->SetActorLabel(TEXT("SubtickAimTrainerFog"));
		if (UExponentialHeightFogComponent* FC = Fog->GetComponent())
		{
			FC->SetFogDensity(0.002f);
			FC->SetFogMaxOpacity(0.08f);
		}
	}

	if (!AnyPlayerStartExists(World))
	{
		// Default character capsule half-height is 96 in this project — stand on floor.
		if (APlayerStart* PS = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.f, 0.f, 96.f), FRotator(0.f, 0.f, 0.f), Params))
		{
			PS->Tags.Add(EnvTag);
			PS->SetActorLabel(TEXT("SubtickAimTrainerPlayerStart"));
		}
	}
}
