// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VBPlayerController.generated.h"

class AVBCharacter;
class AVBHUD;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AVBPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDKills(int32 Kills);
	void SetHUDDeaths(int32 Deaths);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDWeaponMagCapacity(int32 MagCapacity);

	FTimerHandle ImpactCrosshairTimerHandle;
	UFUNCTION(Client, Reliable)
	void ClientSetHUDImpactCrosshair();

	FTimerHandle EliminatedTimerHandle;
	UFUNCTION(Client, Reliable)
	void ClientSetHUDEliminated(const FString& VictimName);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> VBContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookUpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> DropWeaponAction;

	void Move(const FInputActionValue& InputActionValue);
	void LookUp(const FInputActionValue& InputActionValue);
	void Turn(const FInputActionValue& InputActionValue);
	void Jump(const FInputActionValue& InputActionValue);
	void Equip(const FInputActionValue& InputActionValue);
	void Crouch(const FInputActionValue& InputActionValue);
	void UnCrouch(const FInputActionValue& InputActionValue);
	void Aim(const FInputActionValue& InputActionValue);
	void StopAim(const FInputActionValue& InputActionValue);
	void Fire(const FInputActionValue& InputActionValue);
	void StopFire(const FInputActionValue& InputActionValue);
	void Reload(const FInputActionValue& InputActionValue);
	void DropWeapon(const FInputActionValue& InputActionValue);

	void InterpCameraCrouch(float DeltaTime);
	float BaseZLocation = 145.f;
	float CrouchedZLocation = 100.f;
	float CurrentZLocation = 145.f;

	UPROPERTY()
	AVBHUD* VBHUD;

	UPROPERTY()
	AVBCharacter* VBOwnerCharacter;
};
