// Copyright Marco Freemantle


#include "Game/TeamsGameMode.h"

#include "Game/VBGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VBPlayerController.h"
#include "Player/VBPlayerState.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if(AVBGameState* VBGameState = Cast<AVBGameState>(UGameplayStatics::GetGameState(this)))
	{
		AVBPlayerState* VBPlayerState = NewPlayer->GetPlayerState<AVBPlayerState>();
		if(VBPlayerState && VBPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			if(VBGameState->DefendingTeam.Num() >= VBGameState->AttackingTeam.Num())
			{
				VBGameState->AttackingTeam.AddUnique(VBPlayerState);
				VBPlayerState->SetTeam(ETeam::ET_AttackingTeam);
			}
			else
			{
				VBGameState->DefendingTeam.AddUnique(VBPlayerState);
				VBPlayerState->SetTeam(ETeam::ET_DefendingTeam);
			}
		}
	}

	UpdateScoreboards();
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AVBGameState* VBGameState = Cast<AVBGameState>(UGameplayStatics::GetGameState(this));
	AVBPlayerState* VBPlayerState = Exiting->GetPlayerState<AVBPlayerState>();
	if(VBGameState && VBPlayerState)
	{
		if(VBGameState->AttackingTeam.Contains(VBPlayerState))
		{
			VBGameState->AttackingTeam.Remove(VBPlayerState);
		}
		if(VBGameState->DefendingTeam.Contains(VBPlayerState))
		{
			VBGameState->DefendingTeam.Remove(VBPlayerState);
		}
	}
}

void ATeamsGameMode::PlayerEliminated(AVBCharacter* ElimmedCharacter, AVBPlayerController* VictimController,
	AVBPlayerController* AttackerController)
{
	Super::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	AVBPlayerState* AttackerPlayerState = AttackerController ? Cast<AVBPlayerState>(AttackerController->PlayerState) : nullptr;
	AVBGameState* VBGameState = Cast<AVBGameState>(UGameplayStatics::GetGameState(this));
	if(AttackerPlayerState && VBGameState)
	{
		if(AttackerPlayerState->GetTeam() == ETeam::ET_DefendingTeam)
		{
			VBGameState->DefendingTeamScores();
		}
		if(AttackerPlayerState->GetTeam() == ETeam::ET_AttackingTeam)
		{
			VBGameState->AttackingTeamScores();
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	AVBPlayerState* AttackerPState = Attacker->GetPlayerState<AVBPlayerState>();
	AVBPlayerState* VictimPState = Victim->GetPlayerState<AVBPlayerState>();
	if (AttackerPState == nullptr || VictimPState == nullptr) return BaseDamage;
	if (VictimPState == AttackerPState)
	{
		return BaseDamage;
	}
	if (AttackerPState->GetTeam() == VictimPState->GetTeam())
	{
		return 0.f;
	}
	return BaseDamage;
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if(AVBGameState* VBGameState = Cast<AVBGameState>(UGameplayStatics::GetGameState(this)))
	{
		for(auto PState : VBGameState->PlayerArray)
		{
			AVBPlayerState* VBPlayerState = Cast<AVBPlayerState>(PState.Get());
			if(VBPlayerState && VBPlayerState->GetTeam() == ETeam::ET_NoTeam)
			{
				if(VBGameState->DefendingTeam.Num() >= VBGameState->AttackingTeam.Num())
				{
					VBGameState->AttackingTeam.AddUnique(VBPlayerState);
					VBPlayerState->SetTeam(ETeam::ET_AttackingTeam);
				}
				else
				{
					VBGameState->DefendingTeam.AddUnique(VBPlayerState);
					VBPlayerState->SetTeam(ETeam::ET_DefendingTeam);
				}
			}
		}
	}
}

void ATeamsGameMode::FindMatchWinner()
{
	if (AVBGameState* VBGameState = GetGameState<AVBGameState>())
	{
		auto AnnounceToTeam = [](const TArray<AVBPlayerState*>& Team, const FString& Message)
		{
			for (AVBPlayerState* PlayerState : Team)
			{
				if (!PlayerState) continue;
				if (AVBPlayerController* PlayerController = Cast<AVBPlayerController>(PlayerState->GetPlayerController()))
				{
					PlayerController->ClientSetHUDAnnouncementText(Message);
				}
			}
		};

		if (VBGameState->AttackingTeamScore > VBGameState->DefendingTeamScore)
		{
			AnnounceToTeam(VBGameState->AttackingTeam, "YOU WIN");
			AnnounceToTeam(VBGameState->DefendingTeam, "YOU LOSE");
		}
		else if (VBGameState->AttackingTeamScore < VBGameState->DefendingTeamScore)
		{
			AnnounceToTeam(VBGameState->DefendingTeam, "YOU WIN");
			AnnounceToTeam(VBGameState->AttackingTeam, "YOU LOSE");
		}
		else
		{
			TArray<AVBPlayerState*> BothTeams = VBGameState->DefendingTeam;
			BothTeams.Append(VBGameState->AttackingTeam);
			AnnounceToTeam(BothTeams, "IT'S A DRAW");
		}
	}
}

