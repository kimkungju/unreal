#include "Game/IsekaiGameMode.h"

#include "Combat/IsekaiTargetDummy.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Player/IsekaiCharacter.h"
#include "UI/IsekaiHUD.h"
#include "World/IsekaiTimeOfDay.h"

AIsekaiGameMode::AIsekaiGameMode()
{
	DefaultPawnClass = AIsekaiCharacter::StaticClass();
	HUDClass = AIsekaiHUD::StaticClass();
}

void AIsekaiGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 레벨에 시간 관리 액터를 놓지 않았어도 낮밤이 돌도록 하나 만든다.
	const TActorIterator<AIsekaiTimeOfDay> Existing(GetWorld());
	if (!Existing)
	{
		GetWorld()->SpawnActor<AIsekaiTimeOfDay>();
	}

	if (bSpawnTestDummies)
	{
		SpawnTestDummies();
	}
}

void AIsekaiGameMode::SpawnTestDummies()
{
	UWorld* World = GetWorld();

	const TActorIterator<AIsekaiTargetDummy> ExistingDummy(World);
	if (ExistingDummy)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		Origin = It->GetActorLocation();
		Facing = FRotator(0.f, It->GetActorRotation().Yaw, 0.f);
		break;
	}

	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);

	for (int32 Index = -1; Index <= 1; ++Index)
	{
		FVector Location = Origin + Forward * 800.f + Right * (Index * 300.f);

		// 바닥 높이를 찾아 그 위에 세운다. 허수아비는 높이 180, 중심이 원점이다.
		FHitResult Ground;
		const FVector TraceStart = Location + FVector(0.f, 0.f, 500.f);
		const FVector TraceEnd = Location - FVector(0.f, 0.f, 2000.f);
		if (World->LineTraceSingleByChannel(Ground, TraceStart, TraceEnd, ECC_Visibility))
		{
			Location.Z = Ground.ImpactPoint.Z + 90.f;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AIsekaiTargetDummy>(AIsekaiTargetDummy::StaticClass(), Location, Facing, Params);
	}
}
