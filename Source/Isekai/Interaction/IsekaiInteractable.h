#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IsekaiInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UIsekaiInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 플레이어가 바라보고 E 키로 상호작용할 수 있는 모든 대상이 구현하는 인터페이스.
 * C++ 클래스와 블루프린트 양쪽에서 구현할 수 있다.
 */
class ISEKAI_API IIsekaiInteractable
{
	GENERATED_BODY()

public:
	/** 화면에 표시할 안내 문구. 예: "문 열기" */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;

	/** 지금 상호작용이 가능한지. false면 안내 문구가 뜨지 않는다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
};
