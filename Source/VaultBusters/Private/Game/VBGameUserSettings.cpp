// Copyright Marco Freemantle


#include "Game/VBGameUserSettings.h"

UVBGameUserSettings::UVBGameUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MasterVolume = 5.f;
	MouseSensitivity = 5.f;
}

void UVBGameUserSettings::SetMasterVolume(float NewValue)
{
	MasterVolume = NewValue;
}

void UVBGameUserSettings::SetMouseSentivity(float NewValue)
{
	MouseSensitivity = NewValue;
}

float UVBGameUserSettings::GetMasterVolume() const
{
	return MasterVolume;
}

float UVBGameUserSettings::GetMouseSensitivity() const
{
	return MouseSensitivity;
}

UVBGameUserSettings* UVBGameUserSettings::GetVBGameUserSettings()
{
	return Cast<UVBGameUserSettings>(GetGameUserSettings());
}
