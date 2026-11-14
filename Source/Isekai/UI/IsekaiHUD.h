#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IsekaiHUD.generated.h"

class AIsekaiCharacter;
class AIsekaiTimeOfDay;
class SWidget;

/**
 * 프로토타입용 화면 표시. 조준점, 시계, 마나, 상호작용 안내, 대화 상자를 보여 준다.
 * Slate로 직접 그리며 엔진 기본 글꼴을 쓴다. 정식 UI는 나중에 UMG로 교체한다.
 */
UCLASS()
class ISEKAI_API AIsekaiHUD : public AHUD
{
	GENERATED_BODY()

public:
	FText GetClockText();
	FText GetManaText() const;
	FText GetHealthText() const;
	FText GetPromptText() const;
	FText GetDialogueSpeaker() const;
	FText GetDialogueText() const;
	bool IsDialogueActive() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const AIsekaiCharacter* GetCharacter() const;

	TSharedPtr<SWidget> RootWidget;
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;
};
