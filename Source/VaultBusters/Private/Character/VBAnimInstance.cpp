// Copyright Marco Freemantle

#include "Character/VBAnimInstance.h"
#include "Character/VBCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

// Animation blueprint BeginPlay equivalent
void UVBAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	VBCharacter = Cast<AVBCharacter>(TryGetPawnOwner());
}

// Animation blueprint Tick equivalent
void UVBAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(VBCharacter == nullptr)
	{
		VBCharacter = Cast<AVBCharacter>(TryGetPawnOwner());
	}
	if(VBCharacter == nullptr) return;

	FVector Velocity = VBCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = VBCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = VBCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
	bWeaponEquipped = VBCharacter->IsWeaponEquipped();
	EquippedWeapon = VBCharacter->GetEquippedWeapon();
	bIsCrouched = VBCharacter->bIsCrouched;
	bAiming = VBCharacter->IsAiming();
	TurningInPlace = VBCharacter->GetTurningInPlace();

	// Offset Yaw for strafing
	FRotator AimRotation = VBCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(VBCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 10.f);
	YawOffset = DeltaRotation.Yaw;

	// Leaning
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = VBCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	// Aim offsets
	AO_Yaw = VBCharacter->GetAO_Yaw();
	AO_Pitch = VBCharacter->GetAO_Pitch();

	if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && VBCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"),  RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		VBCharacter->GetMesh()->TransformToBoneSpace(FName("Hand_R"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}

	bUseFABRIK = VBCharacter->GetCombatState() != ECombatState::ECS_Reloading;
}
