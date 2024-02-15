// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/TotwBaseAnimInstance.h"
#include "Character/TotwGunFaction.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Actor/GunWeps/GunWep.h"
#include "Actor/GunWeps/GunCombatState.h"

void UTotwBaseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	TotwGunFaction = Cast<ATotwGunFaction>(TryGetPawnOwner());
}

void UTotwBaseAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (TotwGunFaction == nullptr)
	{
		TotwGunFaction = Cast<ATotwGunFaction>(TryGetPawnOwner());
		
	}
	if (TotwGunFaction == nullptr) return;

	
	
	FVector Velocity = TotwGunFaction->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = TotwGunFaction->GetCharacterMovement()->IsFalling();

	bIsAccelerating = TotwGunFaction->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;

	bWeaponEquipped = TotwGunFaction->IsWeaponEquipped();
	EquippedWeapon = TotwGunFaction->GetEquippedWeapon();

	bIsCrouched = TotwGunFaction->bIsCrouched;

	bAiming = TotwGunFaction->IsAiming();

	TurningInPlace = TotwGunFaction->GetTurningInPlace();

	bRotateRootBone = TotwGunFaction->ShouldRotateRootBone();

	bElimmed = TotwGunFaction->IsElimmed();


	//Offset Yaw for Strafe movement
	FRotator AimRotation = TotwGunFaction->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(TotwGunFaction->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = TotwGunFaction->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
	
	AO_Yaw = TotwGunFaction->GetAO_Yaw();
	AO_Pitch = TotwGunFaction->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && TotwGunFaction->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		TotwGunFaction->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));


		//only do this for locally controlled
		if (TotwGunFaction->IsLocallyControlled())
		{
			// align gun direction with crosshair location
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - TotwGunFaction->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
		
		
		//get muzzleflash socket to draw debug line
/**
		FTransform MuzzleTipTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), TotwGunFaction->GetHitTarget(), FColor::Orange);
**/		
	}

	bUseFABRIK = TotwGunFaction->GetGunCombatState() != EGunCombatState::ECS_Reloading; //ignore FABRIK on reloading
	bUseAimOffsets = TotwGunFaction->GetGunCombatState() != EGunCombatState::ECS_Reloading && !TotwGunFaction->GetDisableGameplay(); // ignore AImOffset on reloading 
	bTransformRightHand = TotwGunFaction->GetGunCombatState() != EGunCombatState::ECS_Reloading && !TotwGunFaction->GetDisableGameplay(); // ignore righthand transform when reloading
}
