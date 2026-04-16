// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerCharacter.h"
#include "AimTrainer/AimTrainerSettingsSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"

namespace
{
	USkeletalMesh* ResolveWeaponMeshFromCharacter(const AAimTrainerCharacter* Self)
	{
		if (!Self || Self->AimTrainerWeaponMeshPath.IsNull())
		{
			return nullptr;
		}
		if (UObject* Resolved = Self->AimTrainerWeaponMeshPath.ResolveObject())
		{
			return Cast<USkeletalMesh>(Resolved);
		}
		return Cast<USkeletalMesh>(Self->AimTrainerWeaponMeshPath.TryLoad());
	}

	bool IsUnderFirstPersonChain(const USkeletalMeshComponent* Gun, const AProjectSubtickCharacter* Char)
	{
		if (!Gun || !Char)
		{
			return false;
		}
		for (const USceneComponent* A = Gun->GetAttachParent(); A; A = A->GetAttachParent())
		{
			if (A == Char->GetFirstPersonMesh() || A == Char->GetFirstPersonCameraComponent())
			{
				return true;
			}
		}
		return false;
	}

	void EnsureUspWeaponOnCharacter(AAimTrainerCharacter* Char)
	{
		if (!Char)
		{
			return;
		}

		USkeletalMeshComponent* Gun = nullptr;
		{
			TArray<USkeletalMeshComponent*> SkMeshes;
			Char->GetComponents<USkeletalMeshComponent>(SkMeshes);
			for (USkeletalMeshComponent* Sk : SkMeshes)
			{
				if (!Sk || Sk == Char->GetMesh() || Sk == Char->GetFirstPersonMesh() || Sk == Char->AimTrainerViewWeapon)
				{
					continue;
				}
				const FString N = Sk->GetName();
				if (N.Contains(TEXT("Gun_USP")) || N == TEXT("Gun_USP"))
				{
					Gun = Sk;
					break;
				}
			}
		}
		if (!Gun)
		{
			return;
		}

		if (!Gun->GetSkeletalMeshAsset())
		{
			if (USkeletalMesh* Loaded = ResolveWeaponMeshFromCharacter(Char))
			{
				Gun->SetSkeletalMesh(Loaded);
			}
		}

		if (!IsUnderFirstPersonChain(Gun, Char))
		{
			if (UCameraComponent* Cam = Char->GetFirstPersonCameraComponent())
			{
				Gun->AttachToComponent(Cam, FAttachmentTransformRules::KeepWorldTransform);
				if (Gun->GetRelativeLocation().IsNearlyZero(1.f))
				{
					Gun->SetRelativeLocation(FVector(12.f, 10.f, -8.f));
					Gun->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
				}
			}
			else if (USkeletalMeshComponent* Fp = Char->GetFirstPersonMesh())
			{
				Gun->AttachToComponent(Fp, FAttachmentTransformRules::KeepWorldTransform);
			}
		}

		Gun->SetCollisionProfileName(FName("NoCollision"));
		Gun->SetOnlyOwnerSee(true);
		Gun->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		Gun->SetVisibility(true, false);
	}

	USkeletalMeshComponent* FindBlueprintGunSkMesh(const AProjectSubtickCharacter* Char)
	{
		if (!Char)
		{
			return nullptr;
		}
		TArray<USkeletalMeshComponent*> SkMeshes;
		Char->GetComponents<USkeletalMeshComponent>(SkMeshes);
		for (USkeletalMeshComponent* Sk : SkMeshes)
		{
			if (!Sk || Sk == Char->GetMesh() || Sk == Char->GetFirstPersonMesh())
			{
				continue;
			}
			if (Sk->GetFName() == FName(TEXT("AimTrainerViewWeapon")))
			{
				continue;
			}
			const FString N = Sk->GetName();
			if (N.Contains(TEXT("Gun_USP")) || N == TEXT("Gun_USP"))
			{
				return Sk;
			}
		}
		return nullptr;
	}
} // namespace

AAimTrainerCharacter::AAimTrainerCharacter()
{
	// Base class parents First Person Mesh under CharacterMesh0. Reparent FP subtree to the capsule
	// so hiding the third-person mesh does not cull the first-person camera / weapons.
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		if (USkeletalMeshComponent* Fp = GetFirstPersonMesh())
		{
			const FTransform FpRelativeToCapsule = Body->GetRelativeTransform() * Fp->GetRelativeTransform();
			Fp->SetupAttachment(GetCapsuleComponent());
			Fp->SetRelativeTransform(FpRelativeToCapsule);
		}
	}

	AimTrainerViewWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AimTrainerViewWeapon"));
	if (UCameraComponent* Cam = GetFirstPersonCameraComponent())
	{
		AimTrainerViewWeapon->SetupAttachment(Cam);
	}
	else if (USkeletalMeshComponent* Fp = GetFirstPersonMesh())
	{
		AimTrainerViewWeapon->SetupAttachment(Fp);
	}
	else
	{
		AimTrainerViewWeapon->SetupAttachment(GetRootComponent());
	}
	AimTrainerViewWeapon->SetRelativeLocation(FVector(12.f, 10.f, -6.f));
	AimTrainerViewWeapon->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	AimTrainerViewWeapon->SetCollisionProfileName(FName("NoCollision"));
	AimTrainerViewWeapon->SetOnlyOwnerSee(true);
	AimTrainerViewWeapon->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	AimTrainerViewWeapon->SetCastShadow(false);
}

void AAimTrainerCharacter::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	ApplyAimTrainerWeaponSetup();
}

void AAimTrainerCharacter::ApplyAimTrainerWeaponSetup()
{
	if (AimTrainerViewWeapon && !AimTrainerViewWeapon->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* Sk = ResolveWeaponMeshFromCharacter(this))
		{
			AimTrainerViewWeapon->SetSkeletalMesh(Sk);
		}
	}

	EnsureUspWeaponOnCharacter(this);

	// Prefer a Blueprint-configured gun when it is parented correctly and has a mesh.
	if (USkeletalMeshComponent* BpGun = FindBlueprintGunSkMesh(this))
	{
		if (BpGun->GetSkeletalMeshAsset() && IsUnderFirstPersonChain(BpGun, this))
		{
			if (AimTrainerViewWeapon)
			{
				AimTrainerViewWeapon->SetVisibility(false, false);
			}
		}
	}

	if (AimTrainerViewWeapon && !AimTrainerViewWeapon->GetSkeletalMeshAsset())
	{
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			UE_LOG(LogTemp, Warning, TEXT("Subtick: no weapon skeletal mesh. Set [/Script/ProjectSubtick.AimTrainerCharacter] AimTrainerWeaponMeshPath "
										  "in DefaultEngine.ini to your USPS Skeletal Mesh (Content Browser → Copy Reference), or assign mesh on Gun_USP in Blueprint."));
		}
	}
}

void AAimTrainerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
		Move->StopMovementImmediately();
	}
	ApplyAimTrainerWeaponSetup();
}

void AAimTrainerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAimTrainerCharacter::DoAim(float Yaw, float Pitch)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UAimTrainerSettingsSubsystem* Sub = GI->GetSubsystem<UAimTrainerSettingsSubsystem>())
		{
			const float Sens = Sub->GetEffectiveSensitivity();
			AddControllerYawInput(Yaw * Sens * Sub->MYaw);
			AddControllerPitchInput(-Pitch * Sens * Sub->MPitch);
			return;
		}
	}
	Super::DoAim(Yaw, Pitch);
}

void AAimTrainerCharacter::DoMove(float Right, float Forward)
{
}

void AAimTrainerCharacter::DoJumpStart()
{
}

void AAimTrainerCharacter::DoJumpEnd()
{
}
