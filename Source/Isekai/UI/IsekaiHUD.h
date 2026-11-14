#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "IsekaiHUD.generated.h"

class AIsekaiTimeOfDay;

/**
 * 프로토타입용 임시 화면 표시. 조준점, 상호작용 안내, 시계를 그린다.
 * 엔진 기본 글꼴을 쓰므로 한글은 표시되지 않는다. 정식 UI는 UMG로 교체한다.
 */
UCLASS()
class ISEKAI_API AIsekaiHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	TWeakObjectPtr<AIsekaiTimeOfDay> TimeOfDay;
};
