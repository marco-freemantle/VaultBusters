// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Character/VBCharacter.h"
#include "GameFramework/GameMode.h"
#include "VBGameMode.generated.h"

namespace MatchState
{
	extern VAULTBUSTERS_API const FName Cooldown; //  Match duration has been reached. Display winner and being cooldown timer.
}

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite)
	FString KillsText;

	UPROPERTY(BlueprintReadWrite)
	FString ScoreText;

	UPROPERTY(BlueprintReadWrite)
	FString DeathsText;
};

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AVBGameMode();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerEliminated(AVBCharacter* ElimmedCharacter, AVBPlayerController* VictimController, AVBPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

	bool bTeamsMatch = false;

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<AVBPlayerController*> ConnectedControllers;

	UPROPERTY(BlueprintReadWrite)
	TArray<FPlayerInfo> PlayerInfoArray;

private:
	float CountdownTime = 0.f;
	void UpdateScoreboards();

public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
