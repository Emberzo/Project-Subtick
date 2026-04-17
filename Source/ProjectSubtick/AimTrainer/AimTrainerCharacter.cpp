// Copyright Project Subtick. All Rights Reserved.

#include "AimTrainer/AimTrainerCharacter.h"
#include "AimTrainer/AimTrainerSettingsSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FName ResolveFirstPersonWeaponSocket(const AProjectSubtickCharacter* Char)
	{
		static const FName CandidateSockets[] = {
			FName(TEXT("weapon_r")),
			FName(TEXT("WeaponSocket")),
			FName(TEXT("hand_r_socket")),
			FName(TEXT("hand_r")),
			FName(TEXT("ik_hand_gun")),
		};
		static const FName CandidateBones[] = {
			FName(TEXT("hand_r")),
			FName(TEXT("RightHand")),
			FName(TEXT("b_RightHand")),
			FName(TEXT("index_01_r")),
			FName(TEXT("weapon_r")),
		};

		const USkeletalMeshComponent* Fp = Char ? Char->GetFirstPersonMesh() : nullptr;
		if (!Fp)
		{
			UE_LOG(LogTemp, Warning, TEXT("Subtick: first-person mesh component is null; cannot resolve weapon socket/bone."));
			return NAME_None;
		}
		if (!Fp->GetSkeletalMeshAsset())
		{
			UE_LOG(LogTemp, Warning, TEXT("Subtick: first-person mesh has no skeletal asset at runtime; cannot resolve weapon socket/bone."));
			return NAME_None;
		}

		for (const FName Socket : CandidateSockets)
		{
			if (Fp->DoesSocketExist(Socket))
			{
				UE_LOG(LogTemp, Log, TEXT("Subtick: using first-person socket '%s' on '%s'."),
					*Socket.ToString(), *Fp->GetSkeletalMeshAsset()->GetName());
				return Socket;
			}
		}

		for (const FName Bone : CandidateBones)
		{
			if (Fp->GetBoneIndex(Bone) != INDEX_NONE)
			{
				UE_LOG(LogTemp, Log, TEXT("Subtick: no weapon socket found, using bone '%s' on '%s'."),
					*Bone.ToString(), *Fp->GetSkeletalMeshAsset()->GetName());
				return Bone;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Subtick: no matching socket/bone found on first-person mesh '%s'; attaching to mesh root."),
			*Fp->GetSkeletalMeshAsset()->GetName());

		return NAME_None;
	}

	void AutoFitFirstPersonWeaponScale(USkeletalMeshComponent* Weapon)
	{
		if (!Weapon || !Weapon->GetSkeletalMeshAsset())
		{
			return;
		}

		// Normalize oversized imports (common with external weapon assets) to a pistol-like screen size.
		const FVector Extent = Weapon->GetSkeletalMeshAsset()->GetBounds().BoxExtent;
		const float LongestDimension = Extent.GetMax() * 2.f;
		if (LongestDimension <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		static constexpr float TargetLongestDimension = 26.f; // ~26cm in UE units
		const float UniformScale = FMath::Clamp(TargetLongestDimension / LongestDimension, 0.02f, 2.0f);
		Weapon->SetRelativeScale3D(FVector(UniformScale));
	}

	USkeletalMesh* TryLoadProjectWeaponMesh()
	{
		static USkeletalMesh* Cached = nullptr;
		static bool bAttempted = false;
		if (bAttempted)
		{
			return Cached;
		}
		bAttempted = true;

		static const TCHAR* const CandidatePaths[] = {
			// Primary USPS import location in this project.
			TEXT("/Game/Weapons/Pistol/USPS/SkeletalMeshes/USPS.USPS"),
			// Legacy / alternate pistol mesh path from the template.
			TEXT("/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol"),
		};

		for (const TCHAR* Path : CandidatePaths)
		{
			if (USkeletalMesh* Sk = LoadObject<USkeletalMesh>(nullptr, Path, nullptr, LOAD_None, nullptr))
			{
				Cached = Sk;
				UE_LOG(LogTemp, Log, TEXT("Subtick: loaded project weapon mesh '%s'"), Path);
				return Cached;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Subtick: failed to load project weapon mesh from known paths."));
		return nullptr;
	}

	USkeletalMesh* LoadEngineFallbackWeaponSkeletalMesh()
	{
		static USkeletalMesh* Cached = nullptr;
		static bool bAttempted = false;
		if (bAttempted)
		{
			return Cached;
		}
		bAttempted = true;
		static const TCHAR* const CandidatePaths[] = {
			// Niagara ships with the editor; mesh is small enough to read as a stand-in.
			TEXT("/Niagara/Modules/Update/MeshInterface/SampleSkeletalMesh.SampleSkeletalMesh"),
			TEXT("/ControlRigModules/Modules/Meshes/SKM_Chain_Template.SKM_Chain_Template"),
			TEXT("/ControlRig/Modules/Meshes/SKM_Root_Template.SKM_Root_Template"),
			TEXT("/ControlRigModules/Modules/Meshes/SKM_Biped_Template.SKM_Biped_Template"),
		};
		for (const TCHAR* Path : CandidatePaths)
		{
			if (USkeletalMesh* Sk = LoadObject<USkeletalMesh>(nullptr, Path, nullptr, LOAD_None, nullptr))
			{
				Cached = Sk;
				return Cached;
			}
		}
		return nullptr;
	}

	USkeletalMesh* ResolveWeaponMeshFromCharacter(const AAimTrainerCharacter* Self)
	{
		if (!Self)
		{
			return nullptr;
		}

		// 1) Try ini-configured path first.
		const FSoftObjectPath& Path = Self->GetAimTrainerWeaponMeshPath();
		if (!Path.IsNull())
		{
			if (UObject* Resolved = Path.ResolveObject())
			{
				if (USkeletalMesh* ResolvedMesh = Cast<USkeletalMesh>(Resolved))
				{
					return ResolvedMesh;
				}
				UE_LOG(LogTemp, Warning, TEXT("Subtick: AimTrainerWeaponMeshPath resolves to non-skeletal object '%s'."), *Path.ToString());
			}
			if (USkeletalMesh* Loaded = Cast<USkeletalMesh>(Path.TryLoad()))
			{
				UE_LOG(LogTemp, Log, TEXT("Subtick: loaded ini weapon mesh '%s'"), *Path.ToString());
				return Loaded;
			}
			UE_LOG(LogTemp, Warning, TEXT("Subtick: failed to load AimTrainerWeaponMeshPath '%s'."), *Path.ToString());
		}

		// 2) Force-load from known project content paths.
		if (USkeletalMesh* ProjectMesh = TryLoadProjectWeaponMesh())
		{
			return ProjectMesh;
		}

		// 3) Last resort: engine sample meshes.
		if (Self->UsesEngineWeaponMeshFallback())
		{
			return LoadEngineFallbackWeaponSkeletalMesh();
		}
		return nullptr;
	}

	void EnsureFirstPersonMeshHasAsset(AProjectSubtickCharacter* Char)
	{
		if (!Char)
		{
			return;
		}

		USkeletalMeshComponent* Fp = Char->GetFirstPersonMesh();
		if (!Fp || Fp->GetSkeletalMeshAsset())
		{
			return;
		}

		USkeletalMeshComponent* Body = Char->GetMesh();
		if (!Body || !Body->GetSkeletalMeshAsset())
		{
			UE_LOG(LogTemp, Warning, TEXT("Subtick: first-person mesh has no asset and no body mesh fallback is available."));
			return;
		}

		Fp->SetSkeletalMesh(Body->GetSkeletalMeshAsset());
		if (UClass* AnimClass = Body->GetAnimClass())
		{
			Fp->SetAnimInstanceClass(AnimClass);
		}
		Fp->SetOnlyOwnerSee(true);
		Fp->SetOwnerNoSee(false);
		Fp->SetVisibility(true, true);
		UE_LOG(LogTemp, Warning, TEXT("Subtick: first-person mesh had no asset; copied body mesh '%s' as runtime fallback."),
			*Body->GetSkeletalMeshAsset()->GetName());
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

	bool IsAttachedToFirstPersonMesh(const USkeletalMeshComponent* Gun, const AProjectSubtickCharacter* Char)
	{
		return Gun && Char && Gun->GetAttachParent() == Char->GetFirstPersonMesh();
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
				if (!Sk || Sk == Char->GetMesh() || Sk == Char->GetFirstPersonMesh()
					|| Sk->GetFName() == FName(TEXT("AimTrainerViewWeapon")))
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
				AutoFitFirstPersonWeaponScale(Gun);
			}
		}

		// For an in-hand FPS setup, always prefer parenting to first-person arms mesh.
		if (!IsAttachedToFirstPersonMesh(Gun, Char))
		{
			if (USkeletalMeshComponent* Fp = Char->GetFirstPersonMesh())
			{
				const FName WeaponSocket = ResolveFirstPersonWeaponSocket(Char);
				if (WeaponSocket != NAME_None)
				{
					Gun->AttachToComponent(Fp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
					// Fine tune to sit in the right hand for common first-person skeletons.
					Gun->SetRelativeLocation(FVector(2.f, 1.f, -2.f));
					Gun->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
				}
				else
				{
					Gun->AttachToComponent(Fp, FAttachmentTransformRules::KeepRelativeTransform);
					Gun->SetRelativeLocation(FVector(18.f, 8.f, -9.f));
					Gun->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
				}
			}
			else if (UCameraComponent* Cam = Char->GetFirstPersonCameraComponent())
			{
				Gun->AttachToComponent(Cam, FAttachmentTransformRules::KeepRelativeTransform);
				Gun->SetRelativeLocation(FVector(18.f, 8.f, -9.f));
				Gun->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
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
	if (USkeletalMeshComponent* Fp = GetFirstPersonMesh())
	{
		// Stable default: parent to first-person mesh root.
		// Socket-specific transforms vary wildly across imported rigs and can push the mesh out of view.
		AimTrainerViewWeapon->SetupAttachment(Fp);
	}
	else if (UCameraComponent* Cam = GetFirstPersonCameraComponent())
	{
		AimTrainerViewWeapon->SetupAttachment(Cam);
	}
	else
	{
		AimTrainerViewWeapon->SetupAttachment(GetRootComponent());
	}
	AimTrainerViewWeapon->SetRelativeLocation(FVector(18.f, 8.f, -9.f));
	AimTrainerViewWeapon->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	AimTrainerViewWeapon->SetRelativeScale3D(FVector(1.f));
	AimTrainerViewWeapon->SetCollisionProfileName(FName("NoCollision"));
	AimTrainerViewWeapon->SetOnlyOwnerSee(true);
	AimTrainerViewWeapon->SetHiddenInGame(false);
	AimTrainerViewWeapon->SetVisibility(true, true);
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
	EnsureFirstPersonMeshHasAsset(this);

	if (AimTrainerViewWeapon && !AimTrainerViewWeapon->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* Sk = ResolveWeaponMeshFromCharacter(this))
		{
			AimTrainerViewWeapon->SetSkeletalMesh(Sk);
			AutoFitFirstPersonWeaponScale(AimTrainerViewWeapon);
		}
	}

	if (AimTrainerViewWeapon)
	{
		USkeletalMeshComponent* Fp = GetFirstPersonMesh();
		const bool bHasUsableFirstPersonMesh = Fp && Fp->GetSkeletalMeshAsset();
		if (bHasUsableFirstPersonMesh)
		{
			const FName WeaponSocket = ResolveFirstPersonWeaponSocket(this);
			if (WeaponSocket != NAME_None)
			{
				AimTrainerViewWeapon->AttachToComponent(Fp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
				AimTrainerViewWeapon->SetRelativeLocation(FVector(2.f, 1.f, -2.f));
				AimTrainerViewWeapon->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
			}
			else
			{
				AimTrainerViewWeapon->AttachToComponent(Fp, FAttachmentTransformRules::KeepRelativeTransform);
				AimTrainerViewWeapon->SetRelativeLocation(FVector(18.f, 8.f, -9.f));
				AimTrainerViewWeapon->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
			}
		}
		else if (UCameraComponent* Cam = GetFirstPersonCameraComponent())
		{
			// If the arms mesh is unassigned, anchor the native view weapon to camera so it still renders.
			AimTrainerViewWeapon->AttachToComponent(Cam, FAttachmentTransformRules::KeepRelativeTransform);
			AimTrainerViewWeapon->SetRelativeLocation(FVector(18.f, 8.f, -9.f));
			AimTrainerViewWeapon->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
			UE_LOG(LogTemp, Warning, TEXT("Subtick: first-person mesh unavailable, attached AimTrainerViewWeapon to camera fallback."));
		}
	}

	EnsureUspWeaponOnCharacter(this);

	// Keep native view weapon visible for consistency; Blueprint gun can coexist while we tune sockets.
	if (AimTrainerViewWeapon)
	{
		AimTrainerViewWeapon->SetHiddenInGame(false);
		AimTrainerViewWeapon->SetOwnerNoSee(false);
		AimTrainerViewWeapon->SetOnlyOwnerSee(true);
		AimTrainerViewWeapon->SetHiddenInGame(false);
		AimTrainerViewWeapon->SetVisibility(true, false);
		AimTrainerViewWeapon->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	}

	if (USkeletalMeshComponent* BpGun = FindBlueprintGunSkMesh(this))
	{
		BpGun->SetHiddenInGame(false);
		BpGun->SetOwnerNoSee(false);
		BpGun->SetOnlyOwnerSee(true);
		BpGun->SetVisibility(true, false);
		BpGun->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	}

	if (AimTrainerViewWeapon && !AimTrainerViewWeapon->GetSkeletalMeshAsset())
	{
		static bool bOnce = false;
		if (!bOnce)
		{
			bOnce = true;
			UE_LOG(LogTemp, Warning, TEXT("Subtick: no weapon skeletal mesh (fallback disabled or failed). Set AimTrainerWeaponMeshPath in DefaultEngine.ini, "
										  "enable bAimTrainerEngineFallbackWeapon, or assign mesh on Gun_USP in Blueprint."));
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
