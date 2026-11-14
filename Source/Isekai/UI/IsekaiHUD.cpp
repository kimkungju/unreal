#include "UI/IsekaiHUD.h"

#include "Combat/IsekaiHealthComponent.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Dialogue/IsekaiObjectiveComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/IsekaiInteractionComponent.h"
#include "Misc/App.h"
#include "Player/IsekaiCharacter.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "World/IsekaiTimeOfDay.h"

AIsekaiHUD::AIsekaiHUD()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AIsekaiHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 느린 화면 중에도 화면 연출은 실제 시간으로 움직인다.
	const float RealDelta = FApp::GetDeltaTime();

	FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, FadeTarget, RealDelta, FadeSpeed);
	NarrationAlpha = FMath::FInterpConstantTo(NarrationAlpha, NarrationTarget, RealDelta, 2.5f);
	TitleAlpha = FMath::FInterpConstantTo(TitleAlpha, TitleTarget, RealDelta, 1.2f);
	LetterboxAmount = FMath::FInterpTo(LetterboxAmount, bLetterbox ? 1.f : 0.f, RealDelta, 4.f);
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

FSlateColor AIsekaiHUD::GetFadeColor() const
{
	return FSlateColor(FLinearColor(FadeColor.R, FadeColor.G, FadeColor.B, FadeAlpha));
}

FSlateColor AIsekaiHUD::GetNarrationColor() const
{
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, NarrationAlpha));
}

FSlateColor AIsekaiHUD::GetTitleColor() const
{
	return FSlateColor(FLinearColor(1.f, 1.f, 1.f, TitleAlpha));
}

void AIsekaiHUD::BeginPlay()
{
	Super::BeginPlay();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	const FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 18);
	const FSlateFontInfo SpeakerFont = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	const FSlateFontInfo DialogueFont = FCoreStyle::GetDefaultFontStyle("Regular", 22);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	const FSlateFontInfo NarrationFont = FCoreStyle::GetDefaultFontStyle("Regular", 28);
	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Bold", 26);
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 72);
	const FSlateFontInfo SubtitleFont = FCoreStyle::GetDefaultFontStyle("Regular", 24);

	const FSlateColor White(FLinearColor::White);
	const FSlateColor Black(FLinearColor::Black);
	const FSlateColor ManaColor(FLinearColor(0.5f, 0.8f, 1.f));
	const FSlateColor HealthColor(FLinearColor(1.f, 0.45f, 0.4f));
	const FSlateColor GoldColor(FLinearColor(1.f, 0.85f, 0.5f));
	const FVector2D Shadow(1.f, 1.f);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	TWeakObjectPtr<AIsekaiHUD> WeakThis(this);

	// ---------- 평소 표시 ----------

	TSharedRef<SOverlay> Gameplay = SNew(SOverlay)
		.Visibility_Lambda([WeakThis]()
		{
			return WeakThis.IsValid() && WeakThis->IsGameplayVisible() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})

		// 조준점
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(4.f)
			.HeightOverride(4.f)
			.Visibility_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() && WeakThis->IsDialogueActive() ? EVisibility::Collapsed : EVisibility::Visible;
			})
			[
				SNew(SColorBlock)
				.Color(FLinearColor::White)
			]
		]

		// 시계
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(24.f)
		[
			SNew(STextBlock)
			.Font(BodyFont)
			.ColorAndOpacity(White)
			.ShadowOffset(Shadow)
			.Text_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetClockText() : FText::GetEmpty();
			})
		]

		// 목표
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(24.f)
		[
			SNew(STextBlock)
			.Font(BodyFont)
			.ColorAndOpacity(GoldColor)
			.ShadowOffset(Shadow)
			.Text_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetObjectiveText() : FText::GetEmpty();
			})
		]

		// 체력과 마나
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(24.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(BodyFont)
				.ColorAndOpacity(HealthColor)
				.ShadowOffset(Shadow)
				.Text_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetHealthText() : FText::GetEmpty();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(BodyFont)
				.ColorAndOpacity(ManaColor)
				.ShadowOffset(Shadow)
				.Text_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetManaText() : FText::GetEmpty();
				})
			]
		]

		// 상호작용 안내 (조준점 조금 아래)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(0.f, 90.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(BodyFont)
			.ColorAndOpacity(White)
			.ShadowOffset(Shadow)
			.Text_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetPromptText() : FText::GetEmpty();
			})
		];

	// ---------- 전체 ----------

	TSharedRef<SOverlay> Overlay = SNew(SOverlay)

		// 맞았을 때 화면을 덮는 붉은 막
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(WhiteBrush)
			.ColorAndOpacity_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetDamageFlashColor() : FSlateColor(FLinearColor::Transparent);
			})
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			Gameplay
		]

		// 조작 힌트. 평소 표시와 달리 연출 중에도 보인다.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(0.f, 220.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Font(HintFont)
			.ColorAndOpacity(GoldColor)
			.ShadowOffset(Shadow)
			.Text_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetHintText() : FText::GetEmpty();
			})
		]

		// 화면 전체 페이드
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage)
			.Image(WhiteBrush)
			.ColorAndOpacity_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetFadeColor() : FSlateColor(FLinearColor::Transparent);
			})
		]

		// 위쪽 검은 띠
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.HeightOverride_Lambda([WeakThis]()
			{
				return FOptionalSize(WeakThis.IsValid() ? WeakThis->GetLetterboxHeight() : 0.f);
			})
			[
				SNew(SImage)
				.Image(WhiteBrush)
				.ColorAndOpacity(Black)
			]
		]

		// 아래쪽 검은 띠
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		[
			SNew(SBox)
			.HeightOverride_Lambda([WeakThis]()
			{
				return FOptionalSize(WeakThis.IsValid() ? WeakThis->GetLetterboxHeight() : 0.f);
			})
			[
				SNew(SImage)
				.Image(WhiteBrush)
				.ColorAndOpacity(Black)
			]
		]

		// 가운데 자막
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(NarrationFont)
			.Justification(ETextJustify::Center)
			.ShadowOffset(Shadow)
			.ColorAndOpacity_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetNarrationColor() : FSlateColor(FLinearColor::Transparent);
			})
			.Text_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() ? WeakThis->GetNarrationText() : FText::GetEmpty();
			})
		]

		// 제목
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(TitleFont)
				.ColorAndOpacity_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetTitleColor() : FSlateColor(FLinearColor::Transparent);
				})
				.Text_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetTitleText() : FText::GetEmpty();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 16.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(SubtitleFont)
				.ColorAndOpacity_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetTitleColor() : FSlateColor(FLinearColor::Transparent);
				})
				.Text_Lambda([WeakThis]()
				{
					return WeakThis.IsValid() ? WeakThis->GetSubtitleText() : FText::GetEmpty();
				})
			]
		]

		// 대화 상자
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, 130.f)
		[
			SNew(SBox)
			.WidthOverride(900.f)
			.Visibility_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() && WeakThis->IsDialogueActive() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.75f)))
				.Padding(28.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(SpeakerFont)
						.ColorAndOpacity(GoldColor)
						.Text_Lambda([WeakThis]()
						{
							return WeakThis.IsValid() ? WeakThis->GetDialogueSpeaker() : FText::GetEmpty();
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 12.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(DialogueFont)
						.ColorAndOpacity(White)
						.AutoWrapText(true)
						.Text_Lambda([WeakThis]()
						{
							return WeakThis.IsValid() ? WeakThis->GetDialogueText() : FText::GetEmpty();
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Font(SmallFont)
						.ColorAndOpacity(White)
						.Text(FText::FromString(TEXT("[E] 다음")))
					]
				]
			]
		];

	// 화면 표시가 마우스 입력을 가로채지 않게 한다.
	Overlay->SetVisibility(EVisibility::HitTestInvisible);

	RootWidget = Overlay;
	Viewport->AddViewportWidgetContent(Overlay);
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

const AIsekaiCharacter* AIsekaiHUD::GetCharacter() const
{
	return Cast<AIsekaiCharacter>(GetOwningPawn());
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

FText AIsekaiHUD::GetManaText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FText::GetEmpty();
	}

	const UIsekaiSpellComponent* Spells = Character->GetSpells();
	return FText::FromString(FString::Printf(TEXT("MP %.0f / %.0f"), Spells->GetMana(), Spells->MaxMana));
}

FText AIsekaiHUD::GetHealthText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character)
	{
		return FText::GetEmpty();
	}

	const UIsekaiHealthComponent* HealthComponent = Character->GetHealthComponent();
	return FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f"), HealthComponent->GetHealth(), HealthComponent->MaxHealth));
}

FText AIsekaiHUD::GetObjectiveText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character ? Character->GetObjectives()->GetObjective() : FText::GetEmpty();
}

FSlateColor AIsekaiHUD::GetDamageFlashColor() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	const UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return FSlateColor(FLinearColor::Transparent);
	}

	// 맞은 순간 가장 진하고 0.5초에 걸쳐 사라진다.
	const float Elapsed = World->GetTimeSeconds() - Character->GetLastDamageTime();
	const float Strength = FMath::Clamp(1.f - Elapsed / 0.5f, 0.f, 1.f);
	return FSlateColor(FLinearColor(0.8f, 0.f, 0.f, 0.4f * Strength));
}

FText AIsekaiHUD::GetPromptText() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	if (!Character || IsDialogueActive())
	{
		return FText::GetEmpty();
	}

	const FText Prompt = Character->GetInteraction()->GetFocusedPrompt();
	if (Prompt.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Printf(TEXT("[E] %s"), *Prompt.ToString()));
}

bool AIsekaiHUD::IsDialogueActive() const
{
	const AIsekaiCharacter* Character = GetCharacter();
	return Character && Character->GetDialogue()->IsDialogueActive();
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
