#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiFloatingStone.generated.h"

class UStaticMeshComponent;

/**
 * 이상현상 지대에 떠 있는 돌. 천천히 오르내리며 돈다.
 * 밟고 올라설 수 있다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiFloatingStone : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiFloatingStone();

	virtual void Tick(float DeltaSeconds) override;

	/** 위아래로 움직이는 폭 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	float BobHeight = 60.f;

	/** 한 번 오르내리는 데 걸리는 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating", meta = (ClampMin = "0.1"))
	float BobPeriod = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating")
	float SpinDegreesPerSecond = 6.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	FVector BaseLocation = FVector::ZeroVector;
	float Phase = 0.f;
};
