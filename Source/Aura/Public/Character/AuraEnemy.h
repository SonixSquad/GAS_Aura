// Copyright Kerem Avcil.

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "AuraEnemy.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API AAuraEnemy : public AAuraCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()
public:
	AAuraEnemy();

	/** Enemy Interface */
	virtual void HighLightActor() override;
	virtual void UnHighLightActor() override;
	/** End Enemy Interface */
	
protected:
	
	virtual void BeginPlay() override;

	
};
