// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/VBHUD.h"
#include "VBTypes/CombatState.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

class AVBHUD;
class AVBPlayerController;
class AWeapon;
class AVBCharacter;
class UCameraShakeBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VAULTBUSTERS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();

	// Allows AVBCharacter full access to member variables and methods
	friend class AVBCharacter;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	bool bAiming;

	void EquipWeapon(AWeapon* WeaponToEquip);
	void DropWeapon();
	void SetAiming(bool bIsAiming);
	void FireButtonPressed(bool bPressed);
	
	void Reload();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	void UpdateAmmoValues();
	
	bool bIsFiring;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon, BlueprintReadOnly)
	AWeapon* EquippedWeapon;
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon(const AWeapon* OldWeapon);

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void TraceFromBarrel(FHitResult& TraceHitResult);
	void Fire();

	void SetHUDCrosshairs(float DeltaTime);

private:
	AVBCharacter* Character;
	AVBPlayerController* Controller;
	AVBHUD* HUD;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FHUDPackage HUDPackage;

	FVector HitTarget;
	FVector BarrelHitTarget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> InvalidHitActorClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY()
	AActor* InvalidHitActor;

	bool bElimmed = false;
	
	// Aiming FOV 
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	// Automatic fire
	FTimerHandle FireTimer;

	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();
};
