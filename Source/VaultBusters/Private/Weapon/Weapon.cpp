// Copyright Marco Freemantle

#include "Weapon/Weapon.h"

#include "Character/VBCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/VBPlayerController.h"
#include "Weapon/Casing.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
		WeaponMesh->OnComponentHit.AddDynamic(this, &AWeapon::OnHit);
	}
	
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
	DOREPLIFETIME(AWeapon, TotalAmmo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if(Owner == nullptr)
	{
		VBOwnerCharacter = nullptr;
		VBOwnerController = nullptr;
	}
	else
	{
		SetHUDAmmo();
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(AVBCharacter* VBCharacter = Cast<AVBCharacter>(OtherActor))
	{
		VBCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(AVBCharacter* VBCharacter = Cast<AVBCharacter>(OtherActor))
	{
		VBCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if(bCanPlayHitFloorSound)
	{
		MulticastPlayHitFloorSound();
	}
	bCanPlayHitFloorSound = false;
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if(EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EquipSound, GetActorLocation());
		}
		bCanPlayHitFloorSound = true;
		break;
	case EWeaponState::EWS_Dropped:
		if(HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if(DropSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DropSound, GetActorLocation());
		}
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if(DropSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DropSound, GetActorLocation());
		}
		break;
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if(FireEffectMuzzle && FireSound)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(FireEffectMuzzle, GetWeaponMesh(), FName("muzzle"), FVector(0.f), FRotator(0.f), EAttachLocation::Type::KeepRelativeOffset, true);
		UGameplayStatics::SpawnSoundAttached(FireSound, GetWeaponMesh());
	}
	if(CasingClass)
	{
		if(const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject")))
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			if(UWorld* World = GetWorld())
			{
				FRotator ExistingRotation = SocketTransform.GetRotation().Rotator();
				
				float RandomPitchOffset = FMath::FRandRange(-30.0f, 30.0f);
				FRotator RandomisedRotation = ExistingRotation;
				RandomisedRotation.Pitch += RandomPitchOffset;
				
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), RandomisedRotation);
			}
		}
	}
	SpendRound();
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	if(VBOwnerCharacter->IsLocallyControlled())
	{
		WeaponMesh->SetVisibility(true);
	}
	SetOwner(nullptr);
	VBOwnerCharacter = nullptr;
	VBOwnerController = nullptr;
}

void AWeapon::SetHUDAmmo()
{
	VBOwnerCharacter = VBOwnerCharacter == nullptr ? Cast<AVBCharacter>(GetOwner()) : VBOwnerCharacter;
	if(VBOwnerCharacter)
	{
		VBOwnerController = VBOwnerController == nullptr ? Cast<AVBPlayerController>(VBOwnerCharacter->Controller) : VBOwnerController;
		if(VBOwnerController)
		{
			VBOwnerController->SetHUDWeaponAmmo(Ammo);
			VBOwnerController->SetHUDWeaponTotalAmmo(TotalAmmo);
		}
	}
}

void AWeapon::SetHUDTotalAmmo()
{
	VBOwnerCharacter = VBOwnerCharacter == nullptr ? Cast<AVBCharacter>(GetOwner()) : VBOwnerCharacter;
	if(VBOwnerCharacter)
	{
		VBOwnerController = VBOwnerController == nullptr ? Cast<AVBPlayerController>(VBOwnerCharacter->Controller) : VBOwnerController;
		if(VBOwnerController)
		{
			VBOwnerController->SetHUDWeaponTotalAmmo(TotalAmmo);
		}
	}
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	TotalAmmo = TotalAmmo - AmmoToAdd;
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}

void AWeapon::OnRep_TotalAmmo()
{
	SetHUDTotalAmmo();
}

void AWeapon::MulticastPlayHitFloorSound_Implementation()
{
	if(HitFloorSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitFloorSound, GetActorLocation());
	}
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}


