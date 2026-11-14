#include "Combat/IsekaiEnemy.h"

#include "Combat/IsekaiHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiEnemy::AIsekaiEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// 레벨에 놓았든 실행 중에 만들었든 AI 컨트롤러가 붙게 한다.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->bRunPhysicsWithNoController = true;

	Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
	Placeholder->SetupAttachment(GetCapsuleComponent());
	Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Placeholder->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.7f));

	// NPC(원기둥)와 구분되도록 원뿔을 쓴다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		Placeholder->SetStaticMesh(ConeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ShardMesh = CubeMesh.Object;
	}

	Health = CreateDefaultSubobject<UIsekaiHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 75.f;
}

void AIsekaiEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	PlaceholderBaseScale = Placeholder->GetRelativeScale3D();

	Health->OnHealthChanged.AddDynamic(this, &AIsekaiEnemy::HandleHealthChanged);
	Health->OnDied.AddDynamic(this, &AIsekaiEnemy::HandleDied);
}

void AIsekaiEnemy::StartChasing()
{
	if (State == EState::Idle)
	{
		SetState(EState::Chase);
	}
}

void AIsekaiEnemy::MakeElite()
{
	// 커진 만큼 발이 땅에 묻히지 않도록 먼저 들어 올린다.
	AddActorWorldOffset(FVector(0.f, 0.f, 60.f));
	SetActorScale3D(FVector(1.6f));
	Health->MaxHealth = 175.f;
	Health->ResetHealth();
	AttackDamage = 30.f;
	AttackRange = 240.f;
	WindupTime = 1.0f;
	RecoverTime = 1.6f;
	MoveSpeed = 330.f;
	KnockbackSpeed = 150.f;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

void AIsekaiEnemy::SetState(EState NewState)
{
	State = NewState;
	StateTime = 0.f;

	// 예비 동작 중에는 몸을 부풀려서 곧 공격한다는 것을 알린다.
	const float ScaleFactor = (State == EState::Windup) ? 1.25f : 1.f;
	Placeholder->SetRelativeScale3D(PlaceholderBaseScale * ScaleFactor);
}

void AIsekaiEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EState::Dead)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}

	StateTime += DeltaSeconds;

	const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	const double Distance = ToPlayer.Size2D();
	const FVector Direction = ToPlayer.GetSafeNormal2D();

	switch (State)
	{
	case EState::Idle:
		if (Distance <= DetectRange)
		{
			SetState(EState::Chase);
		}
		break;

	case EState::Chase:
		if (Distance > LoseRange)
		{
			SetState(EState::Idle);
		}
		else if (Distance <= AttackRange)
		{
			SetState(EState::Windup);
		}
		else
		{
			AddMovementInput(Direction);
		}
		break;

	case EState::Windup:
		if (!Direction.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
		}

		if (StateTime >= WindupTime)
		{
			// 예비 동작 동안 충분히 물러났으면 맞지 않는다.
			if (Distance <= AttackRange + 60.f)
			{
				UGameplayStatics::ApplyDamage(PlayerPawn, AttackDamage, GetController(), this, nullptr);
			}
			SetState(EState::Recover);
		}
		break;

	case EState::Recover:
		if (StateTime >= RecoverTime)
		{
			SetState(EState::Chase);
		}
		break;

	default:
		break;
	}
}

void AIsekaiEnemy::HandleHealthChanged(float NewHealth, float Delta)
{
	if (Delta >= 0.f || State == EState::Dead)
	{
		return;
	}

	// 맞으면 플레이어 반대쪽으로 밀린다.
	if (KnockbackSpeed > 0.f)
	{
		if (const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector Away = (GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
			LaunchCharacter(Away * KnockbackSpeed + FVector(0.f, 0.f, 120.f), true, true);
		}
	}

	// 멀리서 맞아도 쫓아오게 한다.
	if (State == EState::Idle)
	{
		SetState(EState::Chase);
	}
}

void AIsekaiEnemy::HandleDied(AActor* Killer)
{
	SetState(EState::Dead);
	SpawnShards();
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(0.5f);
}

void AIsekaiEnemy::SpawnShards()
{
	UWorld* World = GetWorld();
	if (!World || !ShardMesh)
	{
		return;
	}

	const float SizeScale = GetActorScale3D().X;

	for (int32 Index = 0; Index < 8; ++Index)
	{
		const FVector Offset(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-60.f, 60.f));

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AStaticMeshActor* Shard = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),
			GetActorLocation() + Offset * SizeScale, FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), 0.f), Params);
		if (!Shard)
		{
			continue;
		}

		Shard->SetMobility(EComponentMobility::Movable);
		Shard->SetActorScale3D(FVector(FMath::FRandRange(0.15f, 0.35f) * SizeScale));

		UStaticMeshComponent* ShardComponent = Shard->GetStaticMeshComponent();
		ShardComponent->SetStaticMesh(ShardMesh);
		ShardComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
		ShardComponent->SetSimulatePhysics(true);

		const FVector Burst = (Offset.GetSafeNormal() + FVector(0.f, 0.f, 0.8f)) * FMath::FRandRange(350.f, 650.f);
		ShardComponent->AddImpulse(Burst, NAME_None, true);

		Shard->SetLifeSpan(2.5f);
	}
}
