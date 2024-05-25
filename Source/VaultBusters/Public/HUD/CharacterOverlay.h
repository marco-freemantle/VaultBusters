// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;
class UBorder;
class UVerticalBox;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillsAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DeathsAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TotalAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UImage* ImpactCrosshair;

	UPROPERTY(meta = (BindWidget))
	UBorder* Eliminated;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountDownText;
	
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* Killfeeds;
};
