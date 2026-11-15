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
class UAudioComponent;

/**
 * 이야기 진행을 처음부터 끝까지 맡는다. 지금은 프롤로그와 1장이 들어 있다.
 *
 * 프롤로그(약 3분): 검은 화면의 독백 → 하늘에서 추락 → 폐허에 착지 → 마법사와 대화 →
 * 첫 마법 → 짐승 무리 → 큰 짐승 → 이상현상 폭주 → 대화 → 제목.
 *
 * 1장(약 3분): 마법사를 따라 걷기 → 고갯길 매복(원거리 적) → 숲길 → 야영지에서 선택 → 장 제목.
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

	/** 켜면 프롤로그를 통째로 건너뛰고 1장부터 시작한다. 반복 시험용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prologue")
	bool bSkipPrologue = false;

	/** 1장에서 마법사가 걷는 길. 폐허 앞에서 숲의 야영지까지, 순서대로. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chapter1")
	TArray<FVector> RouteToCamp;

	/** RouteToCamp에서 고갯길 꼭대기와 바닥에 해당하는 칸 번호. 그 사이에서 매복이 터진다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chapter1")
	int32 PassTopIndex = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chapter1")
	int32 PassBottomIndex = 3;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 지금 반복 재생 중인 배경 소리와 음악. 가비지 컬렉션에 쓸려 가지 않게 붙잡아 둔다. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbienceLoop;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicLoop;

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
		EscortToPass,
		Ambush,
		EscortToCamp,
		CampTurn,
		CampQuestion,
		CampReply,
		ChapterFade,
		ChapterTitle,
		ChapterEnd,
		Done
	};

	void Go(EStep NewStep);
	bool Acquire();
	void Begin();
	void StartFall();
	void StartCombat();
	void StartSurge();
	void Finish();

	void StartChapter1();
	void StartAmbush();
	void StartCamp();
	void FinishChapter1();

	/** 마법사를 길 따라 걷게 하고, 플레이어가 뒤처지면 기다리게 한다. 끝까지 가서 플레이어도 도착하면 true. */
	bool UpdateEscort(float RealDelta);
	void OnWaypointReached(int32 Index);
	void Bark(const TCHAR* Text, float Duration = 4.f);
	AIsekaiEnemy* SpawnEnemyAt(const FVector& Location, const FVector& LookAt);

	AIsekaiEnemy* SpawnEnemy(float AngleDegrees);
	int32 CountLivingEnemies() const;
	void LookAtMage(float RealDelta);
	void SetTimeScale(float Scale);

	/** 배경 소리를 바꾼다. 이름이 nullptr이면 끄기만 한다. */
	void SetAmbience(const TCHAR* Name, float Volume = 1.f, float FadeSeconds = 0.5f);
	void SetMusic(const TCHAR* Name, float Volume = 1.f, float FadeSeconds = 1.f);

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

	int32 RouteIndex = 0;
	float EscortWaitTime = 0.f;
	FText MageName;

	TWeakObjectPtr<AIsekaiCharacter> Player;
	TWeakObjectPtr<AIsekaiHUD> HUD;
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;
	TWeakObjectPtr<AIsekaiEnemy> FirstEnemy;
	TWeakObjectPtr<AIsekaiEnemy> Elite;
	TArray<TWeakObjectPtr<AIsekaiEnemy>> Enemies;
};
