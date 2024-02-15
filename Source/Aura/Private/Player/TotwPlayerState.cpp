// Copyright Kerem Avcil.


#include "Player/TotwPlayerState.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Character/TotwGunFaction.h"
#include "Net/UnrealNetwork.h"

ATotwPlayerState::ATotwPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	
	AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet"); 
	
	NetUpdateFrequency = 100.f;
}

void ATotwPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATotwPlayerState, Level);
	DOREPLIFETIME(ATotwPlayerState, XP);
	DOREPLIFETIME(ATotwPlayerState, AttributePoints);
	DOREPLIFETIME(ATotwPlayerState, AbilityPoints);
	DOREPLIFETIME(ATotwPlayerState, Defeats);
}

UAbilitySystemComponent* ATotwPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


void ATotwPlayerState::AddToXP(int32 InXP)
{
	XP += InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void ATotwPlayerState::OnRep_AttributePoints(int32 OldAttributePoints)
{
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void ATotwPlayerState::OnRep_AbilityPoints(int32 OldAbilityPoints)
{
	OnAbilityPointsChangedDelegate.Broadcast(AbilityPoints);
}

void ATotwPlayerState::AddToAttributePoints(int32 InPoints)
{
	AttributePoints += InPoints;
	OnAttributePointsChangedDelegate.Broadcast(AttributePoints);
}

void ATotwPlayerState::AddToAbilityPoints(int32 InPoints)
{
	AbilityPoints += InPoints;
	OnAbilityPointsChangedDelegate.Broadcast(AbilityPoints);
}

void ATotwPlayerState::AddToLevel(int32 InLevel)
{
	Level += InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}

void ATotwPlayerState::SetXP(int32 InXP)
{
	XP = InXP;
	OnXPChangedDelegate.Broadcast(XP);
}

void ATotwPlayerState::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnLevelChangedDelegate.Broadcast(Level);
}


void ATotwPlayerState::OnRep_Level(int32 OldLevel)
{
	OnLevelChangedDelegate.Broadcast(Level);
}


void ATotwPlayerState::OnRep_XP(int32 OldXP)
{
	OnXPChangedDelegate.Broadcast(XP);
}

// *** SHOOTER Player State ***




/************************
 *   ADD TO SCORE (Server)
 ************************/
void ATotwPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	GunCharacter = GunCharacter == nullptr ? Cast<ATotwGunFaction>(GetPawn()) : GunCharacter;
	if (GunCharacter)
	{
		Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}


/************************
 *   ONREP SCORE (Client)
 ************************/
void ATotwPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	GunCharacter = GunCharacter == nullptr ? Cast<ATotwGunFaction>(GetPawn()) : GunCharacter;
	if (GunCharacter)
	{
		Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

/************************
 *   ONREP DEFEATS
 ************************/
void ATotwPlayerState::OnRep_Defeats()
{
	GunCharacter = GunCharacter == nullptr ? Cast<ATotwGunFaction>(GetPawn()) : GunCharacter;
	if (GunCharacter)
	{
		Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

/************************
 *   ADD TO DEFEATS
 ************************/
void ATotwPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	GunCharacter = GunCharacter == nullptr ? Cast<ATotwGunFaction>(GetPawn()) : GunCharacter;
	if (GunCharacter)
	{
		Controller = Controller == nullptr ? Cast<ATotwPlayerController>(GunCharacter->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}