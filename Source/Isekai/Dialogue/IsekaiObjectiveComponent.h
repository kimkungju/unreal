#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiObjectiveComponent.generated.h"

/**
 * 지금 해야 할 일 한 줄을 들고 있다. 화면 오른쪽 위에 표시된다.
 * 퀘스트 목록이나 진행 기록은 아직 없다.
 */
UCLASS(ClassGroup = (Isekai), meta = (BlueprintSpawnableComponent))
class ISEKAI_API UIsekaiObjectiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsekaiObjectiveComponent();

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void SetObjective(const FText& NewObjective) { CurrentObjective = NewObjective; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	FText GetObjective() const { return CurrentObjective; }

	/** 게임을 시작했을 때의 목표 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText InitialObjective;

protected:
	virtual void BeginPlay() override;

private:
	FText CurrentObjective;
};
