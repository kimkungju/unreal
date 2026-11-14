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
class UIsekaiSpellComponent;
struct FInputActionValue;

/**
 * 1인칭 플레이어 캐릭터.
 * 입력 어셋이 아직 없으므로 키 매핑을 코드에서 직접 만든다.
 * WASD 이동, 마우스 시점, Space 점프, Shift 달리기, E 상호작용, 마우스 왼쪽 공격 마법.
 */
UCLASS()
class ISEKAI_API AIsekaiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AIsekaiCharacter();

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiInteractionComponent* GetInteraction() const { return Interaction; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiSpellComponent* GetSpells() const { return Spells; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiDialogueComponent* GetDialogue() const { return Dialogue; }

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiHealthComponent* GetHealthComponent() const { return Health; }

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

private:
	void BuildDefaultInput();

	/** 쓰러지면 시작 지점으로 돌아가 체력을 회복한다. 임시 처리. */
	UFUNCTION()
	void HandleDied(AActor* Killer);

	FTransform RespawnTransform;

	bool IsInDialogue() const;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted();
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnInteract();
	void OnCastAttack();

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
};
