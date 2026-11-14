#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IsekaiTimeOfDay.generated.h"

class ADirectionalLight;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIsekaiHourChanged, int32, NewHour);

/**
 * 게임 안의 하루를 흘려 보내고 태양(디렉셔널 라이트)을 그에 맞춰 돌린다.
 * 6시에 해가 뜨고 12시에 머리 위, 18시에 진다.
 * 레벨에 없으면 게임 모드가 하나를 자동으로 만든다.
 */
UCLASS()
class ISEKAI_API AIsekaiTimeOfDay : public AActor
{
	GENERATED_BODY()

public:
	AIsekaiTimeOfDay();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Time")
	void SetTimeOfDay(float NewHours);

	UFUNCTION(BlueprintPure, Category = "Time")
	float GetTimeOfDay() const { return TimeOfDayHours; }

	/** "08:30" 형식 */
	UFUNCTION(BlueprintPure, Category = "Time")
	FString GetClockString() const;

	/** 정각이 바뀔 때마다 호출된다. NPC 일과나 조명 전환을 여기에 건다. */
	UPROPERTY(BlueprintAssignable, Category = "Time")
	FIsekaiHourChanged OnHourChanged;

	/** 비워 두면 레벨에서 첫 번째 디렉셔널 라이트를 찾아 쓴다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Time")
	TObjectPtr<ADirectionalLight> Sun;

	/** 게임 속 하루가 현실 시간으로 몇 분인지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0.1"))
	float DayLengthMinutes = 20.f;

	/** 시작 시각 (0~24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time", meta = (ClampMin = "0", ClampMax = "24"))
	float TimeOfDayHours = 8.f;

	/** 해가 뜨고 지는 방위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float SunYaw = -30.f;

	/** 대화나 컷신 중에 시간을 멈출 때 쓴다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	bool bPaused = false;

protected:
	virtual void BeginPlay() override;

private:
	void ApplySunRotation();
};
