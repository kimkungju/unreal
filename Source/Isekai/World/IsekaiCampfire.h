#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IsekaiInteractable.h"
#include "IsekaiCampfire.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 쉬어 갈 수 있는 모닥불.
 * 쉬면 체력과 마나가 회복되고, 시간이 흐르고, 쓰러졌을 때 돌아올 자리가 여기로 바뀐다.
 * 낮에 쉬면 저녁이 되고, 밤에 쉬면 아침이 된다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiCampfire : public AActor, public IIsekaiInteractable
{
	GENERATED_BODY()

public:
	AIsekaiCampfire();

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Fire;
};
