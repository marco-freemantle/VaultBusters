// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/VBAnimInstance.h"
#include "Character/VBCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UVBAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	VBCharacter = Cast<AVBCharacter>(TryGetPawnOwner());
}

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
}
