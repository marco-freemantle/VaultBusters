// Copyright Marco Freemantle

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class VAULTBUSTERS_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarmupTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;
};
