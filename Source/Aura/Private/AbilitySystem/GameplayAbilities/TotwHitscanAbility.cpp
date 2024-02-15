// Copyright Kerem Avcil


#include "AbilitySystem/GameplayAbilities/TotwHitScanAbility.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

void UTotwHitScanAbility::StoreCrosshairInfo(const FHitResult& HitScanResult)
{
	if (HitScanResult.bBlockingHit)
	{
		CrosshairHitLocation = HitScanResult.ImpactPoint;
		CrosshairHitActor = HitScanResult.GetActor();
	}
	else
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UTotwHitScanAbility::StoreOwnerVariables()
{
	if (CurrentActorInfo)
	{
		OwnerPlayerController = CurrentActorInfo->PlayerController.Get();
		OwnerCharacter = Cast<ACharacter>(CurrentActorInfo->AvatarActor);
	}
}

void UTotwHitScanAbility::HitScanTarget(const FVector& HitScanTargetLocation)
{
	check(OwnerCharacter);
	if (OwnerCharacter->Implements<UCombatInterface>())
	{
		if (USkeletalMeshComponent* Weapon = ICombatInterface::Execute_GetWeapon(OwnerCharacter))
		{
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(OwnerCharacter);
			FHitResult HitScanResult;
			const FVector SocketLocation = Weapon->GetSocketLocation(FName("TipSocket"));
			UKismetSystemLibrary::SphereTraceSingle(
				OwnerCharacter,
				SocketLocation,
				HitScanTargetLocation,
				10.f,
				TraceTypeQuery1,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::None,
				HitScanResult,
				true);

			if (HitScanResult.bBlockingHit)
			{
				CrosshairHitLocation = HitScanResult.ImpactPoint;
				CrosshairHitActor = HitScanResult.GetActor();
			}
		}
	}
	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(CrosshairHitActor))
	{
		if (!CombatInterface->GetOnDeathDelegate().IsAlreadyBound(this, &UTotwHitScanAbility::PrimaryTargetDied))
		{
			CombatInterface->GetOnDeathDelegate().AddDynamic(this, &UTotwHitScanAbility::PrimaryTargetDied);
		}
	}
}
