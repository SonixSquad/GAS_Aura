// Copyright Kerem Avcil.

#pragma once

// *** SHOOTER includes ***
#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "Actor/GunWeps/GunWep.h"
#include "GameFramework/Character.h"
#include "Actor/GunWeps/GunCombatComponent.h"
#include "Animation/TurnInPlace.h"
//#include "MP_Shooter/Interfaces/InteractWithCrosshairs_Interface.h"
#include "Actor/GunWeps/GunCombatState.h"

// *** AURA includes ***
#include "Character/TotwBaseCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "TotwGunFaction.generated.h"

//Aura forward declarations
class UNiagaraComponent;
class UCameraComponent;
class USpringArmComponent;
/**
 * 
 */
UCLASS()
class AURA_API ATotwGunFaction : public ATotwBaseCharacter, public IPlayerInterface
{
	GENERATED_BODY()
public:
	ATotwGunFaction();

	// SHooter public
	virtual void Tick(float DeltaTime) override;
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	virtual void OnRep_ReplicatedMovement() override;
	void UpdateHUDHealth();

	// poll for any relevant classes and initialize HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);

	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
// End SHooter public
	
protected:

	//Shooter protected
	/***************************
	 * BEGIN PLAY
	 **************************/
	virtual void BeginPlay() override;

	//void MoveForward(float Value);
	//void MoveRight(float Value);
	//void Turn(float Value);
	//void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void CalculateAO_Pitch();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();

	
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	// *** END Shooter protected ***
	
private:

	// *** SHOOTER private ***
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AGunWep* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AGunWep* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UGunCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float Interp_AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurnInPlace TurningInPlace;
	void TurnInPlace(float Deltatime);


	/*********************
	 * ANIMATION MONTAGES
	 ********************/
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage_GunFaction;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	
	
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/***************************
	 * PLAYER HEALTH
	 **************************/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()	
	void OnRep_Health();
	
	UPROPERTY()
	class ATotwPlayerController* TotwPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	/************************
	 *   DISSOLVE EFFECT
	 ************************/

	UPROPERTY(VisibleANywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();
	
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance_Gun;

	/************************
	 *   ELIM BOT / DESPAWN
	 ************************/

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ATotwPlayerState* TotwPlayerState;
	// END Shooter private
	
public:
	
	void SetOverlappingWeapon(AGunWep* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AGunWep* GetEquippedWeapon();
	FORCEINLINE ETurnInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	FORCEINLINE bool IsElimmed() const { return bElimmed; }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const {  return MaxHealth; }

	EGunCombatState GetGunCombatState() const;
	FORCEINLINE UGunCombatComponent* GetCombat() const { return Combat; }

	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
};



