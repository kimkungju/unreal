#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiStoryDirector.generated.h"

class AIsekaiBrazier;
class AIsekaiCharacter;
class AIsekaiEnemy;
class AIsekaiExaminable;
class AIsekaiHUD;
class AIsekaiNPC;
class AIsekaiTargetDummy;
class AIsekaiTimeOfDay;
class UAudioComponent;
struct FIsekaiDialogueLine;

/** 시험할 때 어디서부터 시작할지 */
UENUM(BlueprintType)
enum class EIsekaiStoryStart : uint8
{
	Opening			UMETA(DisplayName = "처음 (그림 컷)"),
	Wake			UMETA(DisplayName = "집에서 깨어남"),
	Lesson1			UMETA(DisplayName = "수련 1 · 마법탄"),
	Lesson2			UMETA(DisplayName = "수련 2 · 가속"),
	Lesson3			UMETA(DisplayName = "수련 3 · 빛"),
	VillageAttack	UMETA(DisplayName = "짐승의 습격"),
	Departure		UMETA(DisplayName = "떠나는 날"),
};

/**
 * 이야기 진행을 처음부터 끝까지 맡는다.
 *
 * 1. 그림 컷 4장: 비 오는 밤의 사고 → 낯선 하늘에서 추락 → 폐허에서 세라가 발견
 * 2. 세라의 집에서 깨어남. 출신을 묻는 선택. 마력이 새고 있어 다루는 법을 배워야 한다
 * 3. 마당에서 수련 세 번. 배울 때마다 기술이 하나씩 열리고 몸이 자란다
 *      마법탄(수정 깨기) → 보름 뒤 · 가속(연습 상대의 공격 피하기) → 한 달 뒤 밤 · 빛(화로에 불 옮기기)
 * 4. 모닥불 곁의 대화 중에 짐승이 집을 덮친다
 * 5. 습격 뒤: 강해진 마력이 짐승을 부른다는 것이 드러난다. 떠나는 이유를 고르는 선택
 * 6. 그림 컷 1장: 새벽의 출발. 제목
 * 7. 세라와 함께 동쪽 폐허까지. 고갯길 매복. 제단에서 북쪽을 가리키는 문양 → 끝
 *
 * 떠나는 이유는 세 겹이다.
 *   밀어내는 힘: 수련으로 커진 마력이 짐승을 부른다. 남으면 마을이 다친다.
 *   끌어당기는 힘: 폐허의 제단이 북쪽을 가리킨다. 틈을 닫는 법과 돌아갈 길의 단서가 거기 있다.
 *   함께 가는 이유: 세라는 원래 틈을 재는 측량사라 목적이 같다.
 *
 * 레벨 생성 코드가 이 액터를 놓고 위치와 인물을 채워 준다. 대사는 모두 초안이다.
 */
UCLASS()
class ISEKAI_API AIsekaiStoryDirector : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiStoryDirector();

	virtual void Tick(float DeltaSeconds) override;

	/** 시험할 때 중간부터 시작한다. 평소에는 처음부터. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	EIsekaiStoryStart StartAt = EIsekaiStoryStart::Opening;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Story")
	TObjectPtr<AIsekaiNPC> Sera;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Story")
	TObjectPtr<AIsekaiNPC> Elder;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Story")
	TObjectPtr<AIsekaiExaminable> Altar;

	// 아래 위치는 모두 바닥 높이의 점이다. 인물은 그 위에 세운다.

	/** 깨어날 때 플레이어가 서 있는 자리와 처음 바라보는 곳 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector WakeSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector SeraBedside = FVector::ZeroVector;

	/** 집 문 바로 바깥 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector HouseDoor = FVector::ZeroVector;

	/** 수련하는 마당의 가운데 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector YardCenter = FVector::ZeroVector;

	/** 마당의 모닥불 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector YardFire = FVector::ZeroVector;

	/** 마을 쪽 방향 (마당에서 본) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector TowardVillage = FVector(1.f, 0.f, 0.f);

	/** 떠날 때 출발하는 마을 문 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	FVector VillageGate = FVector::ZeroVector;

	/** 마을 문에서 동쪽 폐허까지 세라가 걷는 길 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	TArray<FVector> Route;

	/** Route에서 고갯길 아래와 위에 해당하는 칸 번호. 그 사이에서 매복이 터진다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	int32 PassBottomIndex = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story|Places")
	int32 PassTopIndex = 6;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> AmbienceLoop;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicLoop;

private:
	enum class EStep : uint8
	{
		Init,

		// 공용
		Talk,
		Transition,
		Wait,
		Cutscene,

		// 집
		Opening,
		Wake,
		WakeReply,
		LeaveRoom,
		GoToYard,
		Lesson1Intro,
		Lesson1,
		Lesson1Done,
		Skip1,
		Lesson2Intro,
		Lesson2Dodge,
		Lesson2Defeat,
		Lesson2Done,
		Skip2,
		Lesson3Intro,
		Lesson3,
		Lesson3Done,

		// 습격과 결심
		AttackStart,
		AttackElite,
		Aftermath,
		AftermathReply,

		// 출발
		Departure,
		DepartureTitle,
		Escort,
		Ambush,
		AltarWait,
		Ending,
		EndTitle,
		Done
	};

	/** 그림 컷 한 장 */
	struct FShot
	{
		FString Image;
		FString Placeholder;
		TArray<FString> Lines;
		FVector2D PanFrom = FVector2D::ZeroVector;
		FVector2D PanTo = FVector2D::ZeroVector;
		float ZoomFrom = 1.05f;
		float ZoomTo = 1.15f;
	};

	// ---- 진행 ----
	bool Acquire();
	void Begin();
	void Go(EStep NewStep);
	void Enter(EStep NewStep);

	/** 대화를 띄우고, 끝나면 Next로 넘어간다. 대화 중에는 LookAt을 바라본다. */
	void Talk(const TArray<FIsekaiDialogueLine>& Lines, AActor* LookAt, EStep Next);

	/** 화면을 검게 덮었다가 Caption을 보여 주고, 시각을 바꾼 뒤 Next로 넘어가며 걷어 낸다. */
	void Transition(const TCHAR* Caption, float Hour, EStep Next);

	/** Seconds 뒤에 Next로 넘어간다. */
	void WaitThen(float Seconds, EStep Next);

	/** 그림 컷을 차례로 보여 주고 끝나면 Next로 넘어간다. */
	void PlayCutscene(const TArray<FShot>& InShots, int32 InCutsceneId, EStep Next);
	void StartShot(int32 Index);
	void OnCutsceneLine(int32 Shot, int32 Line);
	void TickCutscene(float RealDelta);

	// ---- 도우미 ----
	void SetFreeControl(bool bFree);
	void PlacePlayer(const FVector& Floor, const FVector& LookAt);
	void PlaceNPC(AIsekaiNPC* NPC, const FVector& Floor, const FVector& LookAt);
	void LookAt(AActor* Target, float RealDelta);
	void SetObjective(const TCHAR* Text);
	void Bark(const TCHAR* Speaker, const TCHAR* Text, float Duration = 4.5f);
	void Toast(const TCHAR* Header, const TCHAR* Body);
	void SetHour(float Hour, bool bFlow);
	void SetAmbience(const TCHAR* Name, float Volume = 1.f, float FadeSeconds = 0.5f);
	void SetMusic(const TCHAR* Name, float Volume = 1.f, float FadeSeconds = 1.f);

	AIsekaiEnemy* SpawnEnemy(const FVector& Floor, const FVector& LookAtPoint);
	int32 CountLiving() const;
	void HandleGolemAttack(bool bHit);

	bool UpdateEscort(float RealDelta);
	void OnWaypointReached(int32 Index);
	void StartAmbush();

	// ---- 상태 ----
	EStep Step = EStep::Init;
	float StepTime = 0.f;
	bool bFlagA = false;

	EStep NextStep = EStep::Done;
	TWeakObjectPtr<AActor> TalkLookAt;

	FString TransitionCaption;
	float TransitionHour = -1.f;
	float WaitSeconds = 0.f;

	TArray<FShot> Shots;
	int32 CutsceneId = 0;
	int32 ShotIndex = 0;
	int32 LineIndex = 0;
	float LineTime = 0.f;
	bool bLineShown = false;

	int32 DodgeCount = 0;
	int32 RouteIndex = 0;
	float EscortWaitTime = 0.f;
	bool bAmbushDone = false;

	TWeakObjectPtr<AIsekaiCharacter> Player;
	TWeakObjectPtr<AIsekaiHUD> HUD;
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;
	TWeakObjectPtr<AIsekaiEnemy> Golem;
	TWeakObjectPtr<AIsekaiEnemy> Elite;
	TArray<TWeakObjectPtr<AIsekaiEnemy>> Enemies;
	TArray<TWeakObjectPtr<AIsekaiTargetDummy>> Crystals;
	TArray<TWeakObjectPtr<AIsekaiBrazier>> Braziers;
};
