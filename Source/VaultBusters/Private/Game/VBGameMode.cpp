// Copyright Marco Freemantle

#include "Game/VBGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Player/VBPlayerController.h"
#include "Player/VBPlayerState.h"

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
