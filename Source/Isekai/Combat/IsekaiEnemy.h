#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IsekaiEnemy.generated.h"

class UIsekaiHealthComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 가장 기본적인 적. 플레이어가 가까이 오면 쫓아와서 근접 공격을 한다.
 * 공격 전에 잠깐 몸을 부풀리는 예비 동작이 있어 플레이어가 물러날 틈을 준다.
 * 맞으면 뒤로 밀리고, 쓰러지면 조각으로 흩어진다.
 *
 * 지금은 플레이어를 향해 직선으로만 걷는다. 장애물을 돌아가는 길찾기는
 * 실제 레벨이 생긴 뒤 내비게이션 메시와 함께 넣는다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AIsekaiEnemy();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDefeated() const { return State == EState::Dead; }

	/** 공격 예비 동작 중인지. 연출이 이 순간을 잡아 쓴다. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsWindingUp() const { return State == EState::Windup; }

	/** 감지 거리와 상관없이 바로 플레이어를 쫓게 한다. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void StartChasing();

	/** 크기, 체력, 공격력을 한꺼번에 키운다. 만든 직후에 부른다. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void MakeElite();

	/** 이 거리 안에 플레이어가 들어오면 쫓기 시작한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float DetectRange = 1500.f;

	/** 쫓다가 이 거리보다 멀어지면 포기한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float LoseRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackRange = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackDamage = 15.f;

	/** 공격 예비 동작 시간. 길수록 피하기 쉽다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float WindupTime = 0.6f;

	/** 공격 뒤에 멈춰 있는 시간. 길수록 반격하기 쉽다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float RecoverTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MoveSpeed = 380.f;

	/** 맞았을 때 뒤로 밀리는 세기 (cm/s). 0이면 밀리지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float KnockbackSpeed = 450.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Placeholder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiHealthComponent> Health;

	/** 쓰러질 때 흩어지는 조각의 모양 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> ShardMesh;

private:
	enum class EState : uint8
	{
		Idle,
		Chase,
		Windup,
		Recover,
		Dead
	};

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float Delta);

	UFUNCTION()
	void HandleDied(AActor* Killer);

	void SetState(EState NewState);
	void SpawnShards();

	EState State = EState::Idle;
	float StateTime = 0.f;
	FVector PlaceholderBaseScale = FVector::OneVector;
};
