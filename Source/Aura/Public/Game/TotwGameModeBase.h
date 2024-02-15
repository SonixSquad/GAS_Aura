// TeethOfTheWind - Copyright Kerem Avcil 2024

#pragma once

#include "Player/TotwPlayerController.h"
#include "Character/TotwGunFaction.h"
#include "Kismet/GameplayStatics.h"
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TotwGameModeBase.generated.h"

namespace MatchState
{
	extern AURA_API const FName Cooldown; //Match duration reached, display Winner, begin cooldown timer
}
/**
 * 
 */
UCLASS()
class AURA_API ATotwGameModeBase : public AGameMode
{
	GENERATED_BODY()
	
public:
	ATotwGameModeBase();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ATotwGunFaction* ElimmedCharacter, class ATotwPlayerController* VictimController, ATotwPlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* ElimmedCharacter, AController* ElimmedController);

	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;
	
	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;


	
private:
	float CountdownTime = 0.f;

public:
	//	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
