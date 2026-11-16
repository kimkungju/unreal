#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiTargetDummy.generated.h"

class UIsekaiHealthComponent;
class UStaticMeshComponent;

/**
 * 마법을 시험해 볼 허수아비. 체력이 0이 되면 사라졌다가 잠시 뒤 되살아난다.
 * 적 AI가 생기기 전까지 전투 수치를 맞추는 데 쓴다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiTargetDummy : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiTargetDummy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy", meta = (ClampMin = "0"))
	float RespawnDelay = 3.f;

	/** 끄면 한 번 깨진 뒤 다시 나타나지 않는다. 수련 과제에 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy")
	bool bRespawn = true;

	UFUNCTION(BlueprintPure, Category = "Dummy")
	bool IsBroken() const { return bBroken; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UIsekaiHealthComponent> Health;

private:
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float Delta);

	UFUNCTION()
	void HandleDied(AActor* Killer);

	void Respawn();
	void SetActive(bool bActive);

	FTimerHandle RespawnTimer;
	bool bBroken = false;
};
