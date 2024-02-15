// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Actor/WepTypes.h"
#include "GunWep.generated.h"

UENUM(BlueprintType)
enum class EGunWepState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayNAme = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EGunType : uint8
{
	E_Single	UMETA(DisplayName = "Single"),
	E_Burst		UMETA(DisplayName = "Burst"),
	E_Auto		UMETA(DisplayName = "Automatic")
};

UENUM(BlueprintType)
enum class EGunBulletType : uint8
{
	E_Projectile	UMETA(DisplayName = "Projectile"),
	E_Hitscan		UMETA(DisplayName = "Hitscan")
};

UCLASS()
class AURA_API AGunWep : public AActor
{
	GENERATED_BODY()
	
public:
	
	/*********************
	 DEFAULTS
	 ********************/
	AGunWep();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	void SetHUDAmmo();
	
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);

	void Dropped();

	void AddAmmo(int32 AmmoToAdd);

	/*********************
	 CROSSHAIR TEXTURES
	 ********************/

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsBottom;

	/*********************
	 WEAPON ZOOM
	 ********************/

	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*********************
	 WEAPON FIRE
	 ********************/
	
	UPROPERTY (EditAnywhere, Category = Combat)
	float FireDelay = .15f;
	
	UPROPERTY (EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY (EditAnywhere)
	class USoundCue* EquipSound;
	
	
protected:
	
	/*********************
	 BEGIN PLAY
	 ********************/
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	void OnSPhereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
		);
	
public:
	
	/*******
	 * TICK
	 ******/
	

private:
	UPROPERTY(VisibleAnywhere, Category = "GunFactionWeps")
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "GunFactionWeps")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_GunWepState, VisibleAnywhere, Category = "GunFactionWeps")
	EGunWepState GunWepState;

	UFUNCTION()
	void OnRep_GunWepState();

	UPROPERTY(VisibleAnywhere, Category = "GunFactionWeps")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "GunFactionWeps")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;	

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;

	UFUNCTION()
	void OnRep_Ammo();

	
	void SpendRound();
	
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY()
	class ATotwGunFaction* GunFactionChar;
	UPROPERTY()
	class ATotwPlayerController* GunFactionController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
	
public:
	void SetGunWepState(EGunWepState State);
	//FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; } //debug only
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	
	bool IsEmpty();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFactionWeps")
	EGunBulletType GunBulletType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFactionWeps")
	EGunType GunType;
};

