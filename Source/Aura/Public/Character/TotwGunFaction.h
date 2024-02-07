// Copyright Kerem Avcil.

#pragma once

#include "CoreMinimal.h"
#include "Actor/GunWeps/GunWep.h"
#include "GameFramework/Character.h"
#include "Character/TotwBaseCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "TotwGunFaction.generated.h"

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

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	void PlayFireMontage(bool bAiming);
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	virtual void OnRep_Stunned() override;
	virtual void OnRep_Burned() override;

	void SetOverlappingWeapon(AGunWep* GunWep);
	bool IsWeaponEquipped();
	bool IsAiming();
private:
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
	
	virtual void InitAbilityActorInfo() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UGunCombatComponent* Combat;

	/*********************
		 * ANIMATION MONTAGES
		 ********************/
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	
};



