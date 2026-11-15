#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IsekaiCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UIsekaiDialogueComponent;
class UIsekaiHealthComponent;
class UIsekaiInteractionComponent;
class UIsekaiObjectiveComponent;
class UPointLightComponent;
class UIsekaiSpellComponent;
struct FInputActionValue;

/**
 * 1인칭 플레이어 캐릭터.
 * 입력 어셋이 아직 없으므로 키 매핑을 코드에서 직접 만든다.
 * WASD 이동, 마우스 시점, Space 점프, Shift 달리기, E 상호작용,
 * 마우스 왼쪽 공격 마법, 마우스 오른쪽 순간 가속, F 빛.
 */
UCLASS()
class ISEKAI_API AIsekaiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AIsekaiCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiInteractionComponent* GetInteraction() const { return Interaction; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiSpellComponent* GetSpells() const { return Spells; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiDialogueComponent* GetDialogue() const { return Dialogue; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiHealthComponent* GetHealthComponent() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiObjectiveComponent* GetObjectives() const { return Objectives; }

	/** 체력과 마나를 채우고, 쓰러졌을 때 돌아올 자리를 지금 위치로 바꾼다. */
	UFUNCTION(BlueprintCallable, Category = "Isekai")
	void Rest();

	/** 이야기에서 일어난 일을 기록해 두는 표시. 예: 출신을 사실대로 말했는지. */
	UFUNCTION(BlueprintCallable, Category = "Isekai")
	void SetStoryFlag(FName Flag) { StoryFlags.Add(Flag); }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	bool HasStoryFlag(FName Flag) const { return StoryFlags.Contains(Flag); }

	/** 화면을 흔든다. 1이 강한 충격. 이미 더 세게 흔들리고 있으면 무시된다. */
	UFUNCTION(BlueprintCallable, Category = "Isekai")
	void AddCameraShake(float Strength);

	/** 시야각을 부드럽게 바꾼다. 떨어질 때 넓히는 식으로 쓴다. */
	UFUNCTION(BlueprintCallable, Category = "Isekai")
	void SetTargetFOV(float NewFOV) { TargetFOV = NewFOV; }

	/** 연출 중에 플레이어의 조작을 막는 스위치들 */
	UPROPERTY(BlueprintReadWrite, Category = "Isekai")
	bool bMoveLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "Isekai")
	bool bLookLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "Isekai")
	bool bCanCastAttack = true;

	UPROPERTY(BlueprintReadWrite, Category = "Isekai")
	bool bCanCastDash = true;

	/** 마지막으로 맞은 시각 (월드 시간, 초). 맞은 적이 없으면 아주 작은 값. */
	float GetLastDamageTime() const { return LastDamageTime; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 550.f;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiInteractionComponent> Interaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiSpellComponent> Spells;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiDialogueComponent> Dialogue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiObjectiveComponent> Objectives;

	/** 빛 마법. 평소에는 꺼져 있다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> SpellLight;

private:
	void BuildDefaultInput();

	/** 쓰러지면 시작 지점으로 돌아가 체력을 회복한다. 임시 처리. */
	UFUNCTION()
	void HandleDied(AActor* Killer);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float Delta);

	FTransform RespawnTransform;
	float LastDamageTime = -1000.f;

	void HandleAttackCast();

	FVector CameraBaseLocation = FVector::ZeroVector;
	float ShakeStrength = 0.f;
	float TargetFOV = 90.f;

	bool IsInDialogue() const;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnInteract();
	void OnCastAttack();
	void OnCastDash();
	void OnToggleLight();
	void OnChoice1();
	void OnChoice2();
	void OnChoice3();

	TSet<FName> StoryFlags;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CastAttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CastDashAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleLightAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Choice1Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Choice2Action;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> Choice3Action;
};
