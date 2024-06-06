// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VBHUD.generated.h"

class UAnnouncement;
class UCharacterOverlay;
class UScoreboard;
class UUserWidget;

USTRUCT(BlueprintType)
struct  FHUDPackage
{
	GENERATED_BODY()
public:

	UPROPERTY()
	UTexture2D* CrosshairsCentre;
	UPROPERTY()
	UTexture2D* CrosshairsLeft;
	UPROPERTY()
	UTexture2D* CrosshairsRight;
	UPROPERTY()
	UTexture2D* CrosshairsTop;
	UPROPERTY()
	UTexture2D* CrosshairsBottom;
	float CrosshairSpread;
	FLinearColor CrosshairsColour;
};

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	UPROPERTY(EditAnywhere, Category="Player Stats")
	TSubclassOf<UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Player stats")
	TSubclassOf<UUserWidget> CharacterScoreboardClass;

	UPROPERTY(EditAnywhere, Category = "Player stats")
	TSubclassOf<UUserWidget> CharacterTeamScoreboardClass;

	UPROPERTY()
	UScoreboard* Scoreboard;

	UPROPERTY(EditAnywhere, Category="Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY()
	UAnnouncement* Announcement;

	void AddCharacterOverlay();
	void AddScoreboard();
	void AddTeamScoreboard();
	void AddAnnouncement(bool bShouldDisplayImmediately);

protected:
	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
};
