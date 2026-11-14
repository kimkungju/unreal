#include "World/IsekaiFloatingStone.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiFloatingStone::AIsekaiFloatingStone()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AIsekaiFloatingStone::BeginPlay()
{
	Super::BeginPlay();

	BaseLocation = GetActorLocation();

	// 돌마다 박자가 어긋나야 자연스럽다.
	Phase = FMath::FRandRange(0.f, 2.f * PI);
}

void AIsekaiFloatingStone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Time = GetWorld()->GetTimeSeconds();
	const float Offset = FMath::Sin(Time * 2.f * PI / BobPeriod + Phase) * BobHeight;

	SetActorLocation(BaseLocation + FVector(0.f, 0.f, Offset));
	AddActorLocalRotation(FRotator(0.f, SpinDegreesPerSecond * DeltaSeconds, 0.f));
}
