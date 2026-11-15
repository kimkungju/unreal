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
	// 선택을 기다리는 대사는 E로 넘어가지 않는다.
	if (!IsDialogueActive() || IsWaitingForChoice())
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

bool UIsekaiDialogueComponent::IsWaitingForChoice() const
{
	return IsDialogueActive() && Lines[CurrentIndex].Choices.Num() > 0;
}

void UIsekaiDialogueComponent::Choose(int32 ChoiceIndex)
{
	if (!IsWaitingForChoice() || !Lines[CurrentIndex].Choices.IsValidIndex(ChoiceIndex))
	{
		return;
	}

	LastChoice = ChoiceIndex;

	++CurrentIndex;
	IsekaiAudio::Play(this, TEXT("SFX_Tick"));
	if (!Lines.IsValidIndex(CurrentIndex))
	{
		EndDialogue();
	}
}
