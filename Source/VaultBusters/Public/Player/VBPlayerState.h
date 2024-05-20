// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VBPlayerState.generated.h"

class AVBPlayerController;
class AVBCharacter;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void OnRep_Score() override;
	void AddToScore(float ScoreAmount);

private:
	UPROPERTY()
	AVBCharacter* Character;

	UPROPERTY()
	AVBPlayerController* Controller;
};
