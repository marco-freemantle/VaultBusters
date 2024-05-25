// Copyright Marco Freemantle


#include "Weapon/ProjectileBullet.h"

#include "Character/VBCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VBPlayerController.h"

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if(OwnerCharacter)
	{
		AVBPlayerController* OwnerController = Cast<AVBPlayerController>(OwnerCharacter->Controller);
		if(OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
			if(Cast<AVBCharacter>(OtherActor))
			{
				OwnerController->ClientSetHUDImpactCrosshair();
				OwnerController->ClientPlayHitGiven();

				//TODO: Play headshot given sound for local player
			}
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
