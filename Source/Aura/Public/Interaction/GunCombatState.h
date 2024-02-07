// TeethOfTheWind - Copyright Kerem Avcil 2022

#pragma once



/*********************
 *  GUN COMBAT STATE ENUM
 *********************/
UENUM(BlueprintType)
enum class EGunCombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	
	ECS_MAX UMETA(DisplayName = "DefaultMAX"),
	
};
