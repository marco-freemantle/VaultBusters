// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Scoreboard.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API UScoreboard : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* ScoreList;
};
