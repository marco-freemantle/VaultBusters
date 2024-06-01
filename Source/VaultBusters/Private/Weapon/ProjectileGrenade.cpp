// Copyright Marco Freemantle

#include "Weapon/ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGrenade::AProjectileGrenade()
{
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	GrenadeMesh->SetupAttachment(RootComponent);
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay();

	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);

	if(HasAuthority())
	{
		FTimerHandle DestroyTimer;
		GetWorldTimerManager().SetTimer(
			DestroyTimer,
			this,
			&AProjectileGrenade::DestroyTimerFinished,
			2.f
		);
	}
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation()
		);
	}
}

void AProjectileGrenade::DestroyTimerFinished()
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn && HasAuthority())
	{
		if (AController* FiringController = FiringPawn->GetController())
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				200.f,
				500.f, 
				1.f,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				FiringController
			);
		}
		MulticastPlayEffects();
	}
	Destroy();
}

void AProjectileGrenade::MulticastPlayEffects_Implementation()
{
	if(ExplosionParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticles, GetActorLocation());
	}
	if(ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
}
