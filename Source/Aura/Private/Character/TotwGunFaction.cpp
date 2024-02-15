// Copyright Kerem Avcil.


#include "Character/TotwGunFaction.h"

// *** SHOOTER includes ***
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Actor/GunWeps/GunWep.h"
#include "Actor/GunWeps/GunCombatComponent.h"

#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/TotwBaseAnimInstance.h"
#include "Aura/Aura.h"
#include "Player/TotwPlayerController.h"
#include "Game/TotwGameModeBase.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/TotwPlayerState.h"
#include "Actor/WepTypes.h"

// *** AURA includes ***
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "Aura/Aura.h"
#include "UI/HUD/AuraHUD.h"

class AAuraHUD;

ATotwGunFaction::ATotwGunFaction()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UGunCombatComponent>(TEXT("GunCombatComponent"));
	Combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	
	TurningInPlace = ETurnInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

/**************
* TICK
***************/
void ATotwGunFaction::Tick(float DeltaTime)
{

	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

/******************************************************************************
* OLD METHOD INPUT BINDS - (NEW METHOD ON Player Controller via EnhancedInput)
******************************************************************************/
/**
void ATotwGunFaction::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ATotwGunFaction::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ATotwGunFaction::MoveRight);
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &ATotwGunFaction::Turn);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &ATotwGunFaction::LookUp);

	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ATotwGunFaction::EquipButtonPressed);
	//PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ATotwGunFaction::CrouchButtonPressed);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ATotwGunFaction::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ATotwGunFaction::AimButtonReleased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATotwGunFaction::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ATotwGunFaction::FireButtonReleased);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ATotwGunFaction::ReloadButtonPressed);
}
	**/

/***************************
* LIFETIME REPLICATED PROPS
****************************/
void ATotwGunFaction::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATotwGunFaction, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ATotwGunFaction, Health);
	DOREPLIFETIME(ATotwGunFaction, bDisableGameplay);
}

void ATotwGunFaction::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
	
/******************
* PLAY FIRE MONTAGE
*******************/
void ATotwGunFaction::PlayFireMontage(bool bAiming)
{

	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming  ? FName("ADS Rifle") : FName("HIP Rifle");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

/******************
* PLAY RELOAD MONTAGE
*******************/
void ATotwGunFaction::PlayReloadMontage()
{
	
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			//insert additional weapon types here
			
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ATotwGunFaction::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

/**************************
* ONREP REPLICATED MOVEMENT
***************************/
void ATotwGunFaction::OnRep_ReplicatedMovement()
{

	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

/*****************
* UPDATE HUD HEALTH
******************/

void ATotwGunFaction::UpdateHUDHealth()
{
	TotwPlayerController = TotwPlayerController == nullptr ? Cast<ATotwPlayerController>(Controller) : TotwPlayerController;
	if (TotwPlayerController)
	{
		TotwPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}


/*****************
* POLL INIT
******************/

void ATotwGunFaction::PollInit()
{
	if (TotwPlayerState == nullptr)
	{
		TotwPlayerState = GetPlayerState<ATotwPlayerState>();
		if (TotwPlayerState)
		{
			TotwPlayerState->AddToScore(0.f);
			TotwPlayerState->AddToDefeats(0);
		}
	}
}


/**************
* ROTATE IN PLACE
***************/
void ATotwGunFaction::RotateInPlace(float DeltaTime)
{

	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

/*****************
* Player Elimmed
******************/
void ATotwGunFaction::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
	ElimTimer,
	this,
	&ATotwGunFaction::ElimTimerFinished,
	ElimDelay
	);
}

/**************************
* Player Multicast Elimmed
****************************/

void ATotwGunFaction::MulticastElim_Implementation()
{
	if (TotwPlayerController)
	{
		TotwPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	if (DissolveMaterialInstance_Gun)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance_Gun, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.48f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable character movement
	
	bDisableGameplay = true; //set input controllers to be disabled
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/**************************
	*	Spawn ElimBot
	****************************/
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			 ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
	this,
	ElimBotSound,
	GetActorLocation()
		);
	}
}

/**************************
* Destroy ElimBot (replicated)
****************************/
void ATotwGunFaction::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ATotwGameModeBase* Shooter_Gm = Cast<ATotwGameModeBase>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = Shooter_Gm && Shooter_Gm->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

/**************
* BEGIN PLAY
***************/
void ATotwGunFaction::BeginPlay()
{

	Super::BeginPlay();

	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ATotwGunFaction::ReceiveDamage);
	}

	
	if (Combat)
	{
		Combat->GunCharacter = this;
	}
}

/*************************
* BASIC MOVEMENT & TURNING
**************************/

/*************************
* MOVE FORWARD BACKWARD
**************************/
/**
void ATotwGunFaction::MoveForward(float Value)
{

	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction( FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}
**/
/*************************
* MOVE RIGHT LEFT
**************************/
/**
 *void ATotwGunFaction::MoveRight(float Value)
{

	if (bDisableGameplay) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction( FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}
**/
/*************************
* TURN RIGHT LEFT
**************************/
/**
void ATotwGunFaction::Turn(float Value)
{

	AddControllerYawInput(Value);
}
**/
/*************************
* LOOK UP DOWN
**************************/
/**
void ATotwGunFaction::LookUp(float Value)
{

	AddControllerPitchInput(Value);
}
**/
/**************
* EQUIP BUTTON
***************/
void ATotwGunFaction::EquipButtonPressed()
{

	if (bDisableGameplay) return;
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

/***********************
* CROUCH ACTION (UNUSED)
*************************/
void ATotwGunFaction::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

/**************************
* Reload Button PRESSED
****************************/
void ATotwGunFaction::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

/********************
* AIM BUTTON PRESSED
********************/
void ATotwGunFaction::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}
/********************
* AIM BUTTON RELEASED
********************/
void ATotwGunFaction::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

/********************
 CALCULATE AO_PITCH
*********************/
void ATotwGunFaction::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2d InRange(270.f, 360.f);
		FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

/**************
* AIM OFFSET
***************/
void ATotwGunFaction::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (Speed == 0.f && !bIsInAir) //Standing still not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurnInPlace::ETIP_NotTurning)
		{
			Interp_AO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) //running or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
	}
	CalculateAO_Pitch();
}

/**************
* SIMPROXIES
***************/
void ATotwGunFaction::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		return;
	}
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	
	//UE_LOG(LogTemp, Warning, TEXT("ProxyYaw: %f"), ProxyYaw);
	
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurnInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurnInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurnInPlace::ETIP_NotTurning;
}

/********************************
* FIRE BUTTON PRESSED / RELEASED
*********************************/
void ATotwGunFaction::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ATotwGunFaction::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

/*****************
* PLAY HIT MONTAGE
******************/
void ATotwGunFaction::PlayHitReactMontage()
{

	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage_GunFaction)
	{
		AnimInstance->Montage_Play(HitReactMontage_GunFaction);
		FName SectionName("FromFront");
		
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

/**************
* RECEIVE DAMAGE
***************/
void ATotwGunFaction::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		ATotwGameModeBase* TotwGameModeBase = GetWorld()->GetAuthGameMode<ATotwGameModeBase>();
		if (TotwGameModeBase)
		{
			TotwPlayerController = TotwPlayerController == nullptr ? Cast<ATotwPlayerController>(Controller) : TotwPlayerController;
			ATotwPlayerController* AttackerController = Cast<ATotwPlayerController>(InstigatorController);
			TotwGameModeBase->PlayerEliminated(this, TotwPlayerController, AttackerController);
		}
	}
}

/****************************************
* ONREP OVERLAPPING WEP (Floating labels)
*****************************************/
void ATotwGunFaction::OnRep_OverlappingWeapon(AGunWep* LastWeapon)
{

	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

/**************
* SERVER EQUIP
***************/
void ATotwGunFaction::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

/**************
* TURN IN PLACE
***************/
void ATotwGunFaction::TurnInPlace(float Deltatime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurnInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurnInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurnInPlace::ETIP_NotTurning)
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0.f, Deltatime, 4.f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

/**************
* HIDE CAMERA
***************/
void ATotwGunFaction::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

/****************
* CALCULATE SPEED
***************/
float ATotwGunFaction::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

/**************
* ONREP HEAlTH
***************/
void ATotwGunFaction::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

/**************************
* Player Elim Timer Finished
****************************/
void ATotwGunFaction::ElimTimerFinished()
{
	ATotwGameModeBase* TotwGameModeBase = GetWorld()->GetAuthGameMode<ATotwGameModeBase>();
	if (TotwGameModeBase)
	{
		TotwGameModeBase->RequestRespawn(this, Controller);
	}
}

/**************************
* UPDATE DISSOLVE MATERIAL
****************************/
void ATotwGunFaction::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("DISSOLVE"), DissolveValue );
	}
}

/**************************
* START DISSOLVE
****************************/
void ATotwGunFaction::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ATotwGunFaction::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}

	/**************************
	*  DISABLE MOVEMENT / INPUT
	****************************/
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (TotwPlayerController)
	{
		DisableInput(TotwPlayerController);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

/*********************
* SET OVERLAPPING WEP
**********************/
void ATotwGunFaction::SetOverlappingWeapon(AGunWep* GunWep)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = GunWep;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

/*****************
* IS WEP EQUIPPED
*****************/
bool ATotwGunFaction::IsWeaponEquipped()
{

	return (Combat && Combat->EquippedWeapon);
}

/**************
	* IS AIMING
	***************/
bool ATotwGunFaction::IsAiming()
{
	
	
	return (Combat && Combat->bAiming);
}

/******************
GET EQUIPPED WEP
******************/
AGunWep* ATotwGunFaction::GetEquippedWeapon()
{

	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

/***************
* GET HIT TARGET
***************/
FVector ATotwGunFaction::GetHitTarget() const
{

	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

/*****************
* GET GUN COMBAT STATE
******************/
EGunCombatState ATotwGunFaction::GetGunCombatState() const
{
	if (Combat == nullptr) return EGunCombatState::ECS_MAX;
	return Combat->GunCombatState;
}


/**************
* MULTICAST HIT
***************
void ATotwGunFaction::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}**/





































