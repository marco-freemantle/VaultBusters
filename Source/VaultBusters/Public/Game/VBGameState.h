// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VBGameState.generated.h"

class AVBPlayerState;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AttackingTeamScores();
	void DefendingTeamScores();
	
	TArray<AVBPlayerState*> AttackingTeam;
	TArray<AVBPlayerState*> DefendingTeam;

	UPROPERTY(ReplicatedUsing=OnRep_AttackingTeamScore)
	float AttackingTeamScore = 0.f;
	
	UPROPERTY(ReplicatedUsing=OnRep_DefendingTeamScore)
	float DefendingTeamScore = 0.f;

	UFUNCTION()
	void OnRep_AttackingTeamScore();

	UFUNCTION()
	void OnRep_DefendingTeamScore();
};
