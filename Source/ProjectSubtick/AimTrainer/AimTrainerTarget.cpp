// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerTarget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

AAimTrainerTarget::AAimTrainerTarget()
{
	PrimaryActorTick.bCanEverTick = false;
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(SphereRadius);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->SetGenerateOverlapEvents(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereAsset.Succeeded())
	{
		Mesh->SetStaticMesh(SphereAsset.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> BaseMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat.Succeeded())
	{
		Mesh->SetMaterial(0, BaseMat.Object);
	}
	const float S = SphereRadius / 50.f;
	Mesh->SetRelativeScale3D(FVector(S));
}

void AAimTrainerTarget::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Mat = Mesh ? Mesh->GetMaterial(0) : nullptr)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(Mat, this);
		if (DynamicMaterial)
		{
			Mesh->SetMaterial(0, DynamicMaterial);
			SetHighlight(false);
		}
	}
}

void AAimTrainerTarget::SetHighlight(bool bOn)
{
	if (DynamicMaterial)
	{
		const FLinearColor C = bOn ? FLinearColor(1.f, 0.25f, 0.08f) : FLinearColor(0.15f, 0.85f, 0.35f);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), C);
	}
}
