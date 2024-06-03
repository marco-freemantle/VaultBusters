// Copyright Marco Freemantle

#include "Game/VBGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/VBPlayerController.h"

void AVBGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVBGameState, AttackingTeamScore);
	DOREPLIFETIME(AVBGameState, DefendingTeamScore);
}

void AVBGameState::AttackingTeamScores()
{
	++AttackingTeamScore;

	if(AVBPlayerController* VBPlayer = Cast<AVBPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		VBPlayer->SetHUDAttackingTeamScore(AttackingTeamScore);
	}
}

void AVBGameState::DefendingTeamScores()
{
	++DefendingTeamScore;

	if(AVBPlayerController* VBPlayer = Cast<AVBPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		VBPlayer->SetHUDDefendingTeamScore(DefendingTeamScore);
	}
}

void AVBGameState::OnRep_AttackingTeamScore()
{
	if(AVBPlayerController* VBPlayer = Cast<AVBPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		VBPlayer->SetHUDAttackingTeamScore(AttackingTeamScore);
	}
}

void AVBGameState::OnRep_DefendingTeamScore()
{
	if(AVBPlayerController* VBPlayer = Cast<AVBPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		VBPlayer->SetHUDDefendingTeamScore(DefendingTeamScore);
	}
}

