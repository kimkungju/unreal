#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IsekaiCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UIsekaiInteractionComponent;
struct FInputActionValue;

/**
 * 1인칭 플레이어 캐릭터.
 * 입력 어셋이 아직 없으므로 키 매핑을 코드에서 직접 만든다.
 * WASD 이동, 마우스 시점, Space 점프, Shift 달리기, E 상호작용.
 */
UCLASS()
class ISEKAI_API AIsekaiCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AIsekaiCharacter();

	UFUNCTION(BlueprintPure, Category = "Isekai")
	UIsekaiInteractionComponent* GetInteraction() const { return Interaction; }

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

private:
	void BuildDefaultInput();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnSprintStarted();
	void OnSprintCompleted();
	void OnInteract();

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
};
