// TeethOfTheWind - Copyright Kerem Avcil 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Player/TotwPlayerState.h"
#include "TotwGameState.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API ATotwGameState : public AGameState
{
	GENERATED_BODY()
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ATotwPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<ATotwPlayerState*> TopScoringPlayers;

private:

	float TopScore = 0.f;
};