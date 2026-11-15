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
		// 캐릭터마다 평소 중력이 다를 수 있으므로(플레이어는 더 무겁다) 그 값에 곱한다.
		const ACharacter* Defaults = Character->GetClass()->GetDefaultObject<ACharacter>();
		const float NormalGravity = Defaults ? Defaults->GetCharacterMovement()->GravityScale : 1.f;
		Character->GetCharacterMovement()->GravityScale = NormalGravity * Scale;
	}
}

void AIsekaiGravityAnomaly::SetGravityScaleInside(float NewScale)
{
	GravityScaleInside = NewScale;

	TArray<AActor*> Inside;
	Zone->GetOverlappingActors(Inside, ACharacter::StaticClass());
	for (AActor* Actor : Inside)
	{
		ApplyGravity(Actor, GravityScaleInside);
	}
}
