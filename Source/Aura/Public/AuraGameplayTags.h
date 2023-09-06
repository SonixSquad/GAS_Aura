// Copyright Kerem Avcil.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"


/**
 *  AuraGameplayTags
 *
 *  Singleton containing native gameplay tags
 */

struct FAuraGameplayTags
{
public:
	static const FAuraGameplayTags& Get() { return GameplayTags;}
	static void InitializeNativeGameplayTags();

	//Primary Atts
	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Resilience;
	FGameplayTag Attributes_Primary_Vigor;
	
	//Secondary Atts
	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxMana;

	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	FGameplayTag Attributes_Secondary_BlockChance;
	FGameplayTag Attributes_Secondary_CritHitChance;
	FGameplayTag Attributes_Secondary_CritHitDamage;
	FGameplayTag Attributes_Secondary_CritHitResist;
	FGameplayTag Attributes_Secondary_HealthRegen;
	FGameplayTag Attributes_Secondary_ManaRegen;
	

	// Vital Atts
	//FGameplayTag Attributes_Vital_Health;
	//FGameplayTag Attributes_Vital_Mana;

	


	
protected:



private:
	static FAuraGameplayTags GameplayTags;

 
};
