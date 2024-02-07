// TeethOfTheWind - Copyright Kerem Avcil 2022

#pragma once


/********************
 * WEAPON TYPES ENUM
 ********************/
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};