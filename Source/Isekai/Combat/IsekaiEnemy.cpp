#include "Combat/IsekaiEnemy.h"

#include "Audio/IsekaiAudio.h"
#include "Combat/IsekaiHealthComponent.h"
#include "Combat/IsekaiSpellProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	UStaticMesh* Sphere = SphereMesh.Object;
	UStaticMesh* Cylinder = CylinderMesh.Object;
	UStaticMesh* Cone = ConeMesh.Object;
	ShardMesh = CubeMesh.Object;

	// 뼈대. 앞이 +X, 캡슐의 가운데가 원점, 발바닥이 Z -88이다.
	Rig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig"));
	Rig->SetupAttachment(GetCapsuleComponent());

	// 몸통: 앞뒤로 긴 타원체.
	Torso = AddPart(TEXT("Torso"), Sphere, Rig, FVector(0.f, 0.f, -8.f), FRotator::ZeroRotator, FVector(1.3f, 0.7f, 0.62f));

	// 머리: 몸통 앞, 조금 위.
	Head = AddPart(TEXT("Head"), Sphere, Rig, FVector(72.f, 0.f, 14.f), FRotator::ZeroRotator, FVector(0.44f, 0.4f, 0.38f));

	// 꼬리: 뒤쪽 위로 뻗은 원뿔. Pitch 70이면 원뿔의 끝이 뒤쪽 위를 향한다.
	Tail = AddPart(TEXT("Tail"), Cone, Rig, FVector(-82.f, 0.f, 6.f), FRotator(70.f, 0.f, 0.f), FVector(0.2f, 0.2f, 0.62f));

	// 목: 원거리 종류에만 보인다. 앞으로 기운 긴 원기둥.
	Neck = AddPart(TEXT("Neck"), Cylinder, Rig, FVector(58.f, 0.f, 42.f), FRotator(-28.f, 0.f, 0.f), FVector(0.2f, 0.2f, 0.95f));
	Neck->SetVisibility(false);

	// 뿔: 큰 종류에만 보인다.
	HornLeft = AddPart(TEXT("HornLeft"), Cone, Head, FVector(4.f, -28.f, 52.f), FRotator(0.f, 0.f, -18.f), FVector(0.3f, 0.3f, 0.9f));
	HornRight = AddPart(TEXT("HornRight"), Cone, Head, FVector(4.f, 28.f, 52.f), FRotator(0.f, 0.f, 18.f), FVector(0.3f, 0.3f, 0.9f));
	HornLeft->SetVisibility(false);
	HornRight->SetVisibility(false);

	// 다리 넷. 엉덩이 관절을 따로 두어야 다리가 관절을 축으로 흔들린다.
	const FVector HipOffsets[4] =
	{
		FVector(42.f, -24.f, -26.f),
		FVector(42.f, 24.f, -26.f),
		FVector(-42.f, -24.f, -26.f),
		FVector(-42.f, 24.f, -26.f),
	};
	const TCHAR* const HipNames[4] = { TEXT("HipFrontLeft"), TEXT("HipFrontRight"), TEXT("HipBackLeft"), TEXT("HipBackRight") };
	const TCHAR* const LegNames[4] = { TEXT("LegFrontLeft"), TEXT("LegFrontRight"), TEXT("LegBackLeft"), TEXT("LegBackRight") };

	for (int32 Index = 0; Index < 4; ++Index)
	{
		USceneComponent* Hip = CreateDefaultSubobject<USceneComponent>(HipNames[Index]);
		Hip->SetupAttachment(Rig);
		Hip->SetRelativeLocation(HipOffsets[Index]);
		LegPivots.Add(Hip);

		// 다리 길이 62. 관절에서 아래로 절반만큼 내려 달아 발바닥이 Z -88에 닿게 한다.
		AddPart(LegNames[Index], Cylinder, Hip, FVector(0.f, 0.f, -31.f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.62f));
	}

	EyeGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeGlow"));
	EyeGlow->SetupAttachment(Head);
	EyeGlow->SetUsingAbsoluteScale(true);
	EyeGlow->SetRelativeLocation(FVector(40.f, 0.f, 10.f));
	EyeGlow->SetIntensity(0.f);
	EyeGlow->SetAttenuationRadius(450.f);
	EyeGlow->SetLightColor(FLinearColor(1.f, 0.15f, 0.1f));
	EyeGlow->SetCastShadows(false);

	Health = CreateDefaultSubobject<UIsekaiHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 75.f;
}

UStaticMeshComponent* AIsekaiEnemy::AddPart(const TCHAR* PartName, UStaticMesh* Shape, USceneComponent* Parent,
	const FVector& Location, const FRotator& Rotation, const FVector& Scale)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Shape)
	{
		Part->SetStaticMesh(Shape);
	}
	return Part;
}

void AIsekaiEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	// 여럿이 함께 걸을 때 발이 똑같이 맞지 않게 한다.
	WalkPhase = FMath::FRandRange(0.f, 6.28f);

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

void AIsekaiEnemy::MakeRanged()
{
	bRanged = true;
	Health->MaxHealth = 50.f;
	Health->ResetHealth();
	AttackDamage = 12.f;
	WindupTime = 0.8f;
	RecoverTime = 1.8f;
	MoveSpeed = 260.f;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	// 붙어서 싸우는 종류와 한눈에 구분되게: 가는 몸통, 긴 목, 높이 달린 머리.
	Torso->SetRelativeScale3D(FVector(1.05f, 0.5f, 0.5f));
	Neck->SetVisibility(true);
	Head->SetRelativeLocation(FVector(84.f, 0.f, 92.f));
	Head->SetRelativeScale3D(FVector(0.36f, 0.3f, 0.3f));
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

	HornLeft->SetVisibility(true);
	HornRight->SetVisibility(true);
	Torso->SetRelativeScale3D(FVector(1.4f, 0.85f, 0.75f));
}

void AIsekaiEnemy::FireShot(const APawn* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	// 머리에서 쏜다.
	const FVector Muzzle = Head->GetComponentLocation();
	const FVector Aim = (Target->GetActorLocation() - Muzzle).GetSafeNormal();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIsekaiSpellProjectile* Shot = World->SpawnActor<AIsekaiSpellProjectile>(AIsekaiSpellProjectile::StaticClass(),
		Muzzle + Aim * 70.f, Aim.Rotation(), Params);
	if (Shot)
	{
		Shot->ConfigureAsEnemyShot(ShotSpeed, AttackDamage);
		IsekaiAudio::Play(this, TEXT("SFX_Cast"), 0.45f, 0.1f);
	}
}

void AIsekaiEnemy::SetState(EState NewState)
{
	State = NewState;
	StateTime = 0.f;
}

void AIsekaiEnemy::UpdateRig(float DeltaSeconds)
{
	// 걷기: 대각선에 있는 다리끼리 같은 방향으로 흔든다.
	const float Speed = GetVelocity().Size2D();
	const bool bWalking = Speed > 15.f;

	if (bWalking)
	{
		WalkPhase += DeltaSeconds * (5.f + Speed * 0.028f);
	}
	LegSwing = FMath::FInterpTo(LegSwing, bWalking ? 34.f : 0.f, DeltaSeconds, 8.f);

	const float Swing = FMath::Sin(WalkPhase) * LegSwing;
	for (int32 Index = 0; Index < LegPivots.Num(); ++Index)
	{
		const float Side = (Index == 0 || Index == 3) ? 1.f : -1.f;
		LegPivots[Index]->SetRelativeRotation(FRotator(Swing * Side, 0.f, 0.f));
	}

	// 자세: 공격 직전에는 몸을 일으키고 부풀린다. 공격 뒤에는 앞으로 처진다.
	float WantedPitch = 0.f;
	float WantedScale = 1.f;
	float WantedGlow = 0.f;

	if (State == EState::Windup)
	{
		const float Progress = WindupTime > 0.f ? FMath::Clamp(StateTime / WindupTime, 0.f, 1.f) : 1.f;
		WantedPitch = 24.f;
		WantedScale = 1.15f;
		WantedGlow = 6000.f * Progress;
	}
	else if (State == EState::Recover)
	{
		WantedPitch = -12.f;
	}

	PosePitch = FMath::FInterpTo(PosePitch, WantedPitch, DeltaSeconds, 12.f);
	PoseScale = FMath::FInterpTo(PoseScale, WantedScale, DeltaSeconds, 12.f);
	HitPunch = FMath::FInterpTo(HitPunch, 0.f, DeltaSeconds, 10.f);

	// 걸을 때 몸통이 살짝 오르내린다.
	const float Bob = FMath::Sin(WalkPhase * 2.f) * 2.5f * (LegSwing / 34.f);

	Rig->SetRelativeLocation(FVector(0.f, 0.f, Bob));
	Rig->SetRelativeRotation(FRotator(PosePitch, 0.f, 0.f));
	Rig->SetRelativeScale3D(FVector(PoseScale + HitPunch * 0.22f));

	// 꼬리는 걸음에 맞춰 좌우로 흔들린다.
	Tail->SetRelativeRotation(FRotator(70.f, FMath::Sin(WalkPhase * 0.5f) * 14.f, 0.f));

	EyeGlow->SetIntensity(WantedGlow);
}

void AIsekaiEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == EState::Dead)
	{
		return;
	}

	UpdateRig(DeltaSeconds);

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
		else if (Distance <= (bRanged ? RangedRange : AttackRange))
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
			if (bRanged)
			{
				FireShot(PlayerPawn);
			}
			// 예비 동작 동안 충분히 물러났으면 맞지 않는다.
			else if (Distance <= AttackRange + 60.f)
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

	// 맞는 순간 몸이 움찔 부푼다.
	HitPunch = 1.f;

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
	IsekaiAudio::Play(this, TEXT("SFX_EnemyDie"), 0.9f, 0.08f);
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
