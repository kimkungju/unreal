#include "Dialogue/IsekaiDialogueComponent.h"

UIsekaiDialogueComponent::UIsekaiDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UIsekaiDialogueComponent::StartDialogue(const TArray<FIsekaiDialogueLine>& InLines)
{
	if (IsDialogueActive() || InLines.Num() == 0)
	{
		return false;
	}

	Lines = InLines;
	CurrentIndex = 0;
	OnDialogueStarted.Broadcast();
	return true;
}

void UIsekaiDialogueComponent::Advance()
{
	if (!IsDialogueActive())
	{
		return;
	}

	++CurrentIndex;
	if (!Lines.IsValidIndex(CurrentIndex))
	{
		EndDialogue();
	}
}

void UIsekaiDialogueComponent::EndDialogue()
{
	const bool bWasActive = Lines.Num() > 0;

	Lines.Reset();
	CurrentIndex = INDEX_NONE;

	if (bWasActive)
	{
		OnDialogueEnded.Broadcast();
	}
}

FIsekaiDialogueLine UIsekaiDialogueComponent::GetCurrentLine() const
{
	return IsDialogueActive() ? Lines[CurrentIndex] : FIsekaiDialogueLine();
}
