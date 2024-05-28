// Copyright Marco Freemantle

#include "Weapon/Projectile.h"

#include "Character/VBCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "VaultBusters/VaultBusters.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

	if(ProjectileTracer)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(ProjectileTracer, GetRootComponent(), FName(), FVector(0.f), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, false);
	}
}

// Only called on the server
void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Bool stops OnHit being registered several times
	if (bHasHitSomething) return;

	AVBCharacter* PlayerHit = Cast<AVBCharacter>(OtherActor);

	bHitFlesh = PlayerHit != nullptr;

	MulticastPlayHitEffects(bHitFlesh);

	FTimerHandle DestroyTimerHandle;
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ThisClass::DelayedDestroy, 0.05f, false);

	bHasHitSomething = true;
}

void AProjectile::MulticastPlayHitEffects_Implementation(bool bFleshHit)
{
	if (bFleshHit)
	{
		if (BloodImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BloodImpactParticles, GetActorLocation());
		}
	}
	else
	{
		if(ImpactParticles)
		{
			FTransform ParticleTransform;
			
			ParticleTransform.SetRotation(FQuat(FRotator(90, 90, 90)));
			
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactParticles, GetActorLocation(), ParticleTransform.Rotator(), FVector(0.5f));
		}
		if(ImpactParticles_NonNiagara)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles_NonNiagara, GetActorLocation());
		}
		if(ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
		}
	}
}

void AProjectile::DelayedDestroy()
{
	Destroy();
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

