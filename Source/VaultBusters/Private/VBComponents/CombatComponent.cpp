// Copyright Marco Freemantle

#include "VBComponents/CombatComponent.h"

#include "Character/VBCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 400.f;
	AimWalkSpeed = 200.f;
	BaseArmLength = 250.f;
	AimArmLength = 85.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		Character->GetCameraBoom()->TargetArmLength = BaseArmLength;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	InterpolateCameraArmLength(DeltaTime);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
}

// Called on the Server from AVBCharacter
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(Character == nullptr || WeaponToEquip == nullptr) return;
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	if(const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket")))
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::Fire(bool bFiring)
{
	bIsFiring = bFiring;
	if(EquippedWeapon == nullptr) return;
	if(Character && bIsFiring)
	{
		Character->PlayFireMonatge(bAiming);
		EquippedWeapon->Fire();
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if(!Character || !Character->GetEquippedWeapon()) return;
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

void UCombatComponent::InterpolateCameraArmLength(float DeltaTime) const
{
	if(Character)
	{
		float DesiredArmLength = bAiming ? AimArmLength : BaseArmLength;
		float CurrentArmLength = Character->GetCameraBoom()->TargetArmLength;
		float InterpArmLength = FMath::FInterpTo(CurrentArmLength, DesiredArmLength, DeltaTime, 10.f); // 10.f is the interpolation speed, adjust as necessary
		Character->GetCameraBoom()->TargetArmLength = InterpArmLength;
	}
}

