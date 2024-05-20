// Copyright Marco Freemantle


#include "Player/VBPlayerState.h"

#include "Character/VBCharacter.h"
#include "Player/VBPlayerController.h"

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
