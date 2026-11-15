#include "Combat/IsekaiSpellProjectile.h"

#include "Audio/IsekaiAudio.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiSpellProjectile::AIsekaiSpellProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 4.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.2f));
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetIntensity(3000.f);
	Glow->SetAttenuationRadius(400.f);
	Glow->SetLightColor(FLinearColor(0.4f, 0.7f, 1.f));
	Glow->SetCastShadows(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2500.f;
	Movement->MaxSpeed = 2500.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
}

void AIsekaiSpellProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 쏜 사람 자신에게는 맞지 않게 한다.
	if (APawn* Caster = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Caster, true);
	}

	Collision->OnComponentHit.AddDynamic(this, &AIsekaiSpellProjectile::HandleHit);
}

void AIsekaiSpellProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this)
	{
		UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);
	}

	IsekaiAudio::Play(this, TEXT("SFX_Impact"), 0.8f, 0.08f);

	// 바로 사라지지 않고, 멈춘 자리에서 한순간 크게 번쩍인 뒤 사라진다.
	Movement->StopMovementImmediately();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false);
	Glow->SetIntensity(40000.f);
	Glow->SetAttenuationRadius(900.f);
	SetLifeSpan(0.12f);
}
