#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IsekaiSpellComponent.generated.h"

class AIsekaiSpellProjectile;

/**
 * 마나와 마법 시전을 담당한다. 공격(마법탄)과 이동(순간 가속)이 있다.
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

	/** 움직이던 방향으로 몸을 날린다. 가만히 있었다면 앞으로. 마나가 부족하거나 쿨다운 중이면 false. */
	UFUNCTION(BlueprintCallable, Category = "Spell")
	bool CastDash();

	/** 마나가 충분하면 쓰고 true. */
	UFUNCTION(BlueprintCallable, Category = "Spell")
	bool TrySpendMana(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Spell")
	void RestoreMana() { Mana = MaxMana; }

	UFUNCTION(BlueprintPure, Category = "Spell")
	float GetMana() const { return Mana; }

	/** 시전에 성공했을 때 알린다. 화면 흔들림이나 연출이 여기에 건다. */
	FSimpleMulticastDelegate OnAttackCast;
	FSimpleMulticastDelegate OnDashCast;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float DashManaCost = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float DashCooldown = 1.2f;

	/** 수평으로 날아가는 속도 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	float DashSpeed = 1500.f;

	/** 살짝 떠오르는 속도 (cm/s). 땅에 붙어 있으면 마찰로 바로 멈춘다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell")
	float DashLift = 260.f;

	/** 빛 마법을 켤 때 드는 마나 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell", meta = (ClampMin = "0"))
	float LightManaCost = 5.f;

protected:
	virtual void BeginPlay() override;

private:
	float Mana = 0.f;
	float CooldownRemaining = 0.f;
	float DashCooldownRemaining = 0.f;
};
