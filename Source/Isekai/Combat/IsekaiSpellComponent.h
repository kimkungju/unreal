#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiSpellComponent.generated.h"

class AIsekaiSpellProjectile;

/**
 * 마나와 마법 시전을 담당한다. 지금은 공격 마법 1종만 있다.
 * 마법이 늘어나면 마법마다 데이터(소모량, 쿨다운, 발사체)를 따로 두는 구조로 바꾼다.
 */
UCLASS(ClassGroup = (Isekai), meta = (BlueprintSpawnableComponent))
class ISEKAI_API UIsekaiSpellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIsekaiSpellComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 시선 방향으로 공격 마법을 쏜다. 마나가 부족하거나 쿨다운 중이면 false. */
	UFUNCTION(BlueprintCallable, Category = "Spell")
	bool CastAttack();

	UFUNCTION(BlueprintPure, Category = "Spell")
	float GetMana() const { return Mana; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "1"))
	float MaxMana = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float ManaRegenPerSecond = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float AttackManaCost = 10.f;

	/** 연속 시전 사이의 최소 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float AttackCooldown = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	TSubclassOf<AIsekaiSpellProjectile> AttackProjectileClass;

protected:
	virtual void BeginPlay() override;

private:
	float Mana = 0.f;
	float CooldownRemaining = 0.f;
};
