// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "VBTypes/TurningInPlace.h"
#include "VBTypes/CombatState.h"
#include "VBTypes/Team.h"
#include "VBCharacter.generated.h"

class UOverheadWidget;
class AVBGameMode;
class AVBPlayerState;
class AVBPlayerController;
class USpringArmComponent;
class UCameraComponent;
class AWeapon;
class UCombatComponent;
class UWidgetComponent;

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
	virtual void Destroyed() override;
	void SetupOverheadWidget();
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	void UpdateHUDGrenadeCount();
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapWeaponsMontage();

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

	void SpawnDefaultWeapon();

	void SetTeamMesh(ETeam Team);

	void SwapWeapons();

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

	UPROPERTY(EditAnywhere)
	USkeletalMesh* AttackingTeamMesh;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* AttackingTeamHairMesh;
	
	UPROPERTY(EditAnywhere)
	USkeletalMesh* DefendingTeamMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

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

	UFUNCTION(Server, Reliable)
	void ServerSwapWeapons();

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

	UPROPERTY(EditAnywhere, Category=Combat)
	UAnimMontage* SwapWeaponsMontage;

	void HideCameraIfCharacterClose() const;

	// Player health
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, Category="Player Stats", VisibleAnywhere)
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	// Player shield
	UPROPERTY(EditAnywhere, Category="Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, Category="Player Stats", VisibleAnywhere)
	float Shield = 100.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	bool bElimmed = false;

	bool bOverheadWidgetSetup = false;

	FTimerHandle ElimTimer;
	void ElimTimerFinished();

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	UPROPERTY(EditAnywhere)
	USoundBase* HitReceivedSound;

	UPROPERTY(EditAnywhere)
	USoundBase* HeadshotReceivedSound;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY()
	AVBGameMode* VBGameMode;

	// Throwables
	UPROPERTY(ReplicatedUsing=OnRep_ExplosiveGrenadeCount)
	int32 ExplosiveGrenadeCount = 2;

	UFUNCTION()
	void OnRep_ExplosiveGrenadeCount();

public:
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE int32 GetExplosiveGrenadeCount() const { return ExplosiveGrenadeCount; }
	FORCEINLINE void ExpendExplosiveGrenade() { --ExplosiveGrenadeCount; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UWidgetComponent* GetOverheadWidget() const { return OverheadWidget; }
	AWeapon* GetEquippedWeapon() const;
	ECombatState GetCombatState() const;

	bool IsWeaponEquipped() const;
	bool IsAiming() const;
};
