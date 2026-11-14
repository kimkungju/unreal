#include "Combat/IsekaiSpellComponent.h"

#include "Combat/IsekaiSpellProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

UIsekaiSpellComponent::UIsekaiSpellComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	AttackProjectileClass = AIsekaiSpellProjectile::StaticClass();
}

void UIsekaiSpellComponent::BeginPlay()
{
	Super::BeginPlay();
	Mana = MaxMana;
}

void UIsekaiSpellComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Mana = FMath::Min(MaxMana, Mana + ManaRegenPerSecond * DeltaTime);
	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
	DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaTime);
}

bool UIsekaiSpellComponent::CastAttack()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !AttackProjectileClass)
	{
		return false;
	}

	if (CooldownRemaining > 0.f || Mana < AttackManaCost)
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// 카메라 바로 앞에서 생기면 화면을 가리므로 조금 앞에서 시작한다.
	const FVector SpawnLocation = EyeLocation + EyeRotation.Vector() * 60.f;

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!World->SpawnActor<AIsekaiSpellProjectile>(AttackProjectileClass, SpawnLocation, EyeRotation, Params))
	{
		return false;
	}

	Mana -= AttackManaCost;
	CooldownRemaining = AttackCooldown;
	OnAttackCast.Broadcast();
	return true;
}

bool UIsekaiSpellComponent::TrySpendMana(float Amount)
{
	if (Mana < Amount)
	{
		return false;
	}

	Mana -= Amount;
	return true;
}

bool UIsekaiSpellComponent::CastDash()
{
	ACharacter* Caster = Cast<ACharacter>(GetOwner());
	if (!Caster || DashCooldownRemaining > 0.f || Mana < DashManaCost)
	{
		return false;
	}

	FVector Direction = Caster->GetLastMovementInputVector();
	Direction.Z = 0.0;
	if (!Direction.Normalize())
	{
		Direction = Caster->GetActorForwardVector();
	}

	Caster->LaunchCharacter(Direction * DashSpeed + FVector(0.f, 0.f, DashLift), true, true);

	Mana -= DashManaCost;
	DashCooldownRemaining = DashCooldown;
	OnDashCast.Broadcast();
	return true;
}
