// Copyright Marco Freemantle

#include "Player/VBPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/WidgetTree.h"
#include "Character/VBCharacter.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/VBGameMode.h"
#include "Game/VBGameUserSettings.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "HUD/Announcement.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Scoreboard.h"
#include "Input/VBInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "VBComponents/CombatComponent.h"
#include "Weapon/Weapon.h"

AVBPlayerController::AVBPlayerController()
{
	bReplicates = true;
	CachedPlayerInfoArray = TArray<FPlayerInfo>();
}

void AVBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	InterpCameraCrouch(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
}

void AVBPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AVBPlayerController::PollInit()
{
	if(CharacterOverlay == nullptr)
	{
		if(VBHUD && VBHUD->CharacterOverlay)
		{
			CharacterOverlay = VBHUD->CharacterOverlay;
			if(CharacterOverlay)
			{
				SetHUDHealth(HUDHealth, HUDMaxHealth);
				SetHUDScore(HUDScore);
				SetHUDDeaths(HUDDeaths);
				SetHUDKills(HUDKills);
			}
		}
	}
	if(Scoreboard == nullptr)
	{
		if(VBHUD && VBHUD->Scoreboard)
		{
			Scoreboard = VBHUD->Scoreboard;
			if(Scoreboard)
			{
				ClientUpdateScoreboard(CachedPlayerInfoArray);
			}
		}
	}
}

void AVBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	check(VBContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(VBContext, 0);
	}

	bShowMouseCursor = false;

	FInputModeGameOnly InputModeData;
	SetInputMode(InputModeData);
	
	VBHUD = Cast<AVBHUD>(GetHUD());
	ServerCheckMatchState();
}

void AVBPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AVBCharacter* VBCharacter = Cast<AVBCharacter>(InPawn);
	if(VBCharacter)
	{
		SetHUDHealth(VBCharacter->GetHealth(), VBCharacter->GetMaxHealth());
	}
}

void AVBPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVBPlayerController, MatchState);
}

void AVBPlayerController::InterpCameraCrouch(float DeltaTime)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(!VBCharacter->GetCameraBoom()) return;
		if(VBCharacter->bIsCrouched)
		{
			CurrentZLocation = FMath::FInterpTo(CurrentZLocation, CrouchedZLocation, DeltaTime, 7.f);
			VBCharacter->GetCameraBoom()->SetRelativeLocation(FVector(0.f, 0.f, CurrentZLocation));
		}
		else
		{
			CurrentZLocation = FMath::FInterpTo(CurrentZLocation, BaseZLocation, DeltaTime, 7.f);
			VBCharacter->GetCameraBoom()->SetRelativeLocation(FVector(0.f, 0.f, CurrentZLocation));
		}
	}
}

void AVBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UVBInputComponent* VBInputComponent = CastChecked<UVBInputComponent>(InputComponent);

	VBInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Move);
	VBInputComponent->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AVBPlayerController::LookUp);
	VBInputComponent->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Turn);
	VBInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Jump);
	VBInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &AVBPlayerController::Equip);
	VBInputComponent->BindAction(EquipAction, ETriggerEvent::Completed, this, &AVBPlayerController::SetbCanEquipTrue);
	VBInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &AVBPlayerController::DropWeapon);
	VBInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Completed, this, &AVBPlayerController::SetbCanDropWeaponTrue);
	VBInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AVBPlayerController::Crouch);
	VBInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AVBPlayerController::UnCrouch);
	VBInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AVBPlayerController::Aim);
	VBInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AVBPlayerController::StopAim);
	VBInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AVBPlayerController::Fire);
	VBInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AVBPlayerController::StopFire);
	VBInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AVBPlayerController::Reload);
	VBInputComponent->BindAction(ShowScoreboardAction, ETriggerEvent::Started, this, &AVBPlayerController::ToggleScoreboard);
	VBInputComponent->BindAction(ShowScoreboardAction, ETriggerEvent::Completed, this, &AVBPlayerController::ToggleScoreboard);
}

void AVBPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void AVBPlayerController::LookUp(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		if(UVBGameUserSettings* UserSettings = Cast<UVBGameUserSettings>(UGameUserSettings::GetGameUserSettings()))
		{
			ControlledPawn->AddControllerPitchInput(InputAxisVector.X * UserSettings->GetMouseSensitivity());
		}
	}
}

void AVBPlayerController::Turn(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		if(UVBGameUserSettings* UserSettings = Cast<UVBGameUserSettings>(UGameUserSettings::GetGameUserSettings()))
		{
			ControlledPawn->AddControllerYawInput(InputAxisVector.X * UserSettings->GetMouseSensitivity());
		}
	}
}

void AVBPlayerController::Jump(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->Jump();
	}
}

void AVBPlayerController::Equip(const FInputActionValue& InputActionValue)
{
	if(!bCanEquip) return;
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->EquipWeapon();
	}
	bCanEquip = false;
}

void AVBPlayerController::DropWeapon(const FInputActionValue& InputActionValue)
{
	if(!bCanDropWeapon) return;
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetEquippedWeapon())
		{
			VBCharacter->DropWeapon();
		}
	}
	bCanDropWeapon = false;
}

void AVBPlayerController::ToggleScoreboard()
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;

	if (VBHUD && VBHUD->Scoreboard && bCanToggleScoreboard)
	{
		bIsScoreboardOpen = !bIsScoreboardOpen;

		if (bIsScoreboardOpen)
		{
			VBHUD->Scoreboard->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			VBHUD->Scoreboard->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AVBPlayerController::Crouch(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetCharacterMovement()->IsFalling()) return;
		VBCharacter->Crouch();
	}
}

void AVBPlayerController::UnCrouch(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->UnCrouch();
	}
}

void AVBPlayerController::Aim(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetCombatComponent() && VBCharacter->GetCombatComponent()->EquippedWeapon)
		{
			VBCharacter->GetCombatComponent()->SetAiming(true);
		}
	}
}

void AVBPlayerController::StopAim(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		if(VBCharacter->GetCombatComponent() && VBCharacter->GetCombatComponent()->EquippedWeapon)
		{
			VBCharacter->GetCombatComponent()->SetAiming(false);
		}
	}
}

void AVBPlayerController::Fire(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->FireButtonPressed(true);
	}
}

void AVBPlayerController::StopFire(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}

void AVBPlayerController::Reload(const FInputActionValue& InputActionValue)
{
	if (AVBCharacter* VBCharacter = Cast<AVBCharacter>(GetCharacter()))
	{
		VBCharacter->GetCombatComponent()->Reload();
	}
}

void AVBPlayerController::SetbCanEquipTrue()
{
	bCanEquip = true;
}

void AVBPlayerController::SetbCanDropWeaponTrue()
{
	bCanDropWeapon = true;
}

void AVBPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = Health / MaxHealth;
		FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		VBHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void AVBPlayerController::SetHUDScore(float Score)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		VBHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDScore = Score;
	}
}

void AVBPlayerController::SetHUDKills(int32 Kills)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->KillsAmount)
	{
		FString KillsText = FString::Printf(TEXT("%d"), Kills);
		VBHUD->CharacterOverlay->KillsAmount->SetText(FText::FromString(KillsText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDKills = Kills;
	}
}

void AVBPlayerController::SetHUDDeaths(int32 Deaths)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->DeathsAmount)
	{
		FString DeathsText = FString::Printf(TEXT("%d"), Deaths);
		VBHUD->CharacterOverlay->DeathsAmount->SetText(FText::FromString(DeathsText));
	}
	else
	{
		bInitialiseCharacterOverlay = true;
		HUDDeaths = Deaths;
	}
}

void AVBPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->AmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		VBHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void AVBPlayerController::SetHUDWeaponTotalAmmo(int32 TotalAmmo)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->TotalAmmoAmount)
	{
		FString TotalAmmoText = FString::Printf(TEXT("%d"), TotalAmmo);
		VBHUD->CharacterOverlay->TotalAmmoAmount->SetText(FText::FromString(TotalAmmoText));
	}
}

void AVBPlayerController::SetHUDMatchCountDown(float CountDownTime)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->MatchCountDownText)
	{
		if(CountDownTime < 0.f)
		{
			VBHUD->CharacterOverlay->MatchCountDownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;

		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		VBHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountDownText));
	}
}

void AVBPlayerController::SetHUDAnnouncementCountdown(float CountDownTime)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if (VBHUD && VBHUD->Announcement && VBHUD->Announcement->WarmupTime)
	{
		if(CountDownTime < 0.f)
		{
			VBHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60.f);
		int32 Seconds = CountDownTime - Minutes * 60;

		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		VBHUD->Announcement->WarmupTime->SetText(FText::FromString(CountDownText));
	}
}

void AVBPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if(MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		VBGameMode = VBGameMode == nullptr ? Cast<AVBGameMode>(UGameplayStatics::GetGameMode(this)) : VBGameMode;
		if(VBGameMode)
		{
			SecondsLeft = FMath::CeilToInt(VBGameMode->GetCountdownTime() + LevelStartingTime);
			LevelStartingTime = VBGameMode->LevelStartingTime;
		}
	}
	
	if (CountDownInt != SecondsLeft)
	{
		if(MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if(MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountDown(TimeLeft);
		}
	}

	CountDownInt = SecondsLeft;
}

void AVBPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AVBPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AVBPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return  GetWorld()->GetTimeSeconds();
	}
	return  GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AVBPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AVBPlayerController::ClientSetHUDImpactCrosshair_Implementation()
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ImpactCrosshair && IsLocalController())
	{
		GetWorldTimerManager().ClearTimer(ImpactCrosshairTimerHandle);
		VBHUD->CharacterOverlay->ImpactCrosshair->SetVisibility(ESlateVisibility::Visible);

		GetWorldTimerManager().SetTimer(ImpactCrosshairTimerHandle, [this]() {
			//Set visibility to hidden after 0.75 seconds
			if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->ImpactCrosshair)
			{
				VBHUD->CharacterOverlay->ImpactCrosshair->SetVisibility(ESlateVisibility::Hidden);
			}
			}, 0.75f, false);
	}
}

void AVBPlayerController::ClientSetHUDEliminated_Implementation(const FString& VictimName)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;

	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->Eliminated)
	{
		GetWorldTimerManager().ClearTimer(EliminatedTimerHandle);

		UHorizontalBox* Box = Cast<UHorizontalBox>(VBHUD->CharacterOverlay->Eliminated->GetChildAt(0));
		if (Box)
		{
			UTextBlock* VictimNameText = Cast<UTextBlock>(Box->GetChildAt(2));
			if (VictimNameText)
			{
				VictimNameText->SetText(FText::FromString(VictimName));
			}
		}

		VBHUD->CharacterOverlay->Eliminated->SetVisibility(ESlateVisibility::Visible);

		GetWorldTimerManager().SetTimer(EliminatedTimerHandle, [this]() {
			//Set visibility to hidden after 3 second
			if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->Eliminated)
			{
				VBHUD->CharacterOverlay->Eliminated->SetVisibility(ESlateVisibility::Hidden);
			}
			}, 3.f, false);
	}
}

void AVBPlayerController::ClientSetHUDKillFeeds_Implementation(const FString& VictimName, const FString& AttackerName)
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	
	if (VBHUD && VBHUD->CharacterOverlay && VBHUD->CharacterOverlay->Killfeeds)
	{
		if (KillFeedItemClass)
		{
			//Create an instance of the Widget Blueprint
			UUserWidget* KillFeedItemWidget = CreateWidget<UUserWidget>(this, KillFeedItemClass);
			if (UWidgetTree* WidgetTree = KillFeedItemWidget->WidgetTree)
			{
				UTextBlock* AttackerTextBlock = WidgetTree->FindWidget<UTextBlock>(TEXT("AttackerText"));
				UTextBlock* VictimTextBlock = WidgetTree->FindWidget<UTextBlock>(TEXT("VictimText"));
				if (AttackerTextBlock && VictimTextBlock)
				{
					AttackerTextBlock->SetText(FText::FromString(AttackerName));
					VictimTextBlock->SetText(FText::FromString(VictimName));
					VBHUD->CharacterOverlay->Killfeeds->AddChildToVerticalBox(KillFeedItemWidget);

					FTimerHandle DestroyKillFeedItemTimerHandle;
					GetWorldTimerManager().SetTimer(DestroyKillFeedItemTimerHandle, [KillFeedItemWidget]() {
						//Destroy widget after 5 seconds
						KillFeedItemWidget->RemoveFromParent();
						}, 4.f, false);
				}
			}
		}
	}
}

void AVBPlayerController::ClientPlayHitGiven_Implementation()
{
	if(HitGivenSound)
	{
		UGameplayStatics::PlaySound2D(this, HitGivenSound);
	}
}

void AVBPlayerController::ClientPlayHeadshotGiven_Implementation()
{
	if(HeadshotGivenSound)
	{
		UGameplayStatics::PlaySound2D(this, HeadshotGivenSound);
	}
}

void AVBPlayerController::ClientUpdateScoreboard_Implementation(const TArray<FPlayerInfo>& PlayerInfoArray)
{
	CachedPlayerInfoArray = PlayerInfoArray;
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	
	if (VBHUD && VBHUD->Scoreboard)
	{
		// Clear existing items from the scoreboard
		VBHUD->Scoreboard->ScoreList->ClearChildren();
	
		for (const FPlayerInfo& PlayerInfo : PlayerInfoArray)
		{
			FString DisplayName = PlayerInfo.DisplayName;
			if (ScoreboardItemClass)
			{
				UUserWidget* ScoreboardItemWidget = CreateWidget<UUserWidget>(this, ScoreboardItemClass);
				if (UWidgetTree* WidgetTree = ScoreboardItemWidget->WidgetTree)
				{
					UTextBlock* PlayerNameText = WidgetTree->FindWidget<UTextBlock>(TEXT("PlayerNameText"));
					UTextBlock* PlayerScoreText = WidgetTree->FindWidget<UTextBlock>(TEXT("PlayerScoreText"));
					UTextBlock* PlayerElimsText = WidgetTree->FindWidget<UTextBlock>(TEXT("PlayerElimsText"));
					UTextBlock* PlayerDeathsText = WidgetTree->FindWidget<UTextBlock>(TEXT("PlayerDeathsText"));
					
	
					if (PlayerNameText && PlayerScoreText && PlayerElimsText && PlayerDeathsText)
					{
						PlayerNameText->SetText(FText::FromString(DisplayName));
						PlayerScoreText->SetText(FText::FromString(PlayerInfo.ScoreText));
						PlayerElimsText->SetText(FText::FromString(PlayerInfo.KillsText));
						PlayerDeathsText->SetText(FText::FromString(PlayerInfo.DeathsText));
	
						VBHUD->Scoreboard->ScoreList->AddChild(ScoreboardItemWidget);
					}
				}
			}
		}
	}
}

void AVBPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	if(MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AVBPlayerController::OnRep_MatchState()
{
	if(MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void AVBPlayerController::HandleMatchHasStarted()
{
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	if(VBHUD)
	{
		if(VBHUD->CharacterOverlay == nullptr) VBHUD->AddCharacterOverlay();
		if(VBHUD->Scoreboard == nullptr) VBHUD->AddScoreboard();
		if(VBHUD->Announcement)
		{
			VBHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AVBPlayerController::HandleCooldown()
{
	bCanToggleScoreboard = false;
	VBHUD = VBHUD == nullptr ? Cast<AVBHUD>(GetHUD()) : VBHUD;
	
	if (VBHUD && VBHUD->Scoreboard)
	{
		VBHUD->Scoreboard->SetVisibility(ESlateVisibility::Visible);
	}
	if(VBHUD)
	{
		VBHUD->CharacterOverlay->RemoveFromParent();
		if(VBHUD->Announcement && VBHUD->Announcement->AnnouncementText)
		{
			VBHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts n:");
			VBHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));
		}
	}
	DisableInput(this);
}

void AVBPlayerController::ServerCheckMatchState_Implementation()
{
	AVBGameMode* GameMode = Cast<AVBGameMode>(UGameplayStatics::GetGameMode(this));
	if(GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		if(VBHUD && MatchState == MatchState::WaitingToStart)
		{
			VBHUD->AddAnnouncement();
		}
	}
}

void AVBPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if(VBHUD && MatchState == MatchState::WaitingToStart)
	{
		VBHUD->AddAnnouncement();
	}
}
