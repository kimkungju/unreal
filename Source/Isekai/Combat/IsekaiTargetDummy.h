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
};
