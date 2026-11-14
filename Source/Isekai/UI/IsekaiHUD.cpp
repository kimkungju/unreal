#include "UI/IsekaiHUD.h"

#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Interaction/IsekaiInteractionComponent.h"
#include "Player/IsekaiCharacter.h"
#include "World/IsekaiTimeOfDay.h"

void AIsekaiHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	// 조준점
	const float DotSize = 4.f;
	DrawRect(FLinearColor::White, CenterX - DotSize * 0.5f, CenterY - DotSize * 0.5f, DotSize, DotSize);

	// 상호작용 안내
	if (const AIsekaiCharacter* Character = Cast<AIsekaiCharacter>(GetOwningPawn()))
	{
		const FText Prompt = Character->GetInteraction()->GetFocusedPrompt();
		if (!Prompt.IsEmpty())
		{
			const FString Line = FString::Printf(TEXT("[E] %s"), *Prompt.ToString());
			const float Scale = 1.5f;
			float Width = 0.f;
			float Height = 0.f;
			GetTextSize(Line, Width, Height, nullptr, Scale);
			DrawText(Line, FLinearColor::White, CenterX - Width * 0.5f, CenterY + 40.f, nullptr, Scale);
		}
	}

	// 시계
	if (!TimeOfDay.IsValid())
	{
		for (TActorIterator<AIsekaiTimeOfDay> It(GetWorld()); It; ++It)
		{
			TimeOfDay = *It;
			break;
		}
	}
	if (TimeOfDay.IsValid())
	{
		DrawText(TimeOfDay->GetClockString(), FLinearColor::White, 24.f, 24.f, nullptr, 1.5f);
	}
}
