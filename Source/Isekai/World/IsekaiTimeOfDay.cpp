#include "World/IsekaiTimeOfDay.h"

#include "Components/LightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

AIsekaiTimeOfDay::AIsekaiTimeOfDay()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AIsekaiTimeOfDay::BeginPlay()
{
	Super::BeginPlay();

	if (!Sun)
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			Sun = *It;
			break;
		}
	}

	if (Sun && Sun->GetLightComponent())
	{
		// 고정(Static/Stationary) 라이트는 실행 중에 돌릴 수 없다.
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		NoonIntensity = Sun->GetLightComponent()->Intensity;
	}

	// 하늘빛이 해를 따라 바뀌려면 스카이 라이트가 하늘을 계속 다시 찍어야 한다.
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* SkyComponent = It->GetLightComponent())
		{
			SkyComponent->SetMobility(EComponentMobility::Movable);
			SkyComponent->bRealTimeCapture = true;
			SkyComponent->MarkRenderStateDirty();
		}
	}

	ApplyLighting();
}

void AIsekaiTimeOfDay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPaused || DayLengthMinutes <= 0.f)
	{
		return;
	}

	const float HoursPerSecond = 24.f / (DayLengthMinutes * 60.f);
	SetTimeOfDay(TimeOfDayHours + DeltaSeconds * HoursPerSecond);
}

void AIsekaiTimeOfDay::SetTimeOfDay(float NewHours)
{
	const int32 OldHour = FMath::FloorToInt(TimeOfDayHours);

	TimeOfDayHours = FMath::Fmod(NewHours, 24.f);
	if (TimeOfDayHours < 0.f)
	{
		TimeOfDayHours += 24.f;
	}

	ApplyLighting();

	const int32 NewHour = FMath::FloorToInt(TimeOfDayHours);
	if (NewHour != OldHour)
	{
		OnHourChanged.Broadcast(NewHour);
	}
}

FString AIsekaiTimeOfDay::GetClockString() const
{
	const int32 Hour = FMath::FloorToInt(TimeOfDayHours);
	const int32 Minute = FMath::FloorToInt((TimeOfDayHours - Hour) * 60.f);
	return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
}

void AIsekaiTimeOfDay::ApplyLighting()
{
	if (!Sun || !Sun->GetLightComponent())
	{
		return;
	}

	// 해의 각도. 6시에 0도(동쪽 지평선), 12시에 90도(머리 위), 18시에 180도(서쪽 지평선).
	const float SunAngle = (TimeOfDayHours - 6.f) * 15.f;
	const float SunHeight = FMath::Sin(FMath::DegreesToRadians(SunAngle));
	const bool bDay = SunHeight >= 0.f;

	// 밤에는 같은 빛을 반대편으로 돌려 달로 쓴다. 달은 해의 정반대에 있다.
	const float LightAngle = bDay ? SunAngle : SunAngle - 180.f;
	const float LightHeight = FMath::Abs(SunHeight);

	// 디렉셔널 라이트는 Pitch -90이 정수리에서 내리쬐는 방향이다.
	Sun->SetActorRotation(FRotator(-LightAngle, SunYaw, 0.f));

	ULightComponent* Light = Sun->GetLightComponent();

	if (bDay)
	{
		// 해가 낮으면 붉고, 지평선에 닿으면 꺼진다.
		const float Warmth = 1.f - FMath::SmoothStep(0.f, 0.4f, LightHeight);
		const float Strength = FMath::SmoothStep(0.f, 0.18f, LightHeight);

		Light->SetLightColor(FMath::Lerp(NoonColor, HorizonColor, Warmth));
		Light->SetIntensity(NoonIntensity * Strength);
	}
	else
	{
		const float Strength = FMath::SmoothStep(0.f, 0.25f, LightHeight);

		Light->SetLightColor(MoonColor);
		Light->SetIntensity(NoonIntensity * MoonStrength * Strength);
	}
}
