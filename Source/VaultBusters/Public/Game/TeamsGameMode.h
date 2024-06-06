// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Game/VBGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API ATeamsGameMode : public AVBGameMode
{
	GENERATED_BODY()

public:
	ATeamsGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void PlayerEliminated(AVBCharacter* ElimmedCharacter, AVBPlayerController* VictimController, AVBPlayerController* AttackerController) override;
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
protected:
	virtual void HandleMatchHasStarted() override;
private:
	virtual void FindMatchWinner() override;
};
