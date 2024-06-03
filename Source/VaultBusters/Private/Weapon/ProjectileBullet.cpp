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
			bool bWasHeadShot = Hit.BoneName.ToString() == FString("head");
			const float DamageToCause = bWasHeadShot ? HeadShotDamage : Damage;
			
			UGameplayStatics::ApplyDamage(OtherActor, DamageToCause, OwnerController, this, UDamageType::StaticClass());
			if(AVBCharacter* DamageReceiver = Cast<AVBCharacter>(OtherActor))
			{
				OwnerController->ClientSetHUDImpactCrosshair();
				if(!bWasHeadShot)
				{
					OwnerController->ClientPlayHitGiven();
				}
				else
				{
					OwnerController->ClientPlayHeadshotGiven();
				}

				if(DamageReceiver)
				{
					DamageReceiver->MulticastPlayHitReceived(bWasHeadShot);
				}
			}
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
