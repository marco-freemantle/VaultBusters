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
}

void AVBPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void AVBPlayerState::AddToKills(int32 KillsAmount)
{
	Kills += KillsAmount;
	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->SetHUDKills(Kills);
		}
	}
}

void AVBPlayerState::AddToDeaths(int32 DeathsAmount)
{
	Deaths += DeathsAmount;
	Character = Character == nullptr ? Cast<AVBCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<AVBPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->SetHUDDeaths(Deaths);
		}
	}
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
			Controller->SetHUDScore(GetScore());
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
			Controller->SetHUDKills(Kills);
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
			Controller->SetHUDDeaths(Deaths);
		}
	}
}
