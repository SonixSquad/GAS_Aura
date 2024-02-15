// Copyright Kerem Avcil 2024


#include "Player/TotwPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AIHelpers.h"
#include "AuraGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "UI/HUD/GunFactionHud.h"
#include "UI/HUD/GunCharacterOverlay.h"
#include "GameFramework/Character.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "Character/TotwGunFaction.h"
#include "Net/UnrealNetwork.h"
#include "Game/TotwGameModeBase.h"
#include "UI/Announce.h"
#include "Kismet/GameplayStatics.h"
#include "Actor/GunWeps/GunCombatComponent.h"
#include "Actor/GunWeps/GunWep.h"
#include "Game/TotwGameState.h"
#include "Components/SplineComponent.h"
#include "Input/TotwInputComponent.h"


void ATotwPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateAoeAbilityLocation();
}

/****************
* GET ASC
****************/
UAuraAbilitySystemComponent* ATotwPlayerController::GetASC()
{
	if (AuraAbilitySystemComponent == nullptr)
	{
		AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return AuraAbilitySystemComponent;
}

/**************
 * BEGIN PLAY
 *************/

void ATotwPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	GunFactionHud = Cast<AGunFactionHud>(GetHUD());
	ServerCheckMatchState();
	
/******************************************
 * ADD ENHANCED INPUT CONTEXT TO SUBSYSTEM
 ******************************************/
	
	check(TotwContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(TotwContext, 0);
	}
	// ***
	
	bShowMouseCursor = false;
	//DefaultMouseCursor = EMouseCursor::Default;
	//FInputModeGameAndUI InputModeData;
	//InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
	//InputModeData.SetHideCursorDuringCapture(true);
	//SetInputMode(InputModeData);
}

/***********************
* SETUP INPUT COMPONENT
************************/
void ATotwPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UTotwInputComponent* TotwInputComponent = CastChecked<UTotwInputComponent>(InputComponent);
	TotwInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::Move);
	TotwInputComponent->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::LookUp);
	TotwInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
	
	TotwInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this, &ATotwPlayerController::ShiftPressed);
	TotwInputComponent->BindAction(ShiftAction, ETriggerEvent::Completed, this, &ATotwPlayerController::ShiftReleased);

	// *** GunFaction Specific ***
	TotwInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::Shoot);
	TotwInputComponent->BindAction(ADSAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::ADS);
	TotwInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::Reload);
	TotwInputComponent->BindAction(MeleeAction, ETriggerEvent::Triggered, this, &ATotwPlayerController::Melee);


	
}

/*****************************
 * SERVER REQUEST SERVER TIME
 *****************************/
void ATotwPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}


/********************
* INPUT TAG PRESSED
**********************/
void ATotwPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	bAutoRunning = false;
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}
	if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		bTargeting = ThisActor ? true : false;
		bAutoRunning = false;
	}
	if (GetASC()) GetASC()->AbilityInputTagPressed(InputTag);
}

/*********************
* INPUT TAG RELEASED
**********************/
void ATotwPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputReleased))
	{
		return;
	}
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
		return;
	}

	if (GetASC()) GetASC()->AbilityInputTagReleased(InputTag);
	
	if (!bTargeting)
	{
		const APawn* ControlledPawn = GetPawn();
		if (FollowTime <= ShortPressThreshold && ControlledPawn)
		{
			if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination))
			{
				Spline->ClearSplinePoints();
				for (const FVector& PointLoc : NavPath->PathPoints)
				{
					Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
				}
				if (NavPath->PathPoints.Num() > 0)
				{
					CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
					bAutoRunning = true;
				}
			}
			if (GetASC() && !GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickNiagaraSystem, CachedDestination);
			}
		}
		FollowTime = 0.f;
		bTargeting = false;
	}
}

/****************
* INPUT TAG HELD
****************/
void ATotwPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputHeld))
	{
		return;
	}
	if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
	{
		if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
		return;
	}
}
/** REMOVED TO AVOID AUTORUN **
	if (bTargeting || bShiftKeyDown)
	{
		if (GetASC()) GetASC()->AbilityInputTagHeld(InputTag);
	}
	else
	{
		FollowTime += GetWorld()->GetDeltaSeconds();
		if (CursorHit.bBlockingHit) CachedDestination = CursorHit.ImpactPoint;

		if (APawn* ControlledPawn = GetPawn())
		{
			const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
			ControlledPawn->AddMovementInput(WorldDirection);
		}
	}
}
** AUTORUN ON CURSORHIT END/

/****************
* MOVE
****************/
void ATotwPlayerController::Move(const FInputActionValue& InputActionValue)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();
	
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

/****************
* LOOKUP 
****************/
void ATotwPlayerController::LookUp(const FInputActionValue& InputActionValue)
{
	if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
	{
		return;
	}
	const FVector2D LookVector = InputActionValue.Get<FVector2D>();
	
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{

		//Look Left / Right
		if (LookVector.Y != 0.f)
		{
			ControlledPawn->AddControllerYawInput(LookVector.Y);
		}
		
		//Look Up / Down
		if (LookVector.X != 0.f)
		{
			ControlledPawn->AddControllerPitchInput(LookVector.X);
		}
	}
}

void ATotwPlayerController::Shoot(const FInputActionValue& InputActionValue)
{
}

void ATotwPlayerController::ADS(const FInputActionValue& InputActionValue)
{
}

void ATotwPlayerController::Reload(const FInputActionValue& InputActionValue)
{
}

void ATotwPlayerController::Melee(const FInputActionValue& InputActionValue)
{
}


/***************
 * CURSOR TRACE
 ***************/
void ATotwPlayerController::CursorTrace()
{
	
}

/**************
 * TICK
 *************/
void ATotwPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

/*******************************
 * GET LIFETIME REPLICATED PROPS
 *******************************/
void ATotwPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATotwPlayerController, MatchState);
	
}

/*********************
 * ON MATCH STATE SET
 *********************/
void ATotwPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	/**if (MatchState == MatchState::WaitingToStart)
	{
		
	}**/
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

/***************************
 * HANDLE MATCH HAS STARTED
 ***************************/
void ATotwPlayerController::HandleMatchHasStarted()
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	if (GunFactionHud)
	{
		GunFactionHud->AddCharacterOverlay();
		if (GunFactionHud->Announce)
		{
			GunFactionHud->Announce->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

/*******************************
 * HANDLE COOLDOWN
 *******************************/
void ATotwPlayerController::HandleCooldown()
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	if (GunFactionHud)
	{GunFactionHud->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = GunFactionHud->Announce &&
							GunFactionHud->Announce->AnnouncementText &&
								GunFactionHud->Announce->InfoText;
		
		if (bHUDValid)
		{
			GunFactionHud->Announce->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In"); //message to be changed to this
			GunFactionHud->Announce->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ATotwGameState* TotwGameState = Cast<ATotwGameState>(UGameplayStatics::GetGameState(this));
			ATotwPlayerState* TotwPlayerState = GetPlayerState<ATotwPlayerState>();
			if (TotwGameState && TotwPlayerState)
			{
				TArray<ATotwPlayerState*> TopPlayers = TotwGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no Winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == TotwPlayerState)
				{
					InfoTextString = FString("You Win!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("We have a TIE: \n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}

				
				GunFactionHud->Announce->InfoText->SetText(FText::FromString(InfoTextString));
			}
			
			
		}
	}
	ATotwGunFaction* TotwGunFaction = Cast<ATotwGunFaction>(GetPawn());
	if (TotwGunFaction && TotwGunFaction->GetCombat())
	{
		TotwGunFaction->bDisableGameplay = true;
		TotwGunFaction->GetCombat()->FireButtonPressed(false);
	}
}


/*******************************
 * ONREP MATCH STATE SET
 *******************************/
void ATotwPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ATotwPlayerController::UpdateAoeAbilityLocation()
{
}


/**************************
 * CHECK TIME SYNC
 **************************/
void ATotwPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

/***********************
 * CLIENT JOIN MID GAME
 ***********************/
void ATotwPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (GunFactionHud && MatchState == MatchState::WaitingToStart)
	{
		GunFactionHud->AddAnnouncement();
	}
}

/****************************
 * SERVER CHECK MATCH STATE
 ****************************/
void ATotwPlayerController::ServerCheckMatchState_Implementation()
{
	ATotwGameModeBase* GameMode = Cast<ATotwGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

/*******************
 * GET SERVER TIME
 *******************/
float ATotwPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ATotwPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}




/*****************
 * SET HUD HEALTH
 *****************/
void ATotwPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;

	bool bHUDValid = GunFactionHud &&
						GunFactionHud->CharacterOverlay &&
							GunFactionHud->CharacterOverlay->HealthBar &&
								GunFactionHud->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		GunFactionHud->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		GunFactionHud->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}


/******************
 * SET HUD SCORE
 *****************/
void ATotwPlayerController::SetHUDScore(float Score)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
						GunFactionHud->CharacterOverlay &&
							GunFactionHud->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		
		GunFactionHud->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDScore = Score;
	}
}

/******************
 * SET HUD DEFEATS
 *******************/
void ATotwPlayerController::SetHUDDefeats(int32 Defeats)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
		GunFactionHud->CharacterOverlay &&
		GunFactionHud->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		GunFactionHud->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

/***********************
 * SET HUD WEAPON AMMO
 **********************/
void ATotwPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
		GunFactionHud->CharacterOverlay &&
		GunFactionHud->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		GunFactionHud->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}


/***********************
 * SET HUD CARRIED AMMO
 **********************/
void ATotwPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
		GunFactionHud->CharacterOverlay &&
		GunFactionHud->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		GunFactionHud->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

/**************************
 * SET HUD MATCH COUNTDOWN
 **************************/
void ATotwPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
						GunFactionHud->CharacterOverlay &&
							GunFactionHud->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			GunFactionHud->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		
		GunFactionHud->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

/*****************************
 * SET HUD ANNOUNCE COUNTDOWN
 *****************************/
void ATotwPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	GunFactionHud = GunFactionHud == nullptr ? Cast<AGunFactionHud>(GetHUD()) : GunFactionHud;
	bool bHUDValid = GunFactionHud &&
						GunFactionHud->Announce &&
							GunFactionHud->Announce->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			GunFactionHud->Announce->WarmupTime->SetText(FText()); // deliberately empty text
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		
		GunFactionHud->Announce->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

/**************************
 * SET HUD TIME
 **************************/
void ATotwPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;

	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	/***if (HasAuthority())
	{
		ShooterGameMode = ShooterGameMode == nullptr ? Cast<ATotwGameMode>(UGameplayStatics::GetGameMode(this)) : ShooterGameMode;
		if (ShooterGameMode)
		{
			SecondsLeft =FMath::CeilToInt(ShooterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}**/
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

/**************************
 * POLL INIT
 **************************/
void ATotwPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (GunFactionHud && GunFactionHud->CharacterOverlay)
		{
			CharacterOverlay = GunFactionHud->CharacterOverlay;
			if (CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDefeats(HUDDefeats);
			}
		}
	}
}





/****************************
 * CLIENT REPORT SERVER TIME
 ****************************/
void ATotwPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}


/*************
 * ON POSSESS
 *************/
void ATotwPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ATotwGunFaction* TotwGunFaction = Cast<ATotwGunFaction>(InPawn);
	if (TotwGunFaction)
	{
		SetHUDHealth(TotwGunFaction->GetHealth(), TotwGunFaction->GetMaxHealth());
	}
}

