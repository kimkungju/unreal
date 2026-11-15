#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace IsekaiGreybox
{
	/**
	 * 첫 구역의 그레이박스를 주어진 월드에 만든다. 전에 만든 것이 있으면 지우고 새로 만든다.
	 * 에디터 레벨에서도, 게임이 시작되는 순간에도 쓸 수 있다.
	 * 에디터가 포함되지 않은 빌드에서는 아무 일도 하지 않고 false를 돌려준다.
	 */
	ISEKAI_API bool Build(UWorld* World);
}
