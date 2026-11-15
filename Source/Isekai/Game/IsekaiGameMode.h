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

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/**
	 * 켜면 게임이 시작될 때마다 첫 구역의 그레이박스를 코드로 새로 만든다.
	 * 레벨을 손으로 저장해 둘 필요가 없어진다. 직접 꾸민 레벨로 넘어가면 끈다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bBuildGreyboxOnPlay = true;

	/** 레벨에 허수아비가 하나도 없으면 시작 지점 앞에 3개를 만든다. 시험용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bSpawnTestDummies = true;

protected:
	virtual void BeginPlay() override;

	/** 레벨에 NPC가 하나도 없으면 시작 지점 근처에 1명을 만든다. 시험용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bSpawnTestNPC = true;

	/** 켜면, 레벨에 적이 하나도 없을 때 허수아비 너머 멀리에 2마리를 만든다. 시험용. 이야기 진행이 적을 직접 내보내므로 평소에는 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bSpawnTestEnemies = false;

private:
	void SpawnTestDummies();
	void SpawnTestEnemies();
	void SpawnTestNPC();

	/** 시작 지점의 위치와 바라보는 방향. 시작 지점이 없으면 원점. */
	void GetStartTransform(FVector& OutLocation, FRotator& OutFacing) const;

	/** 주어진 지점 아래의 바닥 높이를 찾는다. 못 찾으면 원래 높이. */
	float FindGroundZ(const FVector& Location) const;
};
