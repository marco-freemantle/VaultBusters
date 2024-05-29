// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ProjectileWeapon.h"
#include "ProjectileShotgun.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AProjectileShotgun : public AProjectileWeapon
{
	GENERATED_BODY()

public:
	virtual void Fire(const FVector& HitTarget) override;

	UPROPERTY(EditAnywhere)
	int32 NumShellsPerShot = 6;
};
