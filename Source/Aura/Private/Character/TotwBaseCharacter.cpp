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

	// Character Collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
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

FOnASCRegistered& ATotwBaseCharacter::GetOnASCRegisteredDelegate()
{
	return OnAscRegistered;
}

void ATotwBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}




void ATotwBaseCharacter::InitAbilityActorInfo()
{
	
}

void ATotwBaseCharacter::InitializeDefaultAttributes() const
{
	//ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	//ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
	//ApplyEffectToSelf(DefaultVitalAttributes, 1.f);
}

void ATotwBaseCharacter::AddCharacterAbilities()
{
	UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority()) return;
	
	AuraASC->AddCharacterAbilities(StartupAbilities);
	AuraASC->AddCharacterPassiveAbilities(StartupPassiveAbilities);
}

ECharacterClass ATotwBaseCharacter::GetCharacterClass_Implementation()
{
	return CharacterClass;
}
