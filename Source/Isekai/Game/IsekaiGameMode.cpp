#include "Game/IsekaiGameMode.h"

#include "EngineUtils.h"
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
}
