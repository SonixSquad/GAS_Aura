// Copyright Kerem Avcil 2024

#pragma once

#include "CoreMinimal.h"
#include "Actor/GunWeps/GunWep.h"
#include "Components/ActorComponent.h"
#include "UI/HUD/GunFactionHud.h"
#include "Actor/WepTypes.h"
#include "Actor/GunWeps/GunCombatState.h"
#include "GunCombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UGunCombatComponent: public UActorComponent
{
	GENERATED_BODY()

public:	
	/**************
		DEFAULTS
	***************/
	UGunCombatComponent();
	friend class ATotwGunFaction;
	
	/**************
		TICK
	***************/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	

	void EquipWeapon(class AGunWep* WeaponToEquip);
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void FireButtonPressed(bool bPressed);
	
protected:
	
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	void Fire();

	
	
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);
	
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);
	
	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();
	
private:
	UPROPERTY()
	class ATotwGunFaction* GunCharacter;
	
	UPROPERTY()
	class ATotwPlayerController* Controller;
	
	UPROPERTY()
	class AGunFactionHud* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AGunWep* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;
	
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	
	
	/**************
	 HUD & CROSSHAIRS
	***************/

	// adjust distance to avoid linetrace clash
	UPROPERTY(EditAnywhere, Category = Combat)
	float CrosshairAdjust = 100.f;
	
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;
	FHUDPackage HUDPackage;
	
	/**************
	 ZOOMED FOV
	***************/

	// FOV when not aiming set to cameras base FOV in beginplay
	UPROPERTY(EditAnywhere, Category = Combat)
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;
	
	void InterpFOV(float DeltaTime);

	/**************
	AUTOMATIC FIRE
	***************/
	FTimerHandle FireTimer;
	


	bool bCanFire = true;
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();
	
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	
	UFUNCTION()
	void OnRep_CarriedAmmo(); //carried ammo for currently equipped weapon

	TMap<EWeaponType, int32> CarriedAmmoMap;  //map data structure

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UFUNCTION()
	void InitialiseCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_GunCombatState)
	EGunCombatState GunCombatState = EGunCombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_GunCombatState();

	void UpdateAmmoValues();
	
public:

		
};



