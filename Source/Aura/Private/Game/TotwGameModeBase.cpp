// TeethOfTheWind - Copyright Kerem Avcil 2022


#include "Game/TotwGameModeBase.h"

#include "Character/TotwGunFaction.h"
#include "Player/TotwPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Player/TotwPlayerState.h"
#include "Game/TotwGameState.h"


namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}



/*************************
 * GAME MODE DEFaults
 ************************/
ATotwGameModeBase::ATotwGameModeBase()
{
	bDelayedStart = true; //puts game into waiting to start state, default pawn created till game starts
	
}

/*************************
 * BEGIN PLAY
 ************************/
void ATotwGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

/*************************
 * TICK
 ************************/
void ATotwGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
        		
        		    UWorld* World = GetWorld(); //Alternate method for RestartGame (if required)
                    	if (World)
                    	{
                    		bUseSeamlessTravel = true;
                    		World->ServerTravel(FString("/Game/Maps/ShooterMap2?listen"));
                    	}
        			//RestartGame(); // Original method. Should be called on both Client and Server.
        }
	}
}

/*************************
 * ON MATCH STATE SET
 ************************/

void ATotwGameModeBase::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATotwPlayerController* ShooterPlayer = Cast<ATotwPlayerController>(*It);
		if (ShooterPlayer)
		{
			ShooterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

/*******************
 * PLAYER ELIMINATED
 *******************/

void ATotwGameModeBase::PlayerEliminated(class ATotwGunFaction* ElimmedCharacter, class ATotwPlayerController* VictimController, ATotwPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ATotwPlayerState* AttackerPlayerState = AttackerController ? Cast<ATotwPlayerState>(AttackerController->PlayerState) : nullptr;
	ATotwPlayerState* VictimPlayerState = VictimController ? Cast<ATotwPlayerState>(VictimController->PlayerState) : nullptr;

	ATotwGameState* Shooter_GS = GetGameState<ATotwGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && Shooter_GS)
	{
		AttackerPlayerState->AddToScore(1.f);
		Shooter_GS->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
}


/*******************
 * REQUEST RESPAWN
 *******************/
void ATotwGameModeBase::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
		{
			ElimmedCharacter->Reset();
			ElimmedCharacter->Destroy();
		}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}
