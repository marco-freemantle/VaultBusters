// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "HUD/Scoreboard.h"
#include "TeamScoreboard.generated.h"

class UVerticalBox;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API UTeamScoreboard : public UScoreboard
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* AttackingTeamScoreList;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* DefendingTeamScoreList;
};
