// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Character/VBCharacter.h"
#include "GameFramework/GameMode.h"
#include "VBGameMode.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(AVBCharacter* ElimmedCharacter, AVBPlayerController* VictimController, AVBPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
};
