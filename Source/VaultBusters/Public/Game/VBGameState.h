// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VBGameState.generated.h"

class AVBPlayerState;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(AVBPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<AVBPlayerState*> TopScoringPlayers;
};
