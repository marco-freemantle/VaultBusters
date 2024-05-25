// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "VBGameUserSettings.generated.h"

/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API UVBGameUserSettings : public UGameUserSettings
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetMasterVolume(float NewValue);

	UFUNCTION(BlueprintCallable)
	void SetMouseSentivity(float NewValue);

	UFUNCTION(BlueprintCallable)
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable)
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintPure)
	static UVBGameUserSettings* GetVBGameUserSettings();

	UPROPERTY(Config, BlueprintReadWrite)
	float MasterVolume;

	UPROPERTY(Config, BlueprintReadWrite)
	float MouseSensitivity;
};
