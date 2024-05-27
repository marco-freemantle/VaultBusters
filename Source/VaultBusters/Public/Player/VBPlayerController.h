// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VBPlayerController.generated.h"

class UScoreboard;
class AVBGameMode;
class UCharacterOverlay;
class AVBCharacter;
class AVBHUD;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UUserWidget;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API AVBPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AVBPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDKills(int32 Kills);
	void SetHUDDeaths(int32 Deaths);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDWeaponTotalAmmo(int32 TotalAmmo);
	void SetHUDMatchCountDown(float CountDownTime);
	void SetHUDAnnouncementCountdown(float CountDownTime);

	virtual float GetServerTime(); //Synced with server world clock
	virtual void ReceivedPlayer() override; //Sync with server clock as soon as possible

	void SetHUDTime();

	/*
	Sync time between client and server
	*/
	//Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//Reports the current server time to the client in response to the ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	//Difference between client and server time
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	FTimerHandle ImpactCrosshairTimerHandle;
	UFUNCTION(Client, Reliable)
	void ClientSetHUDImpactCrosshair();

	FTimerHandle EliminatedTimerHandle;
	UFUNCTION(Client, Reliable)
	void ClientSetHUDEliminated(const FString& VictimName);
	
	UFUNCTION(Client, Unreliable)
	void ClientPlayHitGiven();

	UFUNCTION(Client, Unreliable)
	void ClientPlayHeadshotGiven();

	UFUNCTION(Client, Reliable)
	void ClientSetHUDKillFeeds(const FString& VictimName, const FString& AttackerName);

	UFUNCTION(Client, Reliable)
	void ClientUpdateScoreboard(const TArray<FPlayerInfo>& PlayerInfoArray);

	void OnMatchStateSet(FName State);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void PollInit();
	void HandleMatchHasStarted();
	void HandleCooldown();

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> VBContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookUpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> TurnAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> DropWeaponAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ShowScoreboardAction;

	void Move(const FInputActionValue& InputActionValue);
	void LookUp(const FInputActionValue& InputActionValue);
	void Turn(const FInputActionValue& InputActionValue);
	void Jump(const FInputActionValue& InputActionValue);
	void Equip(const FInputActionValue& InputActionValue);
	void Crouch(const FInputActionValue& InputActionValue);
	void UnCrouch(const FInputActionValue& InputActionValue);
	void Aim(const FInputActionValue& InputActionValue);
	void StopAim(const FInputActionValue& InputActionValue);
	void Fire(const FInputActionValue& InputActionValue);
	void StopFire(const FInputActionValue& InputActionValue);
	void Reload(const FInputActionValue& InputActionValue);
	void DropWeapon(const FInputActionValue& InputActionValue);
	void ToggleScoreboard();

	bool bCanEquip = true;
	bool bCanDropWeapon = true;
	
	bool bIsScoreboardOpen = false;
	bool bCanToggleScoreboard = true;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> ScoreboardItemClass;

	void SetbCanEquipTrue();
	void SetbCanDropWeaponTrue();

	void InterpCameraCrouch(float DeltaTime);
	float BaseZLocation = 145.f;
	float CrouchedZLocation = 100.f;
	float CurrentZLocation = 145.f;

	UPROPERTY()
	AVBHUD* VBHUD;

	UPROPERTY()
	AVBGameMode* VBGameMode;

	UPROPERTY()
	AVBCharacter* VBOwnerCharacter;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountDownInt = 0;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> KillFeedItemClass;

	UPROPERTY(EditAnywhere)
	USoundBase* HitGivenSound;

	UPROPERTY(EditAnywhere)
	USoundBase* HeadshotGivenSound;

	UPROPERTY(ReplicatedUsing=OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;
	bool bInitialiseCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDKills;
	int32 HUDDeaths;
	
	UPROPERTY()
	UScoreboard* Scoreboard;
	TArray<FPlayerInfo> CachedPlayerInfoArray;
	bool bInitialiseScoreboard = false;
};
