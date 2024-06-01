// Copyright Marco Freemantle


#include "Pickups/HealthPickup.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/VBCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "VBComponents/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	AVBCharacter* VBCharacter = Cast<AVBCharacter>(OtherActor);
	if (VBCharacter && VBCharacter->GetHealth() < VBCharacter->GetMaxHealth())
	{
		UBuffComponent* Buff = VBCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
		Destroy();
	}
}

void AHealthPickup::Destroyed()
{
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	Super::Destroyed();
}