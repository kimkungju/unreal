#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IsekaiEnemy.generated.h"

class UIsekaiHealthComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 네 발 짐승 형태의 기본 적. 플레이어가 가까이 오면 쫓아와서 공격한다.
 *
 * 몸은 기본 도형으로 짠 뼈대다: 몸통, 머리, 다리 넷, 꼬리. 변종에 따라 목이나 뿔이 붙는다.
 * 걷기, 공격 예비 동작, 피격 반응은 코드로 직접 움직인다. 모델과 애니메이션이 생기면
 * Rig 아래를 통째로 스켈레탈 메시로 바꾸면 된다.
 *
 * 공격 전에는 몸을 일으키고 눈에 붉은빛이 들어온다. 그 사이에 물러나면 맞지 않는다.
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

	/** 멀리서 탄을 쏘는 종류로 바꾼다. 목이 길고 몸이 가늘다. 만든 직후에 부른다. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void MakeRanged();

	/** 크기, 체력, 공격력을 한꺼번에 키운다. 뿔이 달린다. 만든 직후에 부른다. */
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

	/** 켜면 붙어서 때리는 대신 RangedRange 안에서 탄을 쏜다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	bool bRanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float RangedRange = 1400.f;

	/** 탄이 날아가는 속도 (cm/s). 느릴수록 피하기 쉽다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ShotSpeed = 900.f;

	/** 맞았을 때 뒤로 밀리는 세기 (cm/s). 0이면 밀리지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float KnockbackSpeed = 450.f;

protected:
	virtual void BeginPlay() override;

	/** 몸 전체의 뿌리. 이 아래가 기본 도형으로 짠 뼈대다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<USceneComponent> Rig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> Torso;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> Tail;

	/** 원거리 종류에만 보이는 긴 목 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> Neck;

	/** 큰 종류에만 보이는 뿔 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> HornLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UStaticMeshComponent> HornRight;

	/** 다리 넷의 엉덩이 관절. 앞왼쪽, 앞오른쪽, 뒤왼쪽, 뒤오른쪽 순서. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TArray<TObjectPtr<USceneComponent>> LegPivots;

	/** 공격 예비 동작 때 켜지는 눈빛 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rig")
	TObjectPtr<UPointLightComponent> EyeGlow;

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

	UStaticMeshComponent* AddPart(const TCHAR* PartName, UStaticMesh* Shape, USceneComponent* Parent,
		const FVector& Location, const FRotator& Rotation, const FVector& Scale);

	void SetState(EState NewState);
	void UpdateRig(float DeltaSeconds);
	void SpawnShards();
	void FireShot(const APawn* Target);

	EState State = EState::Idle;
	float StateTime = 0.f;

	// 뼈대를 움직이는 데 쓰는 값들
	float WalkPhase = 0.f;
	float LegSwing = 0.f;
	float PosePitch = 0.f;
	float PoseScale = 1.f;
	float HitPunch = 0.f;
};
