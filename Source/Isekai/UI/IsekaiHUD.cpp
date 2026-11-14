#include "UI/IsekaiHUD.h"

#include "Combat/IsekaiSpellComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/IsekaiInteractionComponent.h"
#include "Player/IsekaiCharacter.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "World/IsekaiTimeOfDay.h"

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
	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);

	const FSlateColor White(FLinearColor::White);
	const FSlateColor ManaColor(FLinearColor(0.5f, 0.8f, 1.f));
	const FSlateColor SpeakerColor(FLinearColor(1.f, 0.85f, 0.5f));
	const FVector2D Shadow(1.f, 1.f);

	TWeakObjectPtr<AIsekaiHUD> WeakThis(this);

	TSharedRef<SOverlay> Overlay = SNew(SOverlay)

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

		// 마나
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(24.f)
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
		]

		// 대화 상자
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, 80.f)
		[
			SNew(SBox)
			.WidthOverride(900.f)
			.Visibility_Lambda([WeakThis]()
			{
				return WeakThis.IsValid() && WeakThis->IsDialogueActive() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.75f)))
				.Padding(28.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(SpeakerFont)
						.ColorAndOpacity(SpeakerColor)
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
						.Font(HintFont)
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
