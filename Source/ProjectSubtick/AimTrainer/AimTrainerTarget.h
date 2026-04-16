// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AimTrainerTarget.generated.h"

class UStaticMeshComponent;
class USphereComponent;

/** Simple sphere "orb" target (Aim Lab / KovaaK style). */
UCLASS()
class PROJECTSUBTICK_API AAimTrainerTarget : public AActor
{
	GENERATED_BODY()

public:
	AAimTrainerTarget();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subtick")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Subtick")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subtick")
	float SphereRadius = 36.f;

	UFUNCTION(BlueprintCallable, Category = "Subtick")
	void SetHighlight(bool bOn);

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
