#include "World/IsekaiGravityAnomaly.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AIsekaiGravityAnomaly::AIsekaiGravityAnomaly()
{
	PrimaryActorTick.bCanEverTick = false;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	Zone->SetBoxExtent(FVector(1000.f, 1000.f, 1000.f));
	Zone->SetCollisionProfileName(TEXT("Trigger"));
	Zone->SetGenerateOverlapEvents(true);
	SetRootComponent(Zone);
}

void AIsekaiGravityAnomaly::BeginPlay()
{
	Super::BeginPlay();

	Zone->OnComponentBeginOverlap.AddDynamic(this, &AIsekaiGravityAnomaly::HandleBeginOverlap);
	Zone->OnComponentEndOverlap.AddDynamic(this, &AIsekaiGravityAnomaly::HandleEndOverlap);
}

void AIsekaiGravityAnomaly::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ApplyGravity(OtherActor, GravityScaleInside);
}

void AIsekaiGravityAnomaly::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ApplyGravity(OtherActor, 1.f);
}

void AIsekaiGravityAnomaly::ApplyGravity(AActor* OtherActor, float Scale) const
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		Character->GetCharacterMovement()->GravityScale = Scale;
	}
}
