// Copyright Kerem Avcil.


#include "Character/TotwGunFaction.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/TotwPlayerController.h"
#include "Player/TotwPlayerState.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UI/HUD/AuraHUD.h"

class AAuraHUD;

ATotwGunFaction::ATotwGunFaction()
{

	// Character Camera Setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;


	
	// Character Overhead Widget
	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	

	// Character Movement
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	
	// Character Weapon
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);



	// TODO: create new GunFaction class to replace Elementalist for this faction
	CharacterClass = ECharacterClass::Elementalist; 
}

void ATotwBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATotwBaseCharacter, OverlappingWeapon, COND_OwnerOnly);
	//DOREPLIFETIME(ATotwBaseCharacter, Health);
	DOREPLIFETIME(ATotwBaseCharacter, bDisableGameplay);
	DOREPLIFETIME(ATotwBaseCharacter, bIsStunned);
	DOREPLIFETIME(ATotwBaseCharacter, bIsBurned);
	//DOREPLIFETIME(ATotwBaseCharacter, bIsBeingShocked);
}

void ATotwGunFaction::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//- Init Ability actor info for Server
	InitAbilityActorInfo();
	AddCharacterAbilities();         
}

void ATotwGunFaction::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//- Init Ability actor info for Client
	InitAbilityActorInfo();
}

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


void ATotwGunFaction::OnRep_Stunned()
{
	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
	{
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
		FGameplayTagContainer BlockedTags;
		BlockedTags.AddTag(GameplayTags.Player_Block_CursorTrace);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputHeld);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputPressed);
		BlockedTags.AddTag(GameplayTags.Player_Block_InputReleased);
		if (bIsStunned)
		{
			AuraASC->AddLooseGameplayTags(BlockedTags);
			StunDebuffComponent->Activate();
		}
		else
		{
			AuraASC->RemoveLooseGameplayTags(BlockedTags);
			StunDebuffComponent->Deactivate();
		}
	}
}

void ATotwGunFaction::OnRep_Burned()
{
	if (bIsBurned)
	{
		BurnDebuffComponent->Activate();
	}
	else
	{
		BurnDebuffComponent->Deactivate();
	}
}

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

bool ATotwGunFaction::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ATotwGunFaction::IsAiming()
{
	return (Combat && Combat->bAiming);
}


void ATotwGunFaction::InitAbilityActorInfo()
{
	ATotwPlayerState* TotwPlayerState = GetPlayerState<ATotwPlayerState>();
	check(TotwPlayerState);
	TotwPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(TotwPlayerState, this);
	Cast<UAuraAbilitySystemComponent>(TotwPlayerState->GetAbilitySystemComponent())->AbilityActorInfoSet();
	AbilitySystemComponent = TotwPlayerState->GetAbilitySystemComponent();
	AttributeSet = TotwPlayerState->GetAttributeSet();
	OnAscRegistered.Broadcast(AbilitySystemComponent);
	AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Debuff_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ATotwGunFaction::StunTagChanged);
	
	if (ATotwPlayerController* TotwPlayerController = Cast<ATotwPlayerController>(GetController()))
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(TotwPlayerController->GetHUD()))
		{
			AuraHUD->InitOverlay(TotwPlayerController, TotwPlayerState, AbilitySystemComponent, AttributeSet);
		}
	}
	InitializeDefaultAttributes();
}


