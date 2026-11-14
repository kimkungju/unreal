#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IsekaiInteractable.h"
#include "IsekaiInteractableActor.generated.h"

class UStaticMeshComponent;

/**
 * 가장 단순한 상호작용 대상. 레벨에 놓고 메시와 문구만 지정하면 동작한다.
 * 블루프린트로 상속해서 OnInteracted 이벤트에 원하는 동작을 연결한다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiInteractableActor : public AActor, public IIsekaiInteractable
{
	GENERATED_BODY()

public:
	AIsekaiInteractableActor();

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bInteractionEnabled = true;

protected:
	/** 상호작용이 일어났을 때 블루프린트에서 받는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteracted(AActor* Interactor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
