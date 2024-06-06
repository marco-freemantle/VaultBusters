// Copyright Marco Freemantle


#include "HUD/VBHUD.h"
#include "Blueprint/UserWidget.h"
#include "HUD/Announcement.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Scoreboard.h"
#include "Player/VBPlayerController.h"

void AVBHUD::DrawHUD()
{
	Super::DrawHUD();

	if(GEngine)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCentre(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

		if(HUDPackage.CrosshairsCentre)
		{
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsCentre, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
		}
		if(HUDPackage.CrosshairsLeft)
		{
			FVector2D Spread(-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
		}
		if(HUDPackage.CrosshairsRight)
		{
			FVector2D Spread(SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
		}
		if(HUDPackage.CrosshairsTop)
		{
			FVector2D Spread(0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
		}
		if(HUDPackage.CrosshairsBottom)
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
		}
	}
}

void AVBHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AVBHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if(PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void AVBHUD::AddScoreboard()
{
	AVBPlayerController* PlayerController = Cast<AVBPlayerController>(GetOwningPlayerController());
	if (PlayerController && CharacterScoreboardClass)
	{
		if(Scoreboard) Scoreboard->RemoveFromParent();
		Scoreboard = CreateWidget<UScoreboard>(PlayerController, CharacterScoreboardClass);
		Scoreboard->AddToViewport();
		Scoreboard->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AVBHUD::AddTeamScoreboard()
{
	AVBPlayerController* PlayerController = Cast<AVBPlayerController>(GetOwningPlayerController());
	if (PlayerController && CharacterTeamScoreboardClass)
	{
		if(Scoreboard) Scoreboard->RemoveFromParent();
		Scoreboard = CreateWidget<UScoreboard>(PlayerController, CharacterTeamScoreboardClass);
		Scoreboard->AddToViewport();
		Scoreboard->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AVBHUD::AddAnnouncement(bool bShouldDisplayImmediately)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if(PlayerController && AnnouncementClass && Announcement == nullptr)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
		if(!bShouldDisplayImmediately)
		{
			Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AVBHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2D TextureDrawPoint(
		ViewportCentre.X - (TextureWidth / 2.f) + Spread.X,
		ViewportCentre.Y - (TextureHeight / 2.f) + Spread.Y
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
		CrosshairColour
		);
}
