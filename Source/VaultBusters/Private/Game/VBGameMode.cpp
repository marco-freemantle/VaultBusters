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

		FString VictimName = VictimController->PlayerState ? VictimController->PlayerState->GetPlayerName() : FString();
		AttackerController->ClientSetHUDEliminated(VictimName);
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
