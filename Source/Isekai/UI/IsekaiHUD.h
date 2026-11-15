#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Styling/SlateColor.h"
#include "IsekaiHUD.generated.h"

class AIsekaiCharacter;
class AIsekaiEnemy;
class AIsekaiTimeOfDay;
class SWidget;

/**
 * 화면 표시 전부를 맡는다.
 *
 * 모양은 명조(Wuthering Waves)를 참고했다: 가는 선, 반투명한 어두운 바탕, 마름모 장식,
 * 금색과 하늘색 강조, 자간을 넓힌 작은 머리글.
 * 배치는 콘솔 게임 방식이다: 화면 가장자리에 안전 여백을 두고, 정보는 네 귀퉁이와 아래 가운데에 모은다.
 *
 *   왼쪽 위    목표
 *   오른쪽 위  시계
 *   위 가운데  큰 적의 체력
 *   아래 가운데 체력, 마나, 걸으며 나누는 말
 *   오른쪽 아래 기술 (마름모 칸, 쿨다운과 마나 부족 표시)
 *   가운데 오른쪽 상호작용 안내 (키 상자)
 *   대화 중에는 평소 표시를 숨기고, 아래쪽을 어둡게 깔아 대사를 띄운다. 선택지는 오른쪽에 세로로.
 *
 * Slate로 직접 그리며 엔진 기본 글꼴을 쓴다. 그림 파일은 쓰지 않는다.
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

	/** 조작 힌트. 빈 문구를 주면 사라진다. */
	void SetHint(const FText& InText);

	/** 큰 제목과 부제. 빈 문구를 주면 서서히 사라진다. */
	void SetTitle(const FText& InTitle, const FText& InSubtitle);

	/** 화면 아래에 잠깐 뜨는 한마디. 조작을 막지 않는다. 걸으면서 나누는 말에 쓴다. */
	void ShowBark(const FText& InSpeaker, const FText& InText, float Duration);

	/** 위아래 검은 띠 */
	void SetLetterbox(bool bInEnabled) { bLetterbox = bInEnabled; }

	/** 체력, 기술 등 평소 표시를 통째로 켜고 끈다. */
	void SetGameplayVisible(bool bInVisible) { bGameplayVisible = bInVisible; }

	// ---- 위젯이 읽어 가는 것 ----

	bool IsGameplayShown() const;
	bool IsDialogueActive() const;

	FText GetClockText();
	FText GetObjectiveText() const;
	FText GetPromptText() const;

	float GetHealthFraction() const;
	float GetHealthLagFraction() const { return HealthLag; }
	FText GetHealthText() const;
	float GetManaFraction() const;

	float GetAttackCooldown() const;
	float GetDashCooldown() const;
	bool CanAffordAttack() const;
	bool CanAffordDash() const;
	bool CanAffordLight() const;
	bool IsLightOn() const;

	bool IsBossShown() const { return Boss.IsValid(); }
	float GetBossFraction() const;
	float GetBossLagFraction() const { return BossLag; }

	FText GetDialogueSpeaker() const;
	FText GetDialogueText() const;
	bool IsChoosing() const;
	int32 GetChoiceCount() const;
	FText GetChoiceText(int32 Index) const;

	FText GetBarkSpeaker() const { return BarkSpeaker; }
	FText GetBarkText() const { return Bark; }
	float GetBarkAlpha() const;

	/** 맞았을 때와 체력이 낮을 때 화면 가장자리를 붉게 물들이는 세기. 0~1. */
	float GetVignetteStrength() const;

	FSlateColor GetFadeColor() const;
	float GetNarrationAlpha() const { return NarrationAlpha; }
	float GetTitleAlpha() const { return TitleAlpha; }
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
	TWeakObjectPtr<AIsekaiEnemy> Boss;

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

	FText BarkSpeaker;
	FText Bark;
	float BarkTimeLeft = 0.f;

	bool bLetterbox = false;
	float LetterboxAmount = 0.f;

	bool bGameplayVisible = true;

	/** 맞은 만큼 잠시 남아 있다가 줄어드는 막대. 얼마나 깎였는지 눈에 보이게 한다. */
	float HealthLag = 1.f;
	float HealthLagHold = 0.f;
	float LastHealth = 1.f;

	float BossLag = 1.f;
	float BossLagHold = 0.f;
	float LastBoss = 1.f;
};
