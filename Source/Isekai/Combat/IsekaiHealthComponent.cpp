#include "Combat/IsekaiHealthComponent.h"

#include "GameFramework/Actor.h"

UIsekaiHealthComponent::UIsekaiHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIsekaiHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UIsekaiHealthComponent::HandleAnyDamage);
	}
}

void UIsekaiHealthComponent::ResetHealth()
{
	const float Delta = MaxHealth - Health;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, Delta);
}

void UIsekaiHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || IsDead())
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Max(0.f, Health - Damage);
	OnHealthChanged.Broadcast(Health, Health - OldHealth);

	if (IsDead())
	{
		OnDied.Broadcast(DamageCauser);
	}
}
