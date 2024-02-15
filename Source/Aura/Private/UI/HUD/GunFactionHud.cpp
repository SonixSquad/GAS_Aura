// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/GunFactionHud.h"

#include "UI/HUD/GunCharacterOverlay.h"
#include "GameFramework/PlayerController.h"
#include "Components/HorizontalBox.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Announce.h"

/*******************
 * DRAW CROSSHAIR
 *******************/
void AGunFactionHud::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	{
		const float TextureWidth = Texture->GetSizeX();
		const float TextureHeight = Texture->GetSizeY();
		const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
		);

		DrawTexture(
			Texture,
			TextureDrawPoint.X,
			TextureDrawPoint.Y,
			TextureWidth,
			TextureHeight,
			0.f,
			0.f,
			1.f,
			1.f,
			CrosshairColor
			);
	}
}

/*******************
 * DRAW HUD
 *******************/
void AGunFactionHud::DrawHUD()
	{
		Super::DrawHUD();

		FVector2D ViewportSize; 
		if (GEngine)
		{

			GEngine->GameViewport->GetViewportSize(ViewportSize);
			const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

			float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

			if (HUDPackage.CrosshairsCenter)
			{
				FVector2D Spread(0.f, 0.f);
				DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
			}
			if (HUDPackage.CrosshairsLeft)
			{
				FVector2D Spread(-SpreadScaled, 0.f);
				DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
			}
			if (HUDPackage.CrosshairsRight)
			{
				FVector2D Spread(SpreadScaled, 0.f);
				DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
			}
			if (HUDPackage.CrosshairsTop)
			{
				FVector2D Spread(0.f, -SpreadScaled);
				DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
			}
			if (HUDPackage.CrosshairsBottom)
			{
				FVector2D Spread(0.f, SpreadScaled);
				DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
			}
		}
	}



/*******************
 * BEGIN PLAY
 *******************/
void AGunFactionHud::BeginPlay()
{
	Super::BeginPlay();
	
}

/*******************
 * ADD CHARACTER OVERLAY
 *******************/
void AGunFactionHud::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UGunCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

/*******************
 * ADD ANNOUNCEMENT
 *******************/
void AGunFactionHud::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announce = CreateWidget<UAnnounce>(PlayerController, AnnouncementClass);
		Announce->AddToViewport();
	}
}



