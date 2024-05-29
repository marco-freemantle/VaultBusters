// Copyright Marco Freemantle

#include "Weapon/ProjectileShotgun.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	if(!HasAuthority()) return;
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName(FName("muzzle"));
	if(MuzzleSocket && ProjectileClass && InstigatorPawn)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		
		float Distance = ToTarget.Length();
		TArray<FVector> HitTargetLocations;
		TArray<FRotator> HitTargetRotations;
		
		for(int i = 0; i < NumShellsPerShot; ++i)
		{
			HitTargetLocations.Add(HitTarget +
					FVector(FMath::RandRange((-Distance / 10), (Distance / 10)),
					FMath::RandRange((-Distance / 10), (Distance / 10)),
					FMath::RandRange((-Distance / 10), (Distance / 10))));
		}
		for(int i = 0; i < HitTargetLocations.Num() ; ++i)
		{
			HitTargetRotations.Add((HitTargetLocations[i] - SocketTransform.GetLocation()).Rotation());
		}
		if(UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;

			for(int i = 0; i < HitTargetRotations.Num() ; ++i)
			{
				World->SpawnActor<AProjectile>(ProjectileClass, SocketTransform.GetLocation(), HitTargetRotations[i], SpawnParams);
			}
		}
	}
}
