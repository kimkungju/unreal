#include "UI/IsekaiHUD.h"

#include "Combat/IsekaiEnemy.h"
#include "Combat/IsekaiHealthComponent.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Dialogue/IsekaiObjectiveComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/IsekaiInteractionComponent.h"
#include "Misc/App.h"
#include "Player/IsekaiCharacter.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/ColText/STextBlock.h"
#include "World/IsekaiTimeOfDay.h"

// ---------------------------------------------------------------------------
// 모양 규칙. 색과 여백을 바꾸려면 여기만 고치면 된다.
// ---------------------------------------------------------------------------
namespace IsekaiUI
{
	// 글자와 선
	const FLinearColor ColText(0.93f, 0.94f, 0.96f, 1.f);
	const FLinearColor ColTextDim(0.93f, 0.94f, 0.96f, 0.45f);
	const FLinearColor ColLine(0.93f, 0.94f, 0.96f, 0.3f);

	// 바탕
	const FLinearColor ColPanel(0.03f, 0.04f, 0.06f, 0.62f);
	const FLinearColor ColBarBack(1.f, 1.f, 1.f, 0.12f);

	// 강조
	const FLinearColor ColGold(0.96f, 0.84f, 0.58f, 1.f);
	const FLinearColor ColHealth(0.84f, 0.96f, 0.9f, 1.f);
	const FLinearColor ColHealthLow(0.98f, 0.45f, 0.38f, 1.f);
	const FLinearColor ColLag(0.98f, 0.4f, 0.32f, 0.85f);
	const FLinearColor ColMana(0.45f, 0.82f, 0.98f, 1.f);
	const FLinearColor ColCooldown(0.f, 0.f, 0.f, 0.55f);

	// 콘솔 게임의 안전 여백. TV는 가장자리가 잘려 보일 수 있어서 정보를 안쪽에 둔다.
	const float SafeX = 80.f;
	const float SafeY = 56.f;

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Color.A * Alpha);
	}

	FSlateFontInfo Font(const char* Typeface, int32 Size, int32 LetterSpacing = 0)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(Typeface, Size);
		Info.LetterSpacing = LetterSpacing;
		return Info;
	}

	const FSlateBrush* White()
	{
		return FCoreStyle::Get().GetBrush("WhiteBrush");
	}

	/** 45도 돌린 변환. 정사각형을 마름모로 만든다. */
	TOptional<FSlateRenderTransform> Diamond()
	{
		return FSlateRenderTransform(FQuat2f(FMath::DegreesToRadians(45.f)));
	}

	/** 키 이름을 테두리 상자에 담는다. 콘솔 게임의 버튼 표시 자리. */
	TSharedRef<SWidget> KeyBox(const FText& Key, int32 Size = 13)
	{
		return SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(FSlateColor(ColText))
			.Padding(1.f)
			[
				SNew(SBorder)
				.BorderImage(White())
				.BorderBackgroundColor(FSlateColor(ColPanel))
				.Padding(FMargin(7.f, 1.f))
				[
					SNew(SBox)
					.MinDesiredWidth(12.f)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(Font("Bold", Size))
						.ColorAndOpacity(FSlateColor(ColText))
						.Text(Key)
					]
				]
			];
	}

	/** 작은 마름모 하나. 머리글 앞에 붙이는 장식. */
	TSharedRef<SWidget> Pip(const FLinearColor& Color, float Size = 7.f)
	{
		return SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			.RenderTransform(Diamond())
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			[
				SNew(SImage)
				.Image(White())
				.ColorAndOpacity(FSlateColor(Color))
			];
	}

	/** 가는 막대. Fill과 Lag은 0~1을 돌려주는 함수다. 뒤에 깔린 ColLag 막대가 깎인 양을 잠시 보여 준다. */
	TSharedRef<SWidget> Bar(float Width, float Height, TFunction<float()> Fill, TFunction<float()> LagFill,
		TFunction<FSlateColor()> FillColor)
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.HeightOverride(Height)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(White())
					.ColorAndOpacity(FSlateColor(ColBarBack))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride_Lambda([Width, LagFill]()
					{
						return FOptionalSize(Width * FMath::Clamp(LagFill ? LagFill() : 0.f, 0.f, 1.f));
					})
					[
						SNew(SImage)
						.Image(White())
						.ColorAndOpacity(FSlateColor(ColLag))
					]
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride_Lambda([Width, Fill]()
					{
						return FOptionalSize(Width * FMath::Clamp(Fill(), 0.f, 1.f));
					})
					[
						SNew(SImage)
						.Image(White())
						.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda(FillColor))
					]
				]
			];
	}
}

using namespace IsekaiUI;

AIsekaiHUD::AIsekaiHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

// ---------------------------------------------------------------------------
// 상태
// ---------------------------------------------------------------------------

void AIsekaiHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 느린 화면 중에도 화면 연출은 실제 시간으로 움직인다.
	const float RealDelta = FApp::GetDeltaTime();

	FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, FadeTarget, RealDelta, FadeSpeed);
	NarrationAlpha = FMath::FInterpConstantTo(NarrationAlpha, NarrationTarget, RealDelta, 2.5f);
	TitleAlpha = FMath::FInterpConstantTo(TitleAlpha, TitleTarget, RealDelta, 1.2f);
	LetterboxAmount = FMath::FInterpTo(LetterboxAmount, bLetterbox ? 1.f : 0.f, RealDelta, 4.f);
	BarkTimeLeft = FMath::Max(0.f, BarkTimeLeft - RealDelta);
	ToastTimeLeft = FMath::Max(0.f, ToastTimeLeft - RealDelta);

	// 그림 컷: 바뀔 그림이 기다리고 있으면 지금 그림이 다 사라진 뒤에 바꾼다.
	CutImageAlpha = FMath::FInterpConstantTo(CutImageAlpha, CutImageTarget, RealDelta, 2.2f);
	CutSubtitleAlpha = FMath::FInterpConstantTo(CutSubtitleAlpha, CutSubtitleTarget, RealDelta, 3.f);
	CutTime += RealDelta;
	if (bHasPendingShot && CutImageAlpha <= 0.01f)
	{
		ApplyPendingShot();
	}

	// 깎인 체력은 0.5초 머물렀다가 줄어든다.
	auto UpdateLag = [RealDelta](float Current, float& InLag, float& Hold, float& Last)
	{
		if (Current < Last)
		{
			Hold = 0.5f;
		}
		Last = Current;

		if (Current >= InLag)
		{
			InLag = Current;
		}
		else if (Hold > 0.f)
		{
			Hold -= RealDelta;
		}
		else
		{
			InLag = FMath::FInterpConstantTo(InLag, Current, RealDelta, 0.8f);
		}
	};

	UpdateLag(GetHealthFraction(), HealthLag, HealthLagHold, LastHealth);

	// 큰 적을 찾는다. 몇 마리 안 되므로 매 프레임 훑어도 괜찮다.
	if (!Boss.IsValid() || Boss->IsDefeated())
	{
		Boss = nullptr;
		for (TActorIterator<AIsekaiEnemy> It(GetWorld()); It; ++It)
		{
			if (It->IsElite() && !It->IsDefeated())
			{
				Boss = *It;
				BossLag = 1.f;
				LastBoss = 1.f;
				break;
			}
		}
	}
	if (Boss.IsValid())
	{
		UpdateLag(Boss->GetHealthFraction(), BossLag, BossLagHold, LastBoss);
	}
}

void AIsekaiHUD::FadeTo(const FLinearColor& InColor, float TargetAlpha, float Duration)
{
	FadeColor = InColor;
	FadeTarget = FMath::Clamp(TargetAlpha, 0.f, 1.f);

	if (Duration <= 0.f)
	{
		FadeAlpha = FadeTarget;
		FadeSpeed = 1.f;
	}
	else
	{
		FadeSpeed = FMath::Max(0.01f, FMath::Abs(FadeTarget - FadeAlpha) / Duration);
	}
}

void AIsekaiHUD::SetNarration(const FText& InText)
{
	if (InText.IsEmpty())
	{
		// 글자는 남겨 둔 채 투명해지게 한다.
		NarrationTarget = 0.f;
	}
	else
	{
		Narration = InText;
		NarrationTarget = 1.f;
	}
}

void AIsekaiHUD::SetHint(const FText& InText)
{
	Hint = InText;
}

void AIsekaiHUD::SetTitle(const FText& InTitle, const FText& InSubtitle)
{
	if (InTitle.IsEmpty())
	{
		TitleTarget = 0.f;
	}
	else
	{
		TitleText = InTitle;
		SubtitleText = InSubtitle;
		TitleTarget = 1.f;
	}
}

void AIsekaiHUD::BeginCutscene()
{
	bCutsceneActive = true;
	CutImageAlpha = 0.f;
	CutImageTarget = 0.f;
	CutSubtitle = FText::GetEmpty();
	CutSubtitleAlpha = 0.f;
	CutSubtitleTarget = 0.f;
}

void AIsekaiHUD::EndCutscene()
{
	bCutsceneActive = false;
	bHasPendingShot = false;
	CutTexture = nullptr;
	PendingTexture = nullptr;
	CutImageAlpha = 0.f;
	CutImageTarget = 0.f;
}

void AIsekaiHUD::ShowCutsceneShot(UTexture2D* Texture, const FText& Placeholder, const FVector2D& PanFrom, const FVector2D& PanTo,
	float ZoomFrom, float ZoomTo, float Duration)
{
	PendingTexture = Texture;
	PendingPlaceholder = Placeholder;
	PendingPanFrom = PanFrom;
	PendingPanTo = PanTo;
	PendingZoomFrom = ZoomFrom;
	PendingZoomTo = ZoomTo;
	PendingDuration = FMath::Max(0.1f, Duration);
	bHasPendingShot = true;

	// 지금 그림을 먼저 걷어 낸다. 다 걷히면 Tick이 새 그림으로 바꾼다.
	CutImageTarget = 0.f;
}

void AIsekaiHUD::ApplyPendingShot()
{
	bHasPendingShot = false;

	CutTexture = PendingTexture;
	PendingTexture = nullptr;
	CutPlaceholder = PendingPlaceholder;
	CutPanFrom = PendingPanFrom;
	CutPanTo = PendingPanTo;
	CutZoomFrom = PendingZoomFrom;
	CutZoomTo = PendingZoomTo;
	CutDuration = PendingDuration;
	CutTime = 0.f;

	CutBrush = FSlateBrush();
	if (CutTexture)
	{
		CutBrush.SetResourceObject(CutTexture);
		const float Width = CutTexture->GetSizeX() > 0 ? static_cast<float>(CutTexture->GetSizeX()) : 1920.f;
		const float Height = CutTexture->GetSizeY() > 0 ? static_cast<float>(CutTexture->GetSizeY()) : 1080.f;
		CutBrush.ImageSize = FVector2D(Width, Height);
		CutBrush.DrawAs = ESlateBrushDrawType::Image;
	}

	CutImageTarget = 1.f;
}

TOptional<FSlateRenderTransform> AIsekaiHUD::GetCutsceneTransform() const
{
	// 처음과 끝에서 부드럽게 멈추는 진행도
	const float Linear = FMath::Clamp(CutTime / CutDuration, 0.f, 1.f);
	const float Progress = FMath::SmoothStep(0.f, 1.f, Linear);

	const float Zoom = FMath::Lerp(CutZoomFrom, CutZoomTo, Progress);
	const FVector2D Pan = FMath::Lerp(CutPanFrom, CutPanTo, Progress);
	return FSlateRenderTransform(Zoom, FVector2f(static_cast<float>(Pan.X), static_cast<float>(Pan.Y)));
}

void AIsekaiHUD::SetCutsceneSubtitle(const FText& InText)
{
	if (InText.IsEmpty())
	{
		CutSubtitleTarget = 0.f;
	}
	else
	{
		CutSubtitle = InText;
		CutSubtitleTarget = 1.f;
		CutSubtitleAlpha = FMath::Min(CutSubtitleAlpha, 0.2f);
	}
}

void AIsekaiHUD::ShowToast(const FText& InHeader, const FText& InBody, float Duration)
{
	ToastHeader = InHeader;
	ToastBody = InBody;
	ToastDuration = FMath::Max(0.5f, Duration);
	ToastTimeLeft = ToastDuration;
}

float AIsekaiHUD::GetToastAlpha() const
{
	// 0.3초에 걸쳐 나타나고 마지막 0.5초에 걸쳐 사라진다.
	const float Elapsed = ToastDuration - ToastTimeLeft;
	return FMath::Clamp(FMath::Min(Elapsed / 0.3f, ToastTimeLeft / 0.5f), 0.f, 1.f);
}

void AIsekaiHUD::ShowBark(const FText& InSpeaker, const FText& InText, float Duration)
{
	BarkSpeaker = InSpeaker;
	Bark = InText;
	BarkTimeLeft = Duration;
}

float AIsekaiHUD::GetBarkAlpha() const
{
	// 마지막 0.4초 동안 사라진다. 대화 상자가 떠 있으면 숨긴다.
	return IsDialogueActive() ? 0.f : FMath::Clamp(BarkTimeLeft / 0.4f, 0.f, 1.f);
}

FSlateColor AIsekaiHUD::GetFadeColor() const
{
	return FSlateColor(FLinearColor(FadeColor.R, FadeColor.G, FadeColor.B, FadeAlpha));
}

const AIsekaiCharacter* AIsekaiHUD::GetCharacter() const
{
	return Cast<AIsekaiCharacter>(GetOwningPawn());
}

bool AIsekaiHUD::IsGameplayShown() const
{
	return bGameplayVisible && !IsDialogueActive();
}

bool AIsekaiHUD::IsDialogueActive() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->GetDialogue()->IsDialogueActive();
}

FText AIsekaiHUD::GetClockText()
{
	if (!TimeOfDay.IsValid())
	{
		for (TActorIterator<AIsekaiTimeOfDay> It(GetWorld()); It; ++It)
		{
			TimeOfDay = *It;
			break;
		}
	}

	return TimeOfDay.IsValid() ? FText::FromString(TimeOfDay->GetClockString()) : FText::GetEmpty();
}

FText AIsekaiHUD::GetObjectiveText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetObjectives()->GetObjective() : FText::GetEmpty();
}

FText AIsekaiHUD::GetPromptText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character || IsDialogueActive())
	{
		return FText::GetEmpty();
	}

	return Character->GetInteraction()->GetFocusedPrompt();
}

float AIsekaiHUD::GetHealthFraction() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return 1.f;
	}

	const UIsekaiHealthComponent* HealthComponent = Character->GetHealthComponent();
	return HealthComponent->MaxHealth > 0.f ? HealthComponent->GetHealth() / HealthComponent->MaxHealth : 0.f;
}

FText AIsekaiHUD::GetHealthText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FText::GetEmpty();
	}

	const UIsekaiHealthComponent* HealthComponent = Character->GetHealthComponent();
	return FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), HealthComponent->GetHealth(), HealthComponent->MaxHealth));
}

float AIsekaiHUD::GetManaFraction() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return 1.f;
	}

	const UIsekaiSpellComponent* Spells = Character->GetSpells();
	return Spells->MaxMana > 0.f ? Spells->GetMana() / Spells->MaxMana : 0.f;
}

float AIsekaiHUD::GetAttackCooldown() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetSpells()->GetAttackCooldownFraction() : 0.f;
}

float AIsekaiHUD::GetDashCooldown() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetSpells()->GetDashCooldownFraction() : 0.f;
}

bool AIsekaiHUD::CanAffordAttack() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->GetSpells()->GetMana() >= Character->GetSpells()->AttackManaCost;
}

bool AIsekaiHUD::CanAffordDash() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->GetSpells()->GetMana() >= Character->GetSpells()->DashManaCost;
}

bool AIsekaiHUD::CanAffordLight() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && (Character->IsLightOn() || Character->GetSpells()->GetMana() >= Character->GetSpells()->LightManaCost);
}

bool AIsekaiHUD::IsAttackLearned() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->bAttackLearned;
}

bool AIsekaiHUD::IsDashLearned() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->bDashLearned;
}

bool AIsekaiHUD::IsLightLearned() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->bLightLearned;
}

bool AIsekaiHUD::IsLightOn() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->IsLightOn();
}

float AIsekaiHUD::GetBossFraction() const
{
	return Boss.IsValid() ? Boss->GetHealthFraction() : 0.f;
}

FText AIsekaiHUD::GetDialogueSpeaker() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetDialogue()->GetCurrentLine().Speaker : FText::GetEmpty();
}

FText AIsekaiHUD::GetDialogueText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetDialogue()->GetCurrentLine().Text : FText::GetEmpty();
}

bool AIsekaiHUD::IsChoosing() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->GetDialogue()->IsWaitingForChoice();
}

int32 AIsekaiHUD::GetChoiceCount() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetDialogue()->GetCurrentLine().Choices.Num() : 0;
}

FText AIsekaiHUD::GetChoiceText(int32 Index) const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FText::GetEmpty();
	}

	const TArray<FText> Choices = Character->GetDialogue()->GetCurrentLine().Choices;
	return Choices.IsValidIndex(Index) ? Choices[Index] : FText::GetEmpty();
}

float AIsekaiHUD::GetVignetteStrength() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	const UWorld* World = GetWorld();
	if (!Character || !World || !bGameplayVisible)
	{
		return 0.f;
	}

	// 맞은 순간 가장 진하고 0.6초에 걸쳐 사라진다.
	const float Elapsed = World->GetTimeSeconds() - Character->GetLastDamageTime();
	const float Hit = FMath::Clamp(1.f - Elapsed / 0.6f, 0.f, 1.f);

	// 체력이 30% 아래면 은은하게 맥박친다.
	const float HealthNow = GetHealthFraction();
	float Low = 0.f;
	if (HealthNow > 0.f && HealthNow < 0.3f)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(FApp::GetCurrentTime() * 4.f);
		Low = (1.f - HealthNow / 0.3f) * (0.35f + 0.25f * Pulse);
	}

	return FMath::Max(Hit, Low);
}

// ---------------------------------------------------------------------------
// 위젯
// ---------------------------------------------------------------------------

void AIsekaiHUD::BeginPlay()
{
	Super::BeginPlay();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	TWeakObjectPtr<AIsekaiHUD> W(this);
	const FVector2D Shadow(1.f, 1.f);
	const FLinearColor ShadowColor(0.f, 0.f, 0.f, 0.55f);

	auto Gameplay = [W]()
	{
		return W.IsValid() && W->IsGameplayShown() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
	};

	// ---- 기술 칸: 마름모 안에 이름, 아래에 키 ----
	auto SkillCell = [](const TCHAR* Key, const TCHAR* Name, float Size,
		TFunction<float()> CooldownFn, TFunction<bool()> AffordFn, TFunction<bool()> ActiveFn) -> TSharedRef<SWidget>
	{
		const float Inner = Size - 4.f;
		const float Box = Size * 1.45f;

		return SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(Box)
				.HeightOverride(Box)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(Size)
						.HeightOverride(Size)
						.RenderTransform(Diamond())
						.RenderTransformPivot(FVector2D(0.5f, 0.5f))
						[
							SNew(SOverlay)

							// 테두리. 켜져 있으면 금색.
							+ SOverlay::Slot()
							[
								SNew(SImage)
								.Image(White())
								.ColorAndOpacity_Lambda([ActiveFn, AffordFn]()
								{
									if (ActiveFn && ActiveFn())
									{
										return FSlateColor(ColGold);
									}
									return FSlateColor(AffordFn() ? ColLine : WithAlpha(ColLine, 0.5f));
								})
							]

							+ SOverlay::Slot()
							.Padding(2.f)
							[
								SNew(SImage)
								.Image(White())
								.ColorAndOpacity(FSlateColor(ColPanel))
							]

							// 쿨다운. 아래에서부터 차오르는 어두운 막.
							+ SOverlay::Slot()
							.Padding(2.f)
							.VAlign(VAlign_Bottom)
							[
								SNew(SBox)
								.HeightOverride_Lambda([CooldownFn, Inner]()
								{
									return FOptionalSize(Inner * FMath::Clamp(CooldownFn ? CooldownFn() : 0.f, 0.f, 1.f));
								})
								[
									SNew(SImage)
									.Image(White())
									.ColorAndOpacity(FSlateColor(ColCooldown))
								]
							]
						]
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(Font("Bold", Size > 60.f ? 15 : 12))
						.Text(FText::FromString(Name))
						.ColorAndOpacity_Lambda([AffordFn, ActiveFn]()
						{
							if (ActiveFn && ActiveFn())
							{
								return FSlateColor(ColGold);
							}
							return FSlateColor(AffordFn() ? ColText : ColTextDim);
						})
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				KeyBox(FText::FromString(Key), 11)
			];
	};

	// ---- 선택지 한 줄 ----
	auto ChoiceRow = [W](int32 Index) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(560.f)
			.Visibility_Lambda([W, Index]()
			{
				return W.IsValid() && W->IsChoosing() && Index < W->GetChoiceCount() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.Padding(FMargin(0.f, 0.f, 0.f, 10.f))
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(White())
					.ColorAndOpacity(FSlateColor(ColPanel))
				]

				// 왼쪽 금색 띠
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					.WidthOverride(3.f)
					[
						SNew(SImage)
						.Image(White())
						.ColorAndOpacity(FSlateColor(ColGold))
					]
				]

				+ SOverlay::Slot()
				.Padding(FMargin(18.f, 14.f))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						KeyBox(FText::AsNumber(Index + 1))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					.Padding(16.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(Font("Regular", 17))
						.ColorAndOpacity(FSlateColor(ColText))
						.AutoWrapText(true)
						.Text_Lambda([W, Index]()
						{
							return W.IsValid() ? W->GetChoiceText(Index) : FText::GetEmpty();
						})
					]
				]
			];
	};

	TSharedRef<SOverlay> Root = SNew(SOverlay);

	// ================= 가장자리 붉은 기운 =================
	// 그림 없이 그러데이션을 흉내 낸다: 크기가 다른 반투명 띠를 겹쳐 가장자리로 갈수록 진하게.
	{
		const int32 Bands = 10;
		const float Step = 16.f;
		for (int32 Band = 1; Band <= Bands; ++Band)
		{
			const float Thickness = Band * Step;
			auto BandColor = [W]()
			{
				const float Strength = W.IsValid() ? W->GetVignetteStrength() : 0.f;
				return FSlateColor(FLinearColor(0.75f, 0.02f, 0.02f, 0.055f * Strength));
			};

			auto AddEdge = [&Root, &BandColor](EHorizontalAlignment H, EVerticalAlignment V, float Width, float Height)
			{
				Root->AddSlot()
					.HAlign(H)
					.VAlign(V)
					[
						SNew(SBox)
						.WidthOverride(Width > 0.f ? FOptionalSize(Width) : FOptionalSize())
						.HeightOverride(Height > 0.f ? FOptionalSize(Height) : FOptionalSize())
						[
							SNew(SImage)
							.Image(White())
							.ColorAndOpacity_Lambda(BandColor)
						]
					];
			};

			AddEdge(HAlign_Fill, VAlign_Top, 0.f, Thickness);
			AddEdge(HAlign_Fill, VAlign_Bottom, 0.f, Thickness);
			AddEdge(HAlign_Left, VAlign_Fill, Thickness, 0.f);
			AddEdge(HAlign_Right, VAlign_Fill, Thickness, 0.f);
		}
	}

	// ================= 평소 표시 =================

	// 조준점: 작은 마름모. 3인칭에서도 같은 자리.
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Visibility_Lambda(Gameplay)
			[
				Pip(ColText, 6.f)
			]
		];

	// 왼쪽 위: 목표
	Root->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(SafeX, SafeY)
		[
			SNew(SBox)
			.WidthOverride(360.f)
			.Visibility_Lambda([W]()
			{
				const bool bShow = W.IsValid() && W->IsGameplayShown() && !W->GetObjectiveText().IsEmpty();
				return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						Pip(ColGold)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(10.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(Font("Bold", 11, 300))
						.ColorAndOpacity(FSlateColor(ColGold))
						.Text(FText::FromString(TEXT("목표")))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(240.f)
					.HeightOverride(1.f)
					.HAlign(HAlign_Left)
					[
						SNew(SImage)
						.Image(White())
						.ColorAndOpacity(FSlateColor(ColLine))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 18))
					.ColorAndOpacity(FSlateColor(ColText))
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.AutoWrapText(true)
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetObjectiveText() : FText::GetEmpty();
					})
				]
			]
		];

	// 오른쪽 위: 시계
	Root->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(SafeX, SafeY)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda(Gameplay)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Pip(ColText, 5.f)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(10.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(Font("Regular", 16, 150))
				.ColorAndOpacity(FSlateColor(ColText))
				.ShadowOffset(Shadow)
				.ShadowColorAndOpacity(ShadowColor)
				.Text_Lambda([W]()
				{
					return W.IsValid() ? W->GetClockText() : FText::GetEmpty();
				})
			]
		];

	// 위 가운데: 큰 적의 체력
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.f, SafeY + 10.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && W->IsGameplayShown() && W->IsBossShown() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					Pip(ColLag)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(10.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Bold", 14, 400))
					.ColorAndOpacity(FSlateColor(ColText))
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.Text(FText::FromString(TEXT("큰 짐승")))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					Pip(ColLag)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 10.f, 0.f, 0.f)
			[
				Bar(640.f, 6.f,
					[W]() { return W.IsValid() ? W->GetBossFraction() : 0.f; },
					[W]() { return W.IsValid() ? W->GetBossLagFraction() : 0.f; },
					[]() { return FSlateColor(ColHealthLow); })
			]
		];

	// 아래 가운데: 체력과 마나
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, SafeY)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda(Gameplay)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(Font("Regular", 13, 150))
				.ColorAndOpacity(FSlateColor(ColText))
				.ShadowOffset(Shadow)
				.ShadowColorAndOpacity(ShadowColor)
				.Text_Lambda([W]()
				{
					return W.IsValid() ? W->GetHealthText() : FText::GetEmpty();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				Bar(480.f, 8.f,
					[W]() { return W.IsValid() ? W->GetHealthFraction() : 0.f; },
					[W]() { return W.IsValid() ? W->GetHealthLagFraction() : 0.f; },
					[W]()
					{
						const float Fraction = W.IsValid() ? W->GetHealthFraction() : 1.f;
						return FSlateColor(Fraction < 0.3f ? ColHealthLow : ColHealth);
					})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 5.f, 0.f, 0.f)
			[
				Bar(360.f, 3.f,
					[W]() { return W.IsValid() ? W->GetManaFraction() : 0.f; },
					nullptr,
					[]() { return FSlateColor(ColMana); })
			]
		];

	// 오른쪽 아래: 기술
	Root->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(SafeX, SafeY)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda(Gameplay)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SkillCell(TEXT("V"), TEXT("시점"), 40.f, nullptr, []() { return true; }, nullptr)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			.Padding(14.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox)
				.Visibility_Lambda([W]() { return W.IsValid() && W->IsLightLearned() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SkillCell(TEXT("F"), TEXT("빛"), 46.f, nullptr,
						[W]() { return W.IsValid() && W->CanAffordLight(); },
						[W]() { return W.IsValid() && W->IsLightOn(); })
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			.Padding(14.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox)
				.Visibility_Lambda([W]() { return W.IsValid() && W->IsDashLearned() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SkillCell(TEXT("우클릭"), TEXT("가속"), 52.f,
						[W]() { return W.IsValid() ? W->GetDashCooldown() : 0.f; },
						[W]() { return W.IsValid() && W->CanAffordDash(); },
						nullptr)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			.Padding(18.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox)
				.Visibility_Lambda([W]() { return W.IsValid() && W->IsAttackLearned() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SkillCell(TEXT("좌클릭"), TEXT("마법탄"), 68.f,
						[W]() { return W.IsValid() ? W->GetAttackCooldown() : 0.f; },
						[W]() { return W.IsValid() && W->CanAffordAttack(); },
						nullptr)
				]
			]
		];

	// 가운데 오른쪽: 상호작용 안내. 콘솔 게임처럼 대상 옆에 키와 함께 띄운다.
	// 가운데 정렬에서 왼쪽 여백을 주면 그 절반만큼 오른쪽으로 밀린다.
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(520.f, 0.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(FSlateColor(ColPanel))
			.Padding(FMargin(12.f, 8.f, 18.f, 8.f))
			.Visibility_Lambda([W]()
			{
				const bool bShow = W.IsValid() && W->IsGameplayShown() && !W->GetPromptText().IsEmpty();
				return bShow ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					KeyBox(FText::FromString(TEXT("E")))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(12.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 16))
					.ColorAndOpacity(FSlateColor(ColText))
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetPromptText() : FText::GetEmpty();
					})
				]
			]
		];

	// 아래 가운데, 체력 위: 걸으며 나누는 한마디
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, SafeY + 90.f)
		[
			SNew(SBox)
			.MaxDesiredWidth(900.f)
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && W->GetBarkAlpha() > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(Font("Bold", 17))
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColGold, W.IsValid() ? W->GetBarkAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetBarkSpeaker() : FText::GetEmpty();
					})
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				.Padding(14.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 18))
					.AutoWrapText(true)
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColText, W.IsValid() ? W->GetBarkAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetBarkText() : FText::GetEmpty();
					})
				]
			]
		];

	// ================= 대화 =================

	// 아래쪽을 어둡게 까는 그러데이션
	{
		const int32 Bands = 12;
		for (int32 Band = 1; Band <= Bands; ++Band)
		{
			Root->AddSlot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox)
					.HeightOverride(Band * 28.f)
					.Visibility_Lambda([W]()
					{
						return W.IsValid() && W->IsDialogueActive() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
					})
					[
						SNew(SImage)
						.Image(White())
						.ColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.06f)))
					]
				];
		}
	}

	// 대사
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, SafeY + 60.f)
		[
			SNew(SBox)
			.WidthOverride(1040.f)
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && W->IsDialogueActive() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)

				// 이름: 마름모 + 이름 + 가는 선
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						Pip(ColGold, 9.f)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(14.f, 0.f)
					[
						SNew(STextBlock)
						.Font(Font("Bold", 20, 120))
						.ColorAndOpacity(FSlateColor(ColGold))
						.ShadowOffset(Shadow)
						.ShadowColorAndOpacity(ShadowColor)
						.Text_Lambda([W]()
						{
							return W.IsValid() ? W->GetDialogueSpeaker() : FText::GetEmpty();
						})
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.HeightOverride(1.f)
						[
							SNew(SImage)
							.Image(White())
							.ColorAndOpacity(FSlateColor(ColLine))
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(23.f, 16.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 23))
					.ColorAndOpacity(FSlateColor(ColText))
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.AutoWrapText(true)
					.LineHeightPercentage(1.15f)
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetDialogueText() : FText::GetEmpty();
					})
				]

				// 넘기기 안내
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.f, 18.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.Visibility_Lambda([W]()
						{
							return W.IsValid() && W->IsChoosing() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
						})
						[
							KeyBox(FText::FromString(TEXT("E")), 11)
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(10.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(Font("Regular", 13, 100))
						.ColorAndOpacity(FSlateColor(ColTextDim))
						.Text_Lambda([W]()
						{
							const bool bChoosing = W.IsValid() && W->IsChoosing();
							return FText::FromString(bChoosing ? TEXT("숫자 키로 고르기") : TEXT("다음"));
						})
					]
				]
			]
		];

	// 선택지: 오른쪽에 세로로
	Root->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, SafeX + 80.f, 0.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()[ ChoiceRow(0) ]
			+ SVerticalBox::Slot().AutoHeight()[ ChoiceRow(1) ]
			+ SVerticalBox::Slot().AutoHeight()[ ChoiceRow(2) ]
		];

	// ================= 연출 =================

	// 조작 힌트 (느린 화면 속 안내). 가운데 아래, 어두운 띠 위에.
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(0.f, 300.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor(FSlateColor(ColPanel))
			.Padding(FMargin(26.f, 12.f))
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && !W->GetHintText().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					Pip(ColGold, 8.f)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(14.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Bold", 22, 60))
					.ColorAndOpacity(FSlateColor(ColGold))
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetHintText() : FText::GetEmpty();
					})
				]
			]
		];

	// 위 가운데: 성장 알림
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.f, SafeY + 90.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && W->GetToastAlpha() > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					Pip(ColGold, 8.f)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(12.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Bold", 15, 500))
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColGold, W.IsValid() ? W->GetToastAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetToastHeader() : FText::GetEmpty();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					Pip(ColGold, 8.f)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 10.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(Font("Regular", 20))
				.ShadowOffset(Shadow)
				.ShadowColorAndOpacity(ShadowColor)
				.ColorAndOpacity_Lambda([W]()
				{
					return FSlateColor(WithAlpha(ColText, W.IsValid() ? W->GetToastAlpha() : 0.f));
				})
				.Text_Lambda([W]()
				{
					return W.IsValid() ? W->GetToastBody() : FText::GetEmpty();
				})
			]
		];

	// ================= 그림 컷 =================
	Root->AddSlot()
		[
			SNew(SOverlay)
			.Visibility_Lambda([W]()
			{
				return W.IsValid() && W->IsCutsceneActive() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})

			// 바탕
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(White())
				.ColorAndOpacity(FSlateColor(FLinearColor::Black))
			]

			// 그림: 화면을 꽉 채우도록 잘라 맞춘 뒤 천천히 밀고 당긴다.
			+ SOverlay::Slot()
			[
				SNew(SBox)
				.Visibility_Lambda([W]()
				{
					return W.IsValid() && W->HasCutsceneImage() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
				.RenderTransform_Lambda([W]()
				{
					return W.IsValid() ? W->GetCutsceneTransform() : TOptional<FSlateRenderTransform>();
				})
				.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFill)
					[
						SNew(SImage)
						.Image_Lambda([W]() -> const FSlateBrush*
						{
							return W.IsValid() ? W->GetCutsceneBrush() : nullptr;
						})
						.ColorAndOpacity_Lambda([W]()
						{
							return FSlateColor(FLinearColor(1.f, 1.f, 1.f, W.IsValid() ? W->GetCutsceneImageAlpha() : 0.f));
						})
					]
				]
			]

			// 그림이 없을 때: 어떤 그림이 들어갈 자리인지 설명
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 0.f, 140.f)
			[
				SNew(SBox)
				.WidthOverride(900.f)
				.Visibility_Lambda([W]()
				{
					return W.IsValid() && !W->HasCutsceneImage() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
				})
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(Font("Bold", 11, 400))
						.ColorAndOpacity_Lambda([W]()
						{
							return FSlateColor(WithAlpha(ColGold, 0.6f * (W.IsValid() ? W->GetCutsceneImageAlpha() : 0.f)));
						})
						.Text(FText::FromString(TEXT("그림 자리")))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(Font("Regular", 20))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
						.ColorAndOpacity_Lambda([W]()
						{
							return FSlateColor(WithAlpha(ColTextDim, W.IsValid() ? W->GetCutsceneImageAlpha() : 0.f));
						})
						.Text_Lambda([W]()
						{
							return W.IsValid() ? W->GetCutscenePlaceholder() : FText::GetEmpty();
						})
					]
				]
			]

			// 자막 뒤를 어둡게
			+ SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			[
				SNew(SBox)
				.HeightOverride(240.f)
				[
					SNew(SImage)
					.Image(White())
					.ColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.35f)))
				]
			]

			// 자막
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			.Padding(0.f, 0.f, 0.f, SafeY + 70.f)
			[
				SNew(SBox)
				.MaxDesiredWidth(1200.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 26, 60))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.ShadowOffset(Shadow)
					.ShadowColorAndOpacity(ShadowColor)
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColText, W.IsValid() ? W->GetCutsceneSubtitleAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetCutsceneSubtitle() : FText::GetEmpty();
					})
				]
			]

			// 넘기기 안내
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(SafeX, SafeY)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					KeyBox(FText::FromString(TEXT("E")), 11)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(10.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 13, 100))
					.ColorAndOpacity(FSlateColor(ColTextDim))
					.Text(FText::FromString(TEXT("넘기기")))
				]
			]
		];

	// 화면 전체 페이드
	Root->AddSlot()
		[
			SNew(SImage)
			.Image(White())
			.ColorAndOpacity_Lambda([W]()
			{
				return W.IsValid() ? W->GetFadeColor() : FSlateColor(FLinearColor::Transparent);
			})
		];

	// 위아래 검은 띠
	for (const EVerticalAlignment Edge : { VAlign_Top, VAlign_Bottom })
	{
		Root->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(Edge)
			[
				SNew(SBox)
				.HeightOverride_Lambda([W]()
				{
					return FOptionalSize(W.IsValid() ? W->GetLetterboxHeight() : 0.f);
				})
				[
					SNew(SImage)
					.Image(White())
					.ColorAndOpacity(FSlateColor(FLinearColor::Black))
				]
			];
	}

	// 가운데 자막
	Root->AddSlot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(Font("Regular", 28, 80))
			.Justification(ETextJustify::Center)
			.ShadowOffset(Shadow)
			.ShadowColorAndOpacity(ShadowColor)
			.ColorAndOpacity_Lambda([W]()
			{
				return FSlateColor(WithAlpha(ColText, W.IsValid() ? W->GetNarrationAlpha() : 0.f));
			})
			.Text_Lambda([W]()
			{
				return W.IsValid() ? W->GetNarrationText() : FText::GetEmpty();
			})
		];

	// 제목: 가는 선 사이에 넓은 자간의 큰 글씨
	{
		auto TitleLine = [W]() -> TSharedRef<SWidget>
		{
			return SNew(SBox)
				.WidthOverride(560.f)
				.HeightOverride(1.f)
				[
					SNew(SImage)
					.Image(White())
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColLine, W.IsValid() ? W->GetTitleAlpha() : 0.f));
					})
				];
		};

		Root->AddSlot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					TitleLine()
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 22.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Font(Font("Bold", 64, 700))
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColText, W.IsValid() ? W->GetTitleAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetTitleText() : FText::GetEmpty();
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 10.f, 0.f, 22.f)
				[
					SNew(STextBlock)
					.Font(Font("Regular", 18, 300))
					.ColorAndOpacity_Lambda([W]()
					{
						return FSlateColor(WithAlpha(ColGold, W.IsValid() ? W->GetTitleAlpha() : 0.f));
					})
					.Text_Lambda([W]()
					{
						return W.IsValid() ? W->GetSubtitleText() : FText::GetEmpty();
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					TitleLine()
				]
			];
	}

	// 화면 표시가 마우스 입력을 가로채지 않게 한다.
	Root->SetVisibility(EVisibility::HitTestInvisible);

	RootWidget = Root;
	Viewport->AddViewportWidgetContent(Root);
}

void AIsekaiHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RootWidget.IsValid())
	{
		UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
		if (Viewport)
		{
			Viewport->RemoveViewportWidgetContent(RootWidget.ToSharedRef());
		}
		RootWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
