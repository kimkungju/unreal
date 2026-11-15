#include "Dialogue/IsekaiDialogueComponent.h"

#include "Audio/IsekaiAudio.h"

#include "Dialogue/IsekaiObjectiveComponent.h"
#include "GameFramework/Actor.h"

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
	IsekaiAudio::Play(this, TEXT("SFX_Tick"));
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
	IsekaiAudio::Play(this, TEXT("SFX_Tick"));
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

	if (!PendingObjective.IsEmpty())
	{
		UIsekaiObjectiveComponent* Objectives = GetOwner() ? GetOwner()->FindComponentByClass<UIsekaiObjectiveComponent>() : nullptr;
		if (Objectives)
		{
			Objectives->SetObjective(PendingObjective);
		}
		PendingObjective = FText::GetEmpty();
	}

	if (bWasActive)
	{
		OnDialogueEnded.Broadcast();
	}
}

FIsekaiDialogueLine UIsekaiDialogueComponent::GetCurrentLine() const
{
	return IsDialogueActive() ? Lines[CurrentIndex] : FIsekaiDialogueLine();
}
