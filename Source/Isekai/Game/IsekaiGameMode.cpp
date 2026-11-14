#include "Game/IsekaiGameMode.h"

#include "Combat/IsekaiEnemy.h"
#include "Combat/IsekaiTargetDummy.h"
#include "Dialogue/IsekaiNPC.h"
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

	if (bSpawnTestNPC)
	{
		SpawnTestNPC();
	}

	if (bSpawnTestEnemies)
	{
		SpawnTestEnemies();
	}
}

void AIsekaiGameMode::GetStartTransform(FVector& OutLocation, FRotator& OutFacing) const
{
	OutLocation = FVector::ZeroVector;
	OutFacing = FRotator::ZeroRotator;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		OutLocation = It->GetActorLocation();
		OutFacing = FRotator(0.f, It->GetActorRotation().Yaw, 0.f);
		break;
	}
}

float AIsekaiGameMode::FindGroundZ(const FVector& Location) const
{
	FHitResult Ground;
	const FVector TraceStart = Location + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd = Location - FVector(0.f, 0.f, 2000.f);
	if (GetWorld()->LineTraceSingleByChannel(Ground, TraceStart, TraceEnd, ECC_Visibility))
	{
		return Ground.ImpactPoint.Z;
	}

	return Location.Z;
}

void AIsekaiGameMode::SpawnTestDummies()
{
	UWorld* World = GetWorld();

	const TActorIterator<AIsekaiTargetDummy> ExistingDummy(World);
	if (ExistingDummy)
	{
		return;
	}

	FVector Origin;
	FRotator Facing;
	GetStartTransform(Origin, Facing);

	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);

	for (int32 Index = -1; Index <= 1; ++Index)
	{
		FVector Location = Origin + Forward * 800.f + Right * (Index * 300.f);

		// 허수아비는 높이 180, 중심이 원점이다.
		Location.Z = FindGroundZ(Location) + 90.f;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AIsekaiTargetDummy>(AIsekaiTargetDummy::StaticClass(), Location, Facing, Params);
	}
}

void AIsekaiGameMode::SpawnTestEnemies()
{
	UWorld* World = GetWorld();

	const TActorIterator<AIsekaiEnemy> ExistingEnemy(World);
	if (ExistingEnemy)
	{
		return;
	}

	FVector Origin;
	FRotator Facing;
	GetStartTransform(Origin, Facing);

	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);

	// 시작 지점에서는 감지되지 않는 거리(25m)에 둔다. 다가가야 싸움이 시작된다.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const float Side = (Index == 0) ? -250.f : 250.f;
		FVector Location = Origin + Forward * 2500.f + Right * Side;
		Location.Z = FindGroundZ(Location) + 90.f;

		const FRotator LookAtStart(0.f, (Origin - Location).Rotation().Yaw, 0.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AIsekaiEnemy>(AIsekaiEnemy::StaticClass(), Location, LookAtStart, Params);
	}
}

void AIsekaiGameMode::SpawnTestNPC()
{
	UWorld* World = GetWorld();

	const TActorIterator<AIsekaiNPC> ExistingNPC(World);
	if (ExistingNPC)
	{
		return;
	}

	FVector Origin;
	FRotator Facing;
	GetStartTransform(Origin, Facing);

	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);

	// 시작 지점의 오른쪽 앞에, 플레이어 쪽을 보게 세운다. 캡슐 절반 높이는 88이다.
	FVector Location = Origin + Forward * 350.f + Right * 350.f;
	Location.Z = FindGroundZ(Location) + 90.f;

	const FRotator LookAtPlayer(0.f, (Origin - Location).Rotation().Yaw, 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIsekaiNPC* NPC = World->SpawnActor<AIsekaiNPC>(AIsekaiNPC::StaticClass(), Location, LookAtPlayer, Params);
	if (!NPC)
	{
		return;
	}

	NPC->DisplayName = FText::FromString(TEXT("낯선 마법사"));

	auto AddLine = [NPC](const TCHAR* Speaker, const TCHAR* Text)
	{
		FIsekaiDialogueLine Line;
		Line.Speaker = FText::FromString(Speaker);
		Line.Text = FText::FromString(Text);
		NPC->Lines.Add(Line);
	};

	AddLine(TEXT(""), TEXT("눈을 떴구나. 길가에 쓰러져 있길래 여기까지 옮겨 놨다."));
	AddLine(TEXT("나"), TEXT("여기는... 어디죠?"));
	AddLine(TEXT(""), TEXT("그걸 모른다니, 꽤 멀리서 온 모양이군. 손을 앞으로 뻗어 봐라. 마력이 흐르는지 보자."));
	AddLine(TEXT(""), TEXT("저기 세워 둔 기둥이 보이지? 마우스 왼쪽으로 쏴 봐라."));
}
