#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IsekaiGameMode.generated.h"

UCLASS()
class ISEKAI_API AIsekaiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIsekaiGameMode();

	/** 레벨에 허수아비가 하나도 없으면 시작 지점 앞에 3개를 만든다. 시험용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bSpawnTestDummies = true;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnTestDummies();
};
