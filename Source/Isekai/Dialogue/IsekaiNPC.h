#pragma once

#include "CoreMinimal.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "GameFramework/Character.h"
#include "Interaction/IsekaiInteractable.h"
#include "IsekaiNPC.generated.h"

class UStaticMeshComponent;

/**
 * 말을 걸 수 있는 인물.
 * 캐릭터 모델이 준비되기 전까지 원기둥(Placeholder)으로 보인다.
 * 모델이 생기면 Mesh에 스켈레탈 메시를 넣고 Placeholder를 숨긴다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiNPC : public ACharacter, public IIsekaiInteractable
{
	GENERATED_BODY()

public:
	AIsekaiNPC();

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	/** 대사에서 Speaker를 비워 두면 이 이름이 쓰인다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TArray<FIsekaiDialogueLine> Lines;

	/** 비어 있지 않으면, 대화가 끝났을 때 목표가 이 내용으로 바뀐다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText ObjectiveAfterTalk;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Placeholder;
};
