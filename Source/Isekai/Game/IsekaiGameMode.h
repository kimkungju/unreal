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

protected:
	virtual void BeginPlay() override;
};
