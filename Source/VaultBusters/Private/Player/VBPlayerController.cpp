// Copyright Marco Freemantle

#include "Player/VBPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Character/VBCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/VBInputComponent.h"
#include "VBComponents/CombatComponent.h"

AVBPlayerController::AVBPlayerController()
{
	bReplicates = true;
}

void AVBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// CursorTrace();
}

void AVBPlayerController::CursorTrace()
{
	FHitResult CursorHit;
	GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (!CursorHit.bBlockingHit) return;
}

void AVBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(VBContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(VBContext, 0);
	}

	bShowMouseCursor = false;

	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
}

void AVBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UVBInputComponent* VBInputComponent = CastChecked<UVBInputComponent>(InputComponent);

	VBInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Move);
	VBInputComponent->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AVBPlayerController::LookUp);
	VBInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Turn);
	VBInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Jump);
	VBInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Equip);
	VBInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AVBPlayerController::Crouch);
	VBInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AVBPlayerController::UnCrouch);
	VBInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AVBPlayerController::Aim);
	VBInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AVBPlayerController::StopAim);
	VBInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AVBPlayerController::Fire);
	VBInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AVBPlayerController::StopFire);
}

void AVBPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void AVBPlayerController::LookUp(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddControllerPitchInput(InputAxisVector.X);
	}
}

void AVBPlayerController::Turn(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddControllerYawInput(InputAxisVector.X);
	}
}

void AVBPlayerController::Jump(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->Jump();
	}
}

void AVBPlayerController::Equip(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->EquipWeapon();
	}
}

void AVBPlayerController::Crouch(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetCharacterMovement()->IsFalling()) return;
		VBCharacter->Crouch();
	}
}

void AVBPlayerController::UnCrouch(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->UnCrouch();
	}
}

void AVBPlayerController::Aim(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->SetAiming(true);
	}
}

void AVBPlayerController::StopAim(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->SetAiming(false);
	}
}

void AVBPlayerController::Fire(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->FireButtonPressed(true);
	}
}

void AVBPlayerController::StopFire(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}
