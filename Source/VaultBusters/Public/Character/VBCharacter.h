// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "VBTypes/TurningInPlace.h"
#include "VBTypes/CombatState.h"
#include "VBCharacter.generated.h"

class AVBPlayerState;
class AVBPlayerController;
class USpringArmComponent;
class UCameraComponent;
class AWeapon;
class UCombatComponent;

UCLASS()
class VAULTBUSTERS_API AVBCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	AVBCharacter();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void UpdateHUDHealth();
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();

	void SetOverlappingWeapon(AWeapon* Weapon);

	void EquipWeapon();
	void DropWeapon();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitReceived(bool bWasHeadShot);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastInterruptReload();

	void Elim();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

protected:
	virtual void BeginPlay() override;
	void AimOffset(float DeltaTime);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	void PollInit();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	 USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY()
	AVBPlayerController* VBPlayerController;

	UPROPERTY()
	AVBPlayerState* VBPlayerState;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();

	UFUNCTION(Server, Reliable)
	void ServerDropWeapon();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* ThrowGrenadeMontage;

	void HideCameraIfCharacterClose();

	// Player health
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, Category="Player Stats", VisibleAnywhere)
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	bool bElimmed = false;

	FTimerHandle ElimTimer;
	void ElimTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	UPROPERTY(EditAnywhere)
	USoundBase* HitReceivedSound;

	UPROPERTY(EditAnywhere)
	USoundBase* HeadshotReceivedSound;

public:
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	AWeapon* GetEquippedWeapon() const;
	ECombatState GetCombatState() const;

	bool IsWeaponEquipped() const;
	bool IsAiming() const;
};
