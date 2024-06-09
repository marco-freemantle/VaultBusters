// Copyright Marco Freemantle


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(const FString& TextToDisplay) const
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

