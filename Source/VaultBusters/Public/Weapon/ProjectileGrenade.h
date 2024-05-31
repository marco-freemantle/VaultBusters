// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	void DestroyTimerFinished();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEffects();
private:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* GrenadeMesh;
	
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
	
	UPROPERTY(EditAnywhere)
	USoundBase* ExplosionSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ExplosionParticles;
};
