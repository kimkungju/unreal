#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IsekaiInteractable.h"
#include "IsekaiBrazier.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * 불을 옮겨 붙이는 화로. 빛 마법 수련에 쓴다.
 * 플레이어가 빛(F)을 켠 채로 상호작용하면 불이 붙는다. 꺼져 있으면 알려 준다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiBrazier : public AActor, public IIsekaiInteractable
{
	GENERATED_BODY()

public:
	AIsekaiBrazier();

	virtual FText GetInteractionPrompt_Implementation() const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Brazier")
	bool IsLit() const { return bLit; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bowl;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Flame;

private:
	bool bLit = false;
};
