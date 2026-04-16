// Copyright Project Subtick. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "ProjectSubtickCharacter.h"
#include "AimTrainerCharacter.generated.h"

class USkeletalMeshComponent;

/**
 * Static aim trainer pawn: no movement, CS2-style yaw/pitch from sensitivity * m_yaw.
 */
UCLASS(Config = Engine)
class PROJECTSUBTICK_API AAimTrainerCharacter : public AProjectSubtickCharacter
{
	GENERATED_BODY()

	/**
	 * Skeletal mesh for the native viewmodel (and optional fill-in on Gun_USP). Set in DefaultEngine.ini:
	 * [/Script/ProjectSubtick.AimTrainerCharacter] AimTrainerWeaponMeshPath=/Game/YourFolder/USPS.USPS
	 * Use the asset path from Content Browser → Copy Reference (Skeletal Mesh asset, not Blueprint).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Subtick|Weapon")
	FSoftObjectPath AimTrainerWeaponMeshPath;

	/** Native viewmodel (USPS) on the first-person camera; works even if Blueprint weapon wiring fails. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> AimTrainerViewWeapon;

public:
	AAimTrainerCharacter();

	virtual void PostRegisterAllComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	void ApplyAimTrainerWeaponSetup();

	virtual void DoAim(float Yaw, float Pitch) override;
	virtual void DoMove(float Right, float Forward) override;
	virtual void DoJumpStart() override;
	virtual void DoJumpEnd() override;

	/** Optional Blueprint hook (BeginPlay no longer toggles FirstPersonMesh visibility — that hid weapon children). */
	UPROPERTY(EditAnywhere, Category = "Subtick|Visual")
	bool bShowFirstPersonArms = false;
};
