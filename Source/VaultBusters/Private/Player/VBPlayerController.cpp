// Copyright Marco Freemantle

#include "Player/VBPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Character/VBCharacter.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HUD/CharacterOverlay.h"
#include "Input/VBInputComponent.h"
#include "VBComponents/CombatComponent.h"
#include "Weapon/Weapon.h"

AVBPlayerController::AVBPlayerController()
{
	bReplicates = true;
}

void AVBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	InterpCameraCrouch(DeltaTime);
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

	VBHUD = Cast<AVBHUD>(GetHUD());
}

void AVBPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AVBCharacter* VBCharacter = Cast<AVBCharacter>(InPawn);
	if(VBCharacter)
	{
		SetHUDHealth(VBCharacter->GetHealth(), VBCharacter->GetMaxHealth());
	}
}

void AVBPlayerController::InterpCameraCrouch(float DeltaTime)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(!VBCharacter->GetCameraBoom()) return;
		if(VBCharacter->bIsCrouched)
		{
			CurrentZLocation = FMath::FInterpTo(CurrentZLocation, CrouchedZLocation, DeltaTime, 7.f);
			VBCharacter->GetCameraBoom()->SetRelativeLocation(FVector(0.f, 0.f, CurrentZLocation));
		}
		else
		{
			CurrentZLocation = FMath::FInterpTo(CurrentZLocation, BaseZLocation, DeltaTime, 7.f);
			VBCharacter->GetCameraBoom()->SetRelativeLocation(FVector(0.f, 0.f, CurrentZLocation));
		}
	}
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
	VBInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Triggered, this, &AVBPlayerController::DropWeapon);
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

void AVBPlayerController::Reload(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->Reload();
	}
}

void AVBPlayerController::DropWeapon(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetEquippedWeapon())
		{
			VBCharacter->DropWeapon();
		}
	}
}

void AVBPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		VBHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AVBPlayerController::SetHUDScore(float Score)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		VBHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void AVBPlayerController::SetHUDKills(int32 Kills)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->KillsAmount)
	{
		FString KillsText = FString::Printf(TEXT("%d"), Kills);
		VBHUD->CharacterOverlay->KillsAmount->SetText(FText::FromString(KillsText));
	}
}

void AVBPlayerController::SetHUDDeaths(int32 Deaths)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->DeathsAmount)
	{
		FString DeathsText = FString::Printf(TEXT("%d"), Deaths);
		VBHUD->CharacterOverlay->DeathsAmount->SetText(FText::FromString(DeathsText));
	}
}

void AVBPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->AmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		VBHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AVBPlayerController::SetHUDWeaponMagCapacity(int32 MagCapacity)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->MagCapacity)
	{
		FString MagCapacityText = FString::Printf(TEXT("%d"), MagCapacity);
		VBHUD->CharacterOverlay->MagCapacity->SetText(FText::FromString(MagCapacityText));
	}
}

void AVBPlayerController::ClientSetHUDImpactCrosshair_Implementation()
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;

	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ImpactCrosshair && IsLocalController())
	{
		GetWorldTimerManager().ClearTimer(ImpactCrosshairTimerHandle);
		VBHUD->CharacterOverlay->ImpactCrosshair->SetVisibility(ESlateVisibility::Visible);

		GetWorldTimerManager().SetTimer(ImpactCrosshairTimerHandle, [this]() {
			//Set visibility to hidden after 0.75 seconds
			if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ImpactCrosshair)
			{
				VBHUD->CharacterOverlay->ImpactCrosshair->SetVisibility(ESlateVisibility::Hidden);
			}
			}, 0.75f, false);
	}
}

void AVBPlayerController::ClientSetHUDEliminated_Implementation(const FString& VictimName)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;

	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->Eliminated)
	{
		GetWorldTimerManager().ClearTimer(EliminatedTimerHandle);

		UHorizontalBox* Box = Cast<UHorizontalBox>(VBHUD->CharacterOverlay->Eliminated->GetChildAt(0));
		if (Box)
		{
			UTextBlock* VictimNameText = Cast<UTextBlock>(Box->GetChildAt(2));
			if (VictimNameText)
			{
				VictimNameText->SetText(FText::FromString(VictimName));
			}
		}

		VBHUD->CharacterOverlay->Eliminated->SetVisibility(ESlateVisibility::Visible);

		GetWorldTimerManager().SetTimer(EliminatedTimerHandle, [this]() {
			//Set visibility to hidden after 3 second
			if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->Eliminated)
			{
				VBHUD->CharacterOverlay->Eliminated->SetVisibility(ESlateVisibility::Hidden);
			}
			}, 3.f, false);
	}
}