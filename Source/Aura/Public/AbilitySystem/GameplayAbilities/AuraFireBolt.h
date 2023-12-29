// Copyright Kerem Avcil

#pragma once

#include "CoreMinimal.h"
#include "AuraProjectileAbility.h"
#include "AbilitySystem/GameplayAbilities/AuraProjectileAbility.h"
#include "AuraFireBolt.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraFireBolt : public UAuraProjectileAbility
{
	GENERATED_BODY()
public:
	virtual FString GetDescription(int32 Level) override;
	virtual FString GetNextLevelDescription(int32 Level) override;
};