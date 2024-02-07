// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/GunWeps/GunWep.h"

#include "VectorUtil.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/TotwGunFaction.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Actor/GunWeps/Casing.h"
#include "Engine/SkeletalMeshSocket.h"

#include "Player/TotwPlayerController.h"

/*************************
 *    DEFAULTS
 *************************/
AGunWep::AGunWep()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent	);
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

/*************************
 *    BEGIN PLAY
 *************************/
void AGunWep::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AGunWep::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AGunWep::OnSPhereEndOverlap);
	}
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}


/*************************
 *    TICK
 *************************/
void AGunWep::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


/*********************************
 *  GET LIFETIME REPLICATED PROPS
 *********************************/
void AGunWep::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AGunWep, WeaponState);
	DOREPLIFETIME(AGunWep, Ammo);
}


/*************************
 *    ON SPHERE OVERLAP
 *************************/
void AGunWep::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ATotwGunFaction* TotwGunFaction = Cast<ATotwGunFaction>(OtherActor);
	if (TotwGunFaction)
	{
		TotwGunFaction->SetOverlappingWeapon(this);
	}
}


/*************************
 *  ON SPHERE END OVERLAP
 *************************/
void AGunWep::OnSPhereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ATotwGunFaction* TotwGunFaction = Cast<ATotwGunFaction>(OtherActor);
	if (TotwGunFaction)
	{
		TotwGunFaction->SetOverlappingWeapon(nullptr);
	}
}

/*************************
 *  SET HUD AMMO
 *************************/
void AGunWep::SetHUDAmmo()
{
	ShooterOwnerChar = ShooterOwnerChar == nullptr ? Cast<ATotwGunFaction>(GetOwner()) : ShooterOwnerChar;
	if (ShooterOwnerChar)
	{
		ShooterOwnerController = ShooterOwnerController == nullptr ? Cast<ATotwPlayerController>(ShooterOwnerChar->Controller) : ShooterOwnerController;
		if (ShooterOwnerController)
		{
			ShooterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

/*************************
 *    SPEND ROUND
 *************************/
void AGunWep::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

/*************************
 *    ON REP AMMO
 *************************/
void AGunWep::OnRep_Ammo()
{
	ShooterOwnerChar = ShooterOwnerChar == nullptr ? Cast<ATotwGunFaction>(GetOwner()) : ShooterOwnerChar;
	SetHUDAmmo();
}

/*************************
 *    ON REP OWNER
 *************************/
void AGunWep::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		ShooterOwnerChar = nullptr;
		ShooterOwnerController = nullptr;
	}
	else
	{
		SetHUDAmmo();
	}
}

/*************************
 *    SET WEAPON STATE
 *************************/
void AGunWep::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//turn collision and physics OFF when wep is EQUIPPED
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		} //turn collision and physics ON when wep is DROPPED
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}



/*************************
 *    ONREP WEAPON STATE
 *************************/
void AGunWep::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case  EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}


/*************************
 *  SHOW PICKUP WIDGET
 *************************/
void AGunWep::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

/*************************
 *    FIRE
 *************************/
void AGunWep::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
											);
			}
		}
	}
	SpendRound();
}

/*************************
 *    IS EMPTY CHECK (AMMO)
 *************************/
bool AGunWep::IsEmpty()
{
	return Ammo <= 0;
}

/*************************
 *    DROPPED
 *************************/
void AGunWep::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);

	ShooterOwnerChar = nullptr;
	ShooterOwnerController = nullptr;
}


/*************************
 *    AMMO TO ADD 
 *************************/
void AGunWep::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

