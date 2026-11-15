#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiDialogueComponent.generated.h"

/** 대사 한 줄. 누가 말하는지와 내용. */
USTRUCT(BlueprintType)
struct FIsekaiDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
	FText Text;

	/** 비어 있지 않으면 이 대사에서 플레이어가 숫자 키로 하나를 골라야 넘어간다. 최대 3개. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FText> Choices;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIsekaiDialogueEvent);

/**
 * 플레이어에 붙어 진행 중인 대화를 관리한다.
 * 지금은 대사를 순서대로 넘기는 것만 한다. 선택지와 분기는 나중에 추가한다.
 */
UCLASS(ClassGroup = (Isekai), meta = (BlueprintSpawnableComponent))
class ISEKAI_API UIsekaiDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsekaiDialogueComponent();

	/** 대화를 시작한다. 이미 대화 중이거나 대사가 없으면 false. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(const TArray<FIsekaiDialogueLine>& InLines);

	/** 다음 대사로 넘긴다. 마지막 대사였으면 대화를 끝낸다. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Advance();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();

	/** 지금 진행 중인 대화가 끝나면 목표를 이 내용으로 바꾼다. StartDialogue 직후에 부른다. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetObjectiveOnEnd(const FText& Objective) { PendingObjective = Objective; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return Lines.IsValidIndex(CurrentIndex); }

	/** 지금 대사가 선택을 기다리는 중인지 */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsWaitingForChoice() const;

	/** 선택지를 고른다. 0이 첫 번째. 선택을 기다리는 중이 아니거나 없는 번호면 무시된다. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Choose(int32 ChoiceIndex);

	/** 가장 최근에 고른 선택지 번호. 고른 적이 없으면 -1. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetLastChoice() const { return LastChoice; }

	/** 대화 중이 아니면 빈 대사를 돌려준다. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FIsekaiDialogueLine GetCurrentLine() const;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FIsekaiDialogueEvent OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FIsekaiDialogueEvent OnDialogueEnded;

private:
	UPROPERTY(Transient)
	TArray<FIsekaiDialogueLine> Lines;

	int32 CurrentIndex = INDEX_NONE;
	int32 LastChoice = INDEX_NONE;
	FText PendingObjective;
};
