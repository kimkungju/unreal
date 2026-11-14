#include "Combat/IsekaiTargetDummy.h"

#include "Combat/IsekaiHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiTargetDummy::AIsekaiTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	Health = CreateDefaultSubobject<UIsekaiHealthComponent>(TEXT("Health"));
}

void AIsekaiTargetDummy::BeginPlay()
{
	Super::BeginPlay();

	Health->OnHealthChanged.AddDynamic(this, &AIsekaiTargetDummy::HandleHealthChanged);
	Health->OnDied.AddDynamic(this, &AIsekaiTargetDummy::HandleDied);
}

void AIsekaiTargetDummy::HandleHealthChanged(float NewHealth, float Delta)
{
	if (GEngine && Delta < 0.f)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 1.5f, FColor::Yellow,
			FString::Printf(TEXT("%s  HP %.0f / %.0f"), *GetName(), NewHealth, Health->MaxHealth));
	}
}

void AIsekaiTargetDummy::HandleDied(AActor* Killer)
{
	SetActive(false);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AIsekaiTargetDummy::Respawn, RespawnDelay, false);
}

void AIsekaiTargetDummy::Respawn()
{
	Health->ResetHealth();
	SetActive(true);
}

void AIsekaiTargetDummy::SetActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
}
