#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiGravityAnomaly.generated.h"

class UBoxComponent;

/**
 * 중력이 약해지는 이상현상 지대.
 * 상자 안에 들어온 캐릭터는 높이, 느리게 뛴다. 나가면 원래대로 돌아온다.
 */
UCLASS(Blueprintable)
class ISEKAI_API AIsekaiGravityAnomaly : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiGravityAnomaly();

	UBoxComponent* GetZone() const { return Zone; }

	/** 중력 배율을 바꾸고, 지금 안에 있는 캐릭터에게 바로 적용한다. 음수면 위로 떠오른다. */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void SetGravityScaleInside(float NewScale);

	/** 지대 안에서의 중력 배율. 1이 보통, 0에 가까울수록 떠오른다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly", meta = (ClampMin = "-1", ClampMax = "1"))
	float GravityScaleInside = 0.25f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Zone;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyGravity(AActor* OtherActor, float Scale) const;
};
