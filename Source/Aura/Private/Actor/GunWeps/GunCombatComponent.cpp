// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/GunWeps/GunCombatComponent.h"
#include "Actor/GunWeps/GunWep.h"
#include "GameFramework/Character.h"
#include "Character/TotwBaseCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Player/TotwPlayerController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "Character/TotwGunFaction.h"
#include "Sound/SoundCue.h"



/**************
 DEFAULTS
***************/
UGunCombatComponent::UGunCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 300.f;
	// ...
}

void UGunCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGunCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UGunCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UGunCombatComponent, CarriedAmmo, COND_OwnerOnly); // only replicate carried ammo to local controlled player saving bandwidth
	DOREPLIFETIME(UGunCombatComponent, GunCombatState);
}

/**************
 BEGIN PLAY
***************/
void UGunCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GunCharacter)
	{
		GunCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (GunCharacter->GetFollowCamera())
		{
			DefaultFOV = GunCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (GunCharacter->HasAuthority())
		{
			InitialiseCarriedAmmo();
		}
	}
}

/**************
 TICK
***************/
void UGunCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GunCharacter && GunCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime);
		InterpFOV(DeltaTime);
	}
}


/********************
 FIRE BUTTON PRESSED
*********************/
void UGunCombatComponent::FireButtonPressed(bool bPressed)
{
		bFireButtonPressed = bPressed;
		if (bFireButtonPressed)
		{
			Fire();
		}
}

/**************
 FIRE FUNCTION
***************/
void UGunCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false; //single shot
		ServerFire(HitTarget);
		if (EquippedWeapon)
		{ //crosshair Spread per shot
			CrosshairShootingFactor = 1.0f;
		}
		StartFireTimer();
	}
}

/**********************
* AUTO FIRE TIMER START
**********************/
void UGunCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || GunCharacter == nullptr) return;
	GunCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UGunCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
		);
}

/**************************
* AUTO FIRE TIMER FINISHED
***************************/
void UGunCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

/**************
 SERVER FIRE
***************/
void UGunCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

/**************
 MULTICAST FIRE
***************/
void UGunCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;
	if (GunCharacter && GunCombatState == EGunCombatState::ECS_Unoccupied)
	{
			GunCharacter->PlayFireMontage(bAiming);
			EquippedWeapon->Fire(TraceHitTarget);
	}
}

/**************
 EQUIP WEAPON
***************/
void UGunCombatComponent::EquipWeapon(AGunWep* WeaponToEquip)
{
	if (GunCharacter == nullptr || WeaponToEquip == nullptr) return;
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = GunCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, GunCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(GunCharacter);
	EquippedWeapon->SetHUDAmmo();
	
	InitialiseCarriedAmmo();
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	
	if (EquippedWeapon->EquipSound)
	{// Wep equip sound cue play per wep{
		UGameplayStatics::PlaySoundAtLocation(
	this,
	EquippedWeapon->EquipSound,
	GunCharacter->GetActorLocation()

		);
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
	
	GunCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	GunCharacter->bUseControllerRotationYaw = true;
}

/**************
 RELOAD WEAPON
***************/
void UGunCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && GunCombatState != EGunCombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

/**************
 SERVER RELOAD
***************/
void UGunCombatComponent::ServerReload_Implementation()
{
	if (GunCharacter == nullptr || EquippedWeapon == nullptr) return;
	
	GunCombatState = EGunCombatState::ECS_Reloading;
	HandleReload();
}

/*****************
 FINISH RELOADING
******************/
void UGunCombatComponent::FinishReloading()
{
	if (GunCharacter == nullptr) return;
	if (GunCharacter->HasAuthority())
	{
		GunCombatState = EGunCombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

/*******************
 UPDATE AMMO VALUES
********************/
void UGunCombatComponent::UpdateAmmoValues()
{
	if (GunCharacter == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

/***********************
 ONREP COMBAT STATE
************************/
void UGunCombatComponent::OnRep_GunCombatState()
{
	switch (GunCombatState)
	{
	case EGunCombatState::ECS_Reloading:
		HandleReload();
		break;
	case EGunCombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

/*******************
 HANDLE RELOAD
********************/
void UGunCombatComponent::HandleReload()
{
	GunCharacter->PlayReloadMontage();
}

/*******************
 AMOUNT TO RELOAD
********************/
int32 UGunCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

/*******************
 ONREP EQUIPPED WEP
********************/
void UGunCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && GunCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = GunCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, GunCharacter->GetMesh());
		}
		GunCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		GunCharacter->bUseControllerRotationYaw = true;

		if (EquippedWeapon->EquipSound) // Wep equip sound cue play per wep
			{
			UGameplayStatics::PlaySoundAtLocation(
		this,
		EquippedWeapon->EquipSound,
		GunCharacter->GetActorLocation()
		);
			}
	}
}

/***********************
 TRACE UNDER CROSSHAIRS
*************************/
void UGunCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	} 

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition; 

		if (GunCharacter)
		{
			float DistanceToCharacter = (GunCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + CrosshairAdjust);
			//DrawDebugSphere(GetWorld(), Start, 16.f, 12, FColor::Red, false);
		}
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
			);
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairs_Interface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
		if (!TraceHitResult.bBlockingHit) //fix for snap to objects
		{
			TraceHitResult.ImpactPoint = End;
		}      
	}
}

/*******************
SET HUD CROSSHAIRS
*******************/
void UGunCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (GunCharacter == nullptr || GunCharacter->Controller == nullptr) return;

	Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<AGunFactionHud>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}
			//calculate crosshair spread
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, GunCharacter->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = GunCharacter->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
				WalkSpeedRange,
				VelocityMultiplierRange,
				Velocity.Size()
																		);
			
			if (GunCharacter->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			HUDPackage.CrosshairSpread =
				0.5f +
					CrosshairVelocityFactor +
					CrosshairInAirFactor -
						CrosshairAimFactor +
					CrosshairShootingFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

/**************
 INTERP FOV
***************/
void UGunCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;
	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (GunCharacter && GunCharacter->GetFollowCamera())
	{
		GunCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

/**************
 SET AIMING
***************/
void UGunCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if (GunCharacter)
	{
		GunCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

/******************
 SERVER SET AIMING
******************/
void UGunCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (GunCharacter)
	{
		GunCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

/**************
 CAN FIRE CHECK
***************/
bool UGunCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && GunCombatState == EGunCombatState::ECS_Unoccupied; //Allowing Weapon Fire lecture change
}

/***********************
 ON REP CARRIED AMMo
************************/
void UGunCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

/***********************
 INITIALISE CARRIED AMMO
************************/
void UGunCombatComponent::InitialiseCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}
