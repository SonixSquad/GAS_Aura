// Copyright Kerem Avcil

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbilities/AuraDamageGameplayAbility.h"
#include "TotwHitScanAbility.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UTotwHitScanAbility : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void StoreCrosshairInfo(const FHitResult& HitScanResult);

	UFUNCTION(BlueprintCallable)
	void StoreOwnerVariables();

	UFUNCTION(BlueprintCallable)
	void HitScanTarget(const FVector& HitScanTargetLocation);
	
	UFUNCTION(BlueprintImplementableEvent)
	void PrimaryTargetDied(AActor* DeadActor);

protected:

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	FVector CrosshairHitLocation;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<AActor> CrosshairHitActor;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<APlayerController> OwnerPlayerController;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	int32 MaxNumShockTargets = 5;
};