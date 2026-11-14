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

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return Lines.IsValidIndex(CurrentIndex); }

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
};
