// Copyright Marco Freemantle

#include "Game/VBGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VBPlayerController.h"
#include "Player/VBPlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AVBGameMode::AVBGameMode()
{
	bDelayedStart = true;
}

void AVBGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AVBPlayerController* NewController = Cast<AVBPlayerController>(NewPlayer))
	{
		ConnectedControllers.Add(NewController);
		UpdateScoreboards();
	}
}

void AVBGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (AVBPlayerController* ExitingController = Cast<AVBPlayerController>(Exiting))
	{
		if (ConnectedControllers.Contains(ExitingController))
		{
			ConnectedControllers.Remove(ExitingController);
		}
		UpdateScoreboards();
	}
}

void AVBGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AVBGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if(CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds();
		if(CountdownTime <= 0.f)
		{
			// for (AVBPlayerController* PlayerController : ConnectedControllers)
			// {
			// 	PlayerController->ClientSetHUDFinishGame();
			// }
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if(CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AVBGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if(AVBPlayerController* VBPlayer = Cast<AVBPlayerController>(*It))
		{
			VBPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void AVBGameMode::PlayerEliminated(AVBCharacter* ElimmedCharacter, AVBPlayerController* VictimController,
                                   AVBPlayerController* AttackerController)
{
	AVBPlayerState* AttackerPlayerState = AttackerController ? Cast<AVBPlayerState>(AttackerController->PlayerState) : nullptr;
	AVBPlayerState* VictimPlayerState = VictimController ? Cast<AVBPlayerState>(VictimController->PlayerState) : nullptr;

	if(AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(100.f);
		AttackerPlayerState->AddToKills(1);

		FString VictimSteamName = VictimController->PlayerState ? VictimController->PlayerState->GetPlayerName() : FString();
		FString AttackerSteamName = AttackerController->PlayerState ? AttackerController->PlayerState->GetPlayerName() : FString();
		AttackerController->ClientSetHUDEliminated(VictimSteamName);

		//Get all player controllers and update KillFeed
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PlayerController = It->Get())
			{
				if (AVBPlayerController* IndexedController = Cast<AVBPlayerController>(PlayerController))
				{
					IndexedController->ClientSetHUDKillFeeds(VictimSteamName, AttackerSteamName);
				}
			}
		}
	}
	if(VictimPlayerState)
	{
		VictimPlayerState->AddToDeaths(1);
	}
	
	if(ElimmedCharacter)
	{
		ElimmedCharacter->Elim();
	}
	UpdateScoreboards();
}

void AVBGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if(ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if(ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void AVBGameMode::UpdateScoreboards()
{
	PlayerInfoArray.Empty();
	for (AVBPlayerController* PlayerController : ConnectedControllers)
	{
		FPlayerInfo NewPlayerInfo;
		NewPlayerInfo.DisplayName = PlayerController->PlayerState ? PlayerController->PlayerState->GetPlayerName() : "Unknown";
		NewPlayerInfo.ScoreText = PlayerController->PlayerState ? FString::Printf(TEXT("%d"), FMath::RoundToInt(Cast<AVBPlayerState>(PlayerController->PlayerState)->GetScore())) : FString(TEXT("N/A"));
		NewPlayerInfo.KillsText = PlayerController->PlayerState ? FString::Printf(TEXT("%d"), Cast<AVBPlayerState>(PlayerController->PlayerState)->GetKills()) : FString(TEXT("N/A"));
		NewPlayerInfo.DeathsText = PlayerController->PlayerState ? FString::Printf(TEXT("%d"), Cast<AVBPlayerState>(PlayerController->PlayerState)->GetDeaths()) : FString(TEXT("N/A"));
		PlayerInfoArray.Add(NewPlayerInfo);
	}

	FTimerHandle UpateScoreboardTimerHandle;

	// Update scoreboard after 1 second
	GetWorldTimerManager().SetTimer(UpateScoreboardTimerHandle, [this]() {
		for (AVBPlayerController* PlayerController : ConnectedControllers)
		{
			PlayerController->ClientUpdateScoreboard(PlayerInfoArray);
		}
		}, 1.f, false);
}
