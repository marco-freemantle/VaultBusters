// Copyright Marco Freemantle


#include "Game/VBGameUserSettings.h"

UVBGameUserSettings::UVBGameUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MasterVolume = 1.f;
	MusicVolume = 1.f;
	SoundEffectsVolume = 1.f;
	MouseSensitivity = 1.f;
}

void UVBGameUserSettings::SetMasterVolume(float NewValue)
{
	MasterVolume = NewValue;
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
