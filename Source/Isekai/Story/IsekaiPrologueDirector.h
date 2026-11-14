#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiPrologueDirector.generated.h"

class AIsekaiCharacter;
class AIsekaiEnemy;
class AIsekaiGravityAnomaly;
class AIsekaiHUD;
class AIsekaiNPC;
class AIsekaiTimeOfDay;

/**
 * 프롤로그(약 3분)의 진행을 처음부터 끝까지 맡는다.
 *
 * 검은 화면의 독백 → 하늘에서 추락 → 폐허에 착지 → 마법사와 대화 →
 * 첫 마법 → 짐승 무리 → 큰 짐승 → 이상현상 폭주 → 대화 → 제목.
 *
 * 레벨에 하나 놓으면 게임 시작과 함께 진행된다. 없으면 평소처럼 자유롭게 돌아다닌다.
 */
UCLASS()
class ISEKAI_API AIsekaiPrologueDirector : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiPrologueDirector();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Prologue")
	TObjectPtr<AIsekaiNPC> Mage;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Prologue")
	TObjectPtr<AIsekaiGravityAnomaly> Anomaly;

	/** 폐허 단상 윗면의 가운데 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prologue")
	FVector DaisCenter = FVector::ZeroVector;

	/** 추락을 시작하는 높이 (단상 위로, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prologue")
	float FallHeight = 26000.f;

	/** 끄면 독백을 건너뛰고 바로 추락부터 시작한다. 반복 시험용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prologue")
	bool bPlayOpeningNarration = true;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EStep : uint8
	{
		Init,
		Narration,
		Fall,
		Land,
		TurnToMage,
		DialogueA,
		TeachAttackApproach,
		TeachAttackSlow,
		Wave,
		EliteApproach,
		TeachDashSlow,
		EliteFight,
		Surge,
		SurgeRecover,
		DialogueB,
		TitleIn,
		TitleHold,
		TitleOut,
		Done
	};

	void Go(EStep NewStep);
	bool Acquire();
	void Begin();
	void StartFall();
	void StartCombat();
	void StartSurge();
	void Finish();

	AIsekaiEnemy* SpawnEnemy(float AngleDegrees);
	int32 CountLivingEnemies() const;
	void LookAtMage(float RealDelta);
	void SetTimeScale(float Scale);

	void HandleAttackCast();
	void HandleDashCast();

	EStep Step = EStep::Init;

	/** 지금 단계에 들어온 뒤 흐른 실제 시간 (초). 느린 화면의 영향을 받지 않는다. */
	float StepTime = 0.f;

	/** 단계마다 "한 번만 할 일"을 표시하는 깃발. 단계가 바뀌면 꺼진다. */
	bool bFlagA = false;
	bool bFlagB = false;

	int32 NarrationIndex = 0;
	int32 WaveSpawned = 0;
	bool bAttackCastSeen = false;
	bool bDashCastSeen = false;

	TWeakObjectPtr<AIsekaiCharacter> Player;
	TWeakObjectPtr<AIsekaiHUD> HUD;
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;
	TWeakObjectPtr<AIsekaiEnemy> FirstEnemy;
	TWeakObjectPtr<AIsekaiEnemy> Elite;
	TArray<TWeakObjectPtr<AIsekaiEnemy>> Enemies;
};
