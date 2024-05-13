// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AWeapon;
class AVBCharacter;

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
	
	void EquipWeapon(AWeapon* WeaponToEquip);

	UPROPERTY(Replicated)
	bool bAiming;

	void SetAiming(bool bIsAiming);
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

private:
	AVBCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
};
