// Copyright Marco Freemantle

#include "Character/VBCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Game/VBGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/OverheadWidget.h"
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

	AttackingTeamHairMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HairMesh"));
	AttackingTeamHairMesh->SetupAttachment(GetMesh(), FName("HeadSocket"));
	AttackingTeamHairMesh->SetVisibility(false);
	
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

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
	DOREPLIFETIME(AVBCharacter, ExplosiveGrenadeCount);
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

void AVBCharacter::Destroyed()
{
	Super::Destroyed();

	if(Combat)
	{
		if(!Combat->EquippedWeapon) return;
		Combat->EquippedWeapon->Destroy();
		if(!Combat->SecondaryWeapon) return;
		Combat->SecondaryWeapon->Destroy();
	}
}

void AVBCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	UpdateHUDGrenadeCount();
	SetupOverheadWidget();
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
	SetupOverheadWidget();
	PollInit();
}

void AVBCharacter::PollInit()
{
	if(VBPlayerState == nullptr)
	{
		VBPlayerState = GetPlayerState<AVBPlayerState>();
		if(VBPlayerState)
		{
			SetTeamMesh(VBPlayerState->GetTeam());
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
	VBGameMode = VBGameMode == nullptr ? GetWorld()->GetAuthGameMode<AVBGameMode>() : VBGameMode;
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

void AVBCharacter::SetTeamMesh(ETeam Team)
{
	switch (Team)
	{
	case ETeam::ET_AttackingTeam:
	{
		GetMesh()->SetSkeletalMesh(AttackingTeamMesh);
		AttackingTeamHairMesh->SetVisibility(true);
		break;
	}
	case ETeam::ET_DefendingTeam:
	{
		GetMesh()->SetSkeletalMesh(DefendingTeamMesh);
		break;
	}
	case ETeam::ET_NoTeam:
		break;
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

void AVBCharacter::SwapWeapons()
{
	if (Combat->ShouldSwapWeapons() && !HasAuthority())
	{
		Combat->CombatState = ECombatState::ECS_SwappingWeapons;
		PlaySwapWeaponsMontage();
	}
	ServerSwapWeapons();
}

void AVBCharacter::ServerSwapWeapons_Implementation()
{
	if (Combat && Combat->ShouldSwapWeapons())
	{
		StopAnimMontage(ReloadMontage);
		MulticastInterruptReload();
		Combat->SwapWeapons();
	}
}

void AVBCharacter::EquipWeapon()
{
	if(Combat)
	{
		ServerEquipWeapon();
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
				Combat->EquippedWeapon = nullptr;
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
				Combat->SecondaryWeapon = nullptr;
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
		DisableInput(VBPlayerController);
	}
	bElimmed = true;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetSimulatePhysics(true);

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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
	VBGameMode = VBGameMode == nullptr ? GetWorld()->GetAuthGameMode<AVBGameMode>() : VBGameMode;
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
	VBGameMode = VBGameMode == nullptr ? GetWorld()->GetAuthGameMode<AVBGameMode>() : VBGameMode;
	if (bElimmed || VBGameMode == nullptr) return;
	Damage = VBGameMode->CalculateDamage(InstigatorController, Controller, Damage);
	
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

	if(Health != 0.f) return;
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

void AVBCharacter::HideCameraIfCharacterClose() const
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

void AVBCharacter::UpdateHUDGrenadeCount()
{
	VBPlayerController = VBPlayerController == nullptr ? Cast<AVBPlayerController>(Controller) : VBPlayerController;
	if(VBPlayerController)
	{
		VBPlayerController->SetHUDExplosiveGrenadeCount(ExplosiveGrenadeCount);
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

void AVBCharacter::OnRep_ExplosiveGrenadeCount()
{
	UpdateHUDGrenadeCount();
}

void AVBCharacter::SetupOverheadWidget()
{
	if(bOverheadWidgetSetup) return;
	if (UUserWidget* PlayerWidget = OverheadWidget->GetUserWidgetObject())
	{
		if(UOverheadWidget* PlayerOverheadWidget = Cast<UOverheadWidget>(PlayerWidget); GetPlayerState())
		{
			FString Name = GetPlayerState()->GetPlayerName();
			PlayerOverheadWidget->SetDisplayText(Name);
			
			if(!VBPlayerState) return;
			
			if(IsLocallyControlled())
			{
				OverheadWidget->SetVisibility(false);
			}
			if (!IsLocallyControlled())
			{
				AVBPlayerState* RemotePlayerState = Cast<AVBPlayerState>(GetPlayerState());
				AVBPlayerState* LocalPlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<AVBPlayerState>();

				if (RemotePlayerState && LocalPlayerState)
				{
					if(RemotePlayerState->GetTeam() != LocalPlayerState->GetTeam())
					{
						OverheadWidget->SetVisibility(false);
					}
				}
			}
			bOverheadWidgetSetup = true;
		}
	}
}
