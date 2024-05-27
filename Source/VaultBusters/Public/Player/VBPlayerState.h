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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddToScore(float ScoreAmount);
	void AddToKills(int32 KillsAmount);
	void AddToDeaths(int32 DeathsAmount);

	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Kills();

	UFUNCTION()
	virtual void OnRep_Deaths();

private:
	UPROPERTY()
	AVBCharacter* Character;

	UPROPERTY()
	AVBPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Kills)
	int32 Kills;

	UPROPERTY(ReplicatedUsing = OnRep_Deaths)
	int32 Deaths;

public:
	FORCEINLINE int32 GetKills() const { return Kills; }
	FORCEINLINE int32 GetDeaths() const { return Deaths; }
};

