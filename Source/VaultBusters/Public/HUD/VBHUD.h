// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "VBHUD.generated.h"

class UCharacterOverlay;

USTRUCT(BlueprintType)
struct  FHUDPackage
{
	GENERATED_BODY()
public:
	UTexture2D* CrosshairsCentre;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
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

	UCharacterOverlay* CharacterOverlay;

protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();

private:
	FHUDPackage HUDPackage;
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
};
