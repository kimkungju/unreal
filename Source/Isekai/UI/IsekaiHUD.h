#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Styling/SlateColor.h"
#include "IsekaiHUD.generated.h"

class AIsekaiCharacter;
class AIsekaiTimeOfDay;
class SWidget;

/**
 * 화면 표시 전부를 맡는다.
 *
 * 평소 표시: 조준점, 시계, 체력, 마나, 목표, 상호작용 안내, 대화 상자.
 * 연출 표시: 화면 전체 페이드, 위아래 검은 띠, 가운데 자막, 조작 힌트, 제목.
 *
 * Slate로 직접 그리며 엔진 기본 글꼴을 쓴다. 정식 UI는 나중에 UMG로 교체한다.
 */
UCLASS()
class ISEKAI_API AIsekaiHUD : public AHUD
{
	GENERATED_BODY()

public:
	AIsekaiHUD();

	virtual void Tick(float DeltaSeconds) override;

	// ---- 연출에서 부르는 것 ----

	/** 화면을 한 색으로 덮는다. Duration이 0이면 즉시. TargetAlpha 0은 걷어내기. */
	void FadeTo(const FLinearColor& InColor, float TargetAlpha, float Duration);

	/** 화면 가운데 자막. 빈 문구를 주면 서서히 사라진다. */
	void SetNarration(const FText& InText);

	/** 화면 아래쪽 조작 힌트. 빈 문구를 주면 사라진다. */
	void SetHint(const FText& InText);

	/** 큰 제목과 부제. 빈 문구를 주면 서서히 사라진다. */
	void SetTitle(const FText& InTitle, const FText& InSubtitle);

	/** 위아래 검은 띠 */
	void SetLetterbox(bool bInEnabled) { bLetterbox = bInEnabled; }

	/** 조준점, 체력 등 평소 표시를 통째로 켜고 끈다. */
	void SetGameplayVisible(bool bInVisible) { bGameplayVisible = bInVisible; }

	// ---- 위젯이 읽어 가는 것 ----

	FText GetClockText();
	FText GetManaText() const;
	FText GetHealthText() const;
	FText GetObjectiveText() const;
	FText GetPromptText() const;
	FText GetDialogueSpeaker() const;
	FText GetDialogueText() const;
	bool IsDialogueActive() const;
	bool IsGameplayVisible() const { return bGameplayVisible; }

	/** 맞은 직후 잠깐 화면을 붉게 덮는 색. 평소에는 투명. */
	FSlateColor GetDamageFlashColor() const;

	FSlateColor GetFadeColor() const;
	FSlateColor GetNarrationColor() const;
	FSlateColor GetTitleColor() const;
	float GetLetterboxHeight() const { return LetterboxAmount * 110.f; }
	FText GetNarrationText() const { return Narration; }
	FText GetHintText() const { return Hint; }
	FText GetTitleText() const { return TitleText; }
	FText GetSubtitleText() const { return SubtitleText; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const AIsekaiCharacter* GetCharacter() const;

	TSharedPtr<SWidget> RootWidget;
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;

	FLinearColor FadeColor = FLinearColor::Black;
	float FadeAlpha = 0.f;
	float FadeTarget = 0.f;
	float FadeSpeed = 1.f;

	FText Narration;
	float NarrationAlpha = 0.f;
	float NarrationTarget = 0.f;

	FText Hint;

	FText TitleText;
	FText SubtitleText;
	float TitleAlpha = 0.f;
	float TitleTarget = 0.f;

	bool bLetterbox = false;
	float LetterboxAmount = 0.f;

	bool bGameplayVisible = true;
};
