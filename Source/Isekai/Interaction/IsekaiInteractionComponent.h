#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiInteractionComponent.generated.h"

/**
 * 소유자의 시선 방향으로 매 프레임 선을 쏴서 상호작용 대상을 찾는다.
 * 대상은 Visibility 채널을 막고 IIsekaiInteractable을 구현해야 한다.
 */
UCLASS(ClassGroup = (Isekai), meta = (BlueprintSpawnableComponent))
class ISEKAI_API UIsekaiInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsekaiInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 지금 바라보는 대상과 상호작용한다. 성공하면 true. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** 바라보는 대상이 없으면 빈 문구를 돌려준다. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetFocusedPrompt() const;

	/** 상호작용이 닿는 거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceDistance = 250.f;

private:
	TWeakObjectPtr<AActor> FocusedActor;
};
