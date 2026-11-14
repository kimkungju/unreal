#include "World/IsekaiTimeOfDay.h"

#include "Components/SceneComponent.h"
#include "Engine/DirectionalLight.h"
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

	if (Sun && Sun->GetRootComponent())
	{
		// 고정(Static/Stationary) 라이트는 실행 중에 돌릴 수 없다.
		Sun->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	}

	ApplySunRotation();
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

	ApplySunRotation();

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

void AIsekaiTimeOfDay::ApplySunRotation()
{
	if (!Sun)
	{
		return;
	}

	// 디렉셔널 라이트는 Pitch -90이 정수리에서 내리쬐는 방향이다.
	// 6시 0도(지평선), 12시 -90도, 18시 -180도, 자정 -270도(땅 밑).
	const float Pitch = -(TimeOfDayHours - 6.f) / 24.f * 360.f;
	Sun->SetActorRotation(FRotator(Pitch, SunYaw, 0.f));
}
