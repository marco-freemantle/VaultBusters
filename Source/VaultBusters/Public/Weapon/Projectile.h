// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"
#include "Projectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;

UCLASS()
class VAULTBUSTERS_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	bool bHitFlesh;

	bool bHasHitSomething;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitEffects(bool bFleshHit);

	void DelayedDestroy();

private:
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ProjectileTracer;
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles_NonNiagara;

	UPROPERTY(EditAnywhere)
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BloodImpactParticles;
};
