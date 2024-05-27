// Copyright Marco Freemantle

#include "Game/VBGameState.h"
#include "Net/UnrealNetwork.h"

void AVBGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVBGameState, TopScoringPlayers);
}

void AVBGameState::UpdateTopScore(AVBPlayerState* ScoringPlayer)
{
	
}
