#pragma once

#include "CoreMinimal.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/IsekaiInteractable.h"
#include "IsekaiExaminable.generated.h"

class UStaticMeshComponent;

/**
 * 살펴볼 수 있는 사물. 제단, 표지판, 비석처럼 말을 하지 않는 대상에 쓴다.
 * 살펴보면 대화 상자에 설명이 나온다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiExaminable : public AActor, public IIsekaiInteractable
{
	GENERATED_BODY()

public:
	AIsekaiExaminable();

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Examinable")
	FText PromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Examinable")
	TArray<FIsekaiDialogueLine> Lines;

	/** 비어 있지 않으면, 살펴본 뒤 목표가 이 내용으로 바뀐다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Examinable")
	FText ObjectiveAfter;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
