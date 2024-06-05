// Copyright Marco Freemantle

#include "Player/VBPlayerState.h"
#include "Character/VBCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Player/VBPlayerController.h"

void AVBPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVBPlayerState, Kills);
	DOREPLIFETIME(AVBPlayerState, Deaths);
	DOREPLIFETIME(AVBPlayerState, Team);
}

void AVBPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
}

void AVBPlayerState::AddToKills(int32 KillsAmount)
{
	Kills += KillsAmount;
}

void AVBPlayerState::AddToDeaths(int32 DeathsAmount)
{
	Deaths += DeathsAmount;
}

void AVBPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			
		}
	}
}

void AVBPlayerState::OnRep_Kills()
{
	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			
		}
	}
}

void AVBPlayerState::OnRep_Deaths()
{
	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			
		}
	}
}

void AVBPlayerState::SetTeam(ETeam TeamToSet)
{
	Team = TeamToSet;

	if(AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetPawn()))
	{
		VBCharacter->SetTeamMesh(Team);
	}
}

void AVBPlayerState::OnRep_Team()
{
	if(AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetPawn()))
	{
		VBCharacter->SetTeamMesh(Team);
	}
}
