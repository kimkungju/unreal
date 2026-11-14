#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIsekaiHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIsekaiDied, AActor*, Killer);

/**
 * 체력을 가진 모든 대상(적, 허수아비, 나중에는 플레이어)에 붙인다.
 * 엔진의 기본 데미지 이벤트를 받아 체력을 깎고, 0이 되면 OnDied를 알린다.
 */
UCLASS(ClassGroup = (Isekai), meta = (BlueprintSpawnableComponent))
class ISEKAI_API UIsekaiHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsekaiHealthComponent();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return Health <= 0.f; }

	/** 체력을 최대치로 되돌린다. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FIsekaiHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FIsekaiDied OnDied;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	float Health = 0.f;
};
