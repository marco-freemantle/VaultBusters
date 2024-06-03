// Copyright Marco Freemantle

#include "Character/VBCharacter.h"

#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Game/VBGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/VBPlayerController.h"
#include "Player/VBPlayerState.h"
#include "VaultBusters/VaultBusters.h"
#include "VBComponents/BuffComponent.h"
#include "VBComponents/CombatComponent.h"
#include "Weapon/Weapon.h"

AVBCharacter::AVBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 250.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void AVBCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AVBCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AVBCharacter, Health);
	DOREPLIFETIME(AVBCharacter, Shield);
}

void AVBCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
	}
}

void AVBCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AVBCharacter::ReceiveDamage);
	}
	if(AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void AVBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AimOffset(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void AVBCharacter::PollInit()
{
	if(VBPlayerState == nullptr)
	{
		VBPlayerState = GetPlayerState<AVBPlayerState>();
		if(VBPlayerState)
		{
			VBPlayerState->AddToScore(0.f);
			VBPlayerState->AddToDeaths(0);
			VBPlayerState->AddToKills(0);
		}
	}
}

void AVBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AVBCharacter::SpawnDefaultWeapon()
{
	AVBGameMode* VBGameMode = Cast<AVBGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if(VBGameMode && World && !bElimmed && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = false;
		if(Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void AVBCharacter::PlayFireMontage(bool bAiming)
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AVBCharacter::PlayHitReactMontage()
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AVBCharacter::PlayReloadMontage()
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AVBCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void AVBCharacter::PlaySwapWeaponsMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && SwapWeaponsMontage)
	{
		AnimInstance->Montage_Play(SwapWeaponsMontage);
	}
}

void AVBCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(OverlappingWeapon && IsLocallyControlled())
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon = Weapon;
	
	if(IsLocallyControlled() && OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
}

void AVBCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void AVBCharacter::EquipWeapon()
{
	if(Combat)
	{
		if (Combat->CombatState == ECombatState::ECS_Unoccupied) ServerEquipWeapon();
		if (Combat->ShouldSwapWeapons() && !HasAuthority() && Combat->CombatState == ECombatState::ECS_Unoccupied && OverlappingWeapon == nullptr)
		{
			Combat->CombatState = ECombatState::ECS_SwappingWeapons;
			PlaySwapWeaponsMontage();
		}
	}
}

void AVBCharacter::ServerEquipWeapon_Implementation()
{
	if(Combat)
	{
		if(OverlappingWeapon)
		{
			if(Combat->SecondaryWeapon)
			{
				StopAnimMontage(ReloadMontage);
				MulticastInterruptReload();
			}
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			StopAnimMontage(ReloadMontage);
			MulticastInterruptReload();
			Combat->SwapWeapons();
		}
	}
}

void AVBCharacter::DropWeapon()
{
	if(Combat)
	{
		StopAnimMontage(ReloadMontage);
		MulticastInterruptReload();
		ServerDropWeapon();
	}
}

void AVBCharacter::ServerDropWeapon_Implementation()
{
	if(Combat)
	{
		Combat->DropWeapon();
		StopAnimMontage(ReloadMontage);
		MulticastInterruptReload();
	}
}

void AVBCharacter::MulticastInterruptReload_Implementation()
{
	if(!ReloadMontage) return;
	StopAnimMontage(ReloadMontage);
	if(Combat)
	{
		Combat->CombatState = ECombatState::ECS_Unoccupied;
	}
}

bool AVBCharacter::IsWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon);
}

bool AVBCharacter::IsAiming() const
{
	return (Combat && Combat->bAiming);
}

void AVBCharacter::Elim()
{
	if(Combat)
	{
		if(Combat->EquippedWeapon)
		{
			if(Combat->EquippedWeapon->bDestroyWeapon)
			{
				Combat->EquippedWeapon->Destroy();
			}
			else
			{
				Combat->EquippedWeapon->Dropped();
			}
		}
		if(Combat->SecondaryWeapon)
		{
			if(Combat->SecondaryWeapon->bDestroyWeapon)
			{
				Combat->SecondaryWeapon->Destroy();
			}
			else
			{
				Combat->SecondaryWeapon->Dropped();
			}
		}
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(ElimTimer, this, &AVBCharacter::ElimTimerFinished, ElimDelay);
}

void AVBCharacter::MulticastElim_Implementation()
{
	if(VBPlayerController)
	{
		VBPlayerController->SetHUDWeaponAmmo(0);
		VBPlayerController->SetHUDWeaponTotalAmmo(0);
	}
	bElimmed = true;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if(VBPlayerController)
	{
		DisableInput(VBPlayerController);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if(Combat && Combat->InvalidHitActor)
	{
		Combat->bElimmed = true;
		Combat->InvalidHitActor->Destroy();
	}
	if(IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		ShowSniperScopeWidget(false);
	}
}

void AVBCharacter::ElimTimerFinished()
{
	AVBGameMode* VBGameMode = GetWorld()->GetAuthGameMode<AVBGameMode>();
	if(VBGameMode)
	{
		VBGameMode->RequestRespawn(this, Controller);
	}
}

void AVBCharacter::AimOffset(float DeltaTime)
{
	if(Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f && !bIsInAir) // Standing still & not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if(Speed > 0.f || bIsInAir) // Running or Jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// Map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AVBCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed) return;

	float DamageToHealth = Damage;
	if(Shield > 0.f)
	{
		if(Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}
	
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();
	MulticastPlayHitReceived(false);

	if(Health != 0.f) return;
	AVBGameMode* VBGameMode = GetWorld()->GetAuthGameMode<AVBGameMode>();
	if(VBGameMode)
	{
		VBPlayerController = VBPlayerController == nullptr ? Cast<AVBPlayerController>(Controller) : VBPlayerController;
		AVBPlayerController* AttackerController = Cast<AVBPlayerController>(InstigatorController);
		VBGameMode->PlayerEliminated(this, VBPlayerController, AttackerController);
	}
}

void AVBCharacter::MulticastPlayHitReceived_Implementation(bool bWasHeadShot)
{
	if(bWasHeadShot)
	{
		if(!HeadshotReceivedSound) return;
		UGameplayStatics::PlaySoundAtLocation(this, HeadshotReceivedSound, GetActorLocation());
	}
	else
	{
		if(!HitReceivedSound) return;
		UGameplayStatics::PlaySoundAtLocation(this, HitReceivedSound, GetActorLocation());
	}
}

void AVBCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if( TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 8.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 1.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AVBCharacter::HideCameraIfCharacterClose()
{
	if(!IsLocallyControlled()) return;

	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() <= 200.f)
	{
		GetMesh()->SetVisibility(false);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(false);
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(true);
		}
	}
}

AWeapon* AVBCharacter::GetEquippedWeapon() const
{
	if(Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

ECombatState AVBCharacter::GetCombatState() const
{
	if(Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

void AVBCharacter::UpdateHUDHealth()
{
	VBPlayerController = VBPlayerController == nullptr ? Cast<AVBPlayerController>(Controller) : VBPlayerController;
	if(VBPlayerController)
	{
		VBPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AVBCharacter::UpdateHUDShield()
{
	VBPlayerController = VBPlayerController == nullptr ? Cast<AVBPlayerController>(Controller) : VBPlayerController;
	if(VBPlayerController)
	{
		VBPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void AVBCharacter::UpdateHUDAmmo()
{
	VBPlayerController = VBPlayerController == nullptr ? Cast<AVBPlayerController>(Controller) : VBPlayerController;
	if(VBPlayerController && Combat && Combat->EquippedWeapon)
	{
		VBPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->Ammo);
		VBPlayerController->SetHUDWeaponTotalAmmo(Combat->EquippedWeapon->TotalAmmo);
	}
}

void AVBCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void AVBCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}
