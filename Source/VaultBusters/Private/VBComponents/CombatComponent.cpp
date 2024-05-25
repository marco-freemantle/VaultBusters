// Copyright Marco Freemantle

#include "VBComponents/CombatComponent.h"

#include "KismetTraceUtils.h"
#include "Camera/CameraComponent.h"
#include "Character/VBCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HUD/VBHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/VBPlayerController.h"
#include "Weapon/Weapon.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 400.f;
	AimWalkSpeed = 200.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (Character->GetFollowCamera())
        {
        	DefaultFOV = Character->GetFollowCamera()->FieldOfView;
        	CurrentFOV = DefaultFOV;
        }

		if(UWorld* World = GetWorld())
		{
			InvalidHitActor = World->SpawnActor<AActor>(InvalidHitActorClass, FVector(), FRotator());
			if(InvalidHitActor)
			{
				InvalidHitActor->SetActorHiddenInGame(true);
			}
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		FHitResult BarrelHitResult;
		TraceFromBarrel(BarrelHitResult);
		BarrelHitTarget = BarrelHitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

// Called on the Server from AVBCharacter
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(Character == nullptr || WeaponToEquip == nullptr) return;
	if(EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	if(EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, EquippedWeapon->GetActorLocation());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	if(EquippedWeapon->IsEmpty())
	{
		Reload();
	}
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon(const AWeapon* OldWeapon)
{
	if(EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		if(EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, EquippedWeapon->GetActorLocation());
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
	if(!EquippedWeapon && Character)
	{
		if(OldWeapon && Character->IsLocallyControlled())
		{
			OldWeapon->GetWeaponMesh()->SetVisibility(true);
		}
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->SetHUDWeaponAmmo(0);
			Controller->SetHUDWeaponTotalAmmo(0);
		}
		Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		Character->bUseControllerRotationYaw = false;
	}
}

// Called on the Server from AVBCharacter
void UCombatComponent::DropWeapon()
{
	if(Character == nullptr) return;
	if(EquippedWeapon)
	{
		EquippedWeapon->Dropped();
		EquippedWeapon->GetWeaponMesh()->AddImpulse(Character->GetFollowCamera()->GetForwardVector() * 600.f, FName(), true);
		CombatState = ECombatState::ECS_Unoccupied;
	}
	Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		Controller->SetHUDWeaponAmmo(0);
		Controller->SetHUDWeaponTotalAmmo(0);
	}
	EquippedWeapon = nullptr;
	SetAiming(false);
	Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	Character->GetCharacterMovement()->bOrientRotationToMovement = true;
	Character->bUseControllerRotationYaw = false;
}

void UCombatComponent::Reload()
{
	if(!EquippedWeapon) return;
	if(EquippedWeapon->TotalAmmo > 0 && EquippedWeapon->Ammo < EquippedWeapon->MagCapacity && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if(!Character || !EquippedWeapon) return;
	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

void UCombatComponent::FinishReloading()
{
	if(!Character) return;
	if(Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if(bIsFiring)
	{
		Fire();
	}
}


void UCombatComponent::UpdateAmmoValues()
{
	if (!Character || !EquippedWeapon) return;

	int32 RoomInMag = EquippedWeapon->MagCapacity - EquippedWeapon->Ammo;
	if(EquippedWeapon->TotalAmmo >= RoomInMag)
	{
		EquippedWeapon->AddAmmo(RoomInMag);
	}
	else
	{
		EquippedWeapon->AddAmmo(EquippedWeapon->TotalAmmo);
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if(bIsFiring)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
		);

	if(bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;
		if(Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 50.f);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility);
		
		if(!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		
		if(TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColour = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColour = FLinearColor::White;
		}
	}
}

void UCombatComponent::TraceFromBarrel(FHitResult& TraceHitResult)
{
	if((!EquippedWeapon || bElimmed) && InvalidHitActor)
	{
		InvalidHitActor->SetActorHiddenInGame(true);
		return;
	}
	if(!EquippedWeapon || InvalidHitActor) return;
	const USkeletalMeshSocket* MuzzleSocket = EquippedWeapon->GetWeaponMesh()->GetSocketByName(FName("muzzle"));
	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(EquippedWeapon->GetWeaponMesh());

	GetWorld()->LineTraceSingleByChannel(TraceHitResult, SocketTransform.GetLocation(), HitTarget, ECC_Visibility);

	if(!TraceHitResult.bBlockingHit || Cast<AProjectile>(TraceHitResult.GetActor()))
	{
		InvalidHitActor->SetActorHiddenInGame(true);
		return;
	}

	double VectorDistance = UKismetMathLibrary::Vector_Distance(TraceHitResult.ImpactPoint, HitTarget);

	if(VectorDistance > 10)
	{
		InvalidHitActor->SetActorHiddenInGame(false);
		InvalidHitActor->SetActorLocation(TraceHitResult.ImpactPoint);
	}
	else
	{
		InvalidHitActor->SetActorHiddenInGame(true);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bIsFiring = bPressed;
	if(bIsFiring)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if(CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);
		if(EquippedWeapon)
		{
			CrosshairShootingFactor = 0.75f;
		}
		StartFireTimer();
		if(Character)
		{
			APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
			CameraManager->StartCameraShake(CameraShakeClass);
		}
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if(EquippedWeapon == nullptr) return;
	if(Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::StartFireTimer()
{
	if(EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	if(EquippedWeapon == nullptr) return;
	bCanFire = true;
	if(bIsFiring && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if(EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if(!Character) return;
	if(!EquippedWeapon)
	{
		bAiming = false;
		ServerSetAiming(false);
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		return;
	}
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if(!EquippedWeapon) // No weapon
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (bAiming && EquippedWeapon) // Weapon and aiming
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else // Weapon and not aiming
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if(Character == nullptr || Character->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
	if(Controller)
	{
		HUD = HUD == nullptr ? Cast<AVBHUD>(Controller->GetHUD()) : HUD;
		if(HUD)
		{
			if(EquippedWeapon)
			{
				HUDPackage.CrosshairsCentre = EquippedWeapon->CrosshairsCentre;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCentre = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// Calculate crosshair spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if(Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if(bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			
			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

