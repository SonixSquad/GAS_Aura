// Copyright Kerem Avcil.

#include "Character/TotwBaseCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Aura/Aura.h"
#include "Kismet/GameplayStatics.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"




/**************
* INITIALISATION
***************/
ATotwBaseCharacter::ATotwBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);


	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATotwBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	EffectAttachComponent->SetWorldRotation(FRotator::ZeroRotator);
}

UAbilitySystemComponent* ATotwBaseCharacter::GetAbilitySystemComponent() //const
	{
		return AbilitySystemComponent;
	}


UAnimMontage* ATotwBaseCharacter::GetHitReactMontage_Implementation()
	{
		return HitReactMontage;
	}

void ATotwBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}


void ATotwBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);

		//DOREPLIFETIME_CONDITION(ATotwBaseCharacter, OverlappingWeapon, COND_OwnerOnly);
		//DOREPLIFETIME(ATotwBaseCharacter, Health);
		//DOREPLIFETIME(ATotwBaseCharacter, bDisableGameplay);
		//DOREPLIFETIME(ATotwBaseCharacter, bIsStunned);
		//DOREPLIFETIME(ATotwBaseCharacter, bIsBurned);
		//DOREPLIFETIME(ATotwBaseCharacter, bIsBeingShocked);
	}

void ATotwBaseCharacter::InitAbilityActorInfo()
{
}



