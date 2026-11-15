#pragma once

#include "CoreMinimal.h"

class UAudioComponent;
class USoundBase;

/**
 * 소리를 이름으로 재생하는 작은 도우미.
 *
 * 이름은 Content/Audio 아래 어셋의 이름이다. 예: "SFX_Cast" → /Game/Audio/SFX_Cast
 * 어셋이 없으면 조용히 아무 일도 하지 않는다. 그래서 소리 파일을 아직 가져오지 않았어도
 * 게임은 그대로 돌아가고, 같은 이름의 어셋을 넣으면 코드 수정 없이 소리가 난다.
 */
namespace IsekaiAudio
{
	/**
	 * Content/Audio의 WAV 중 아직 어셋이 아닌 것을 가져온다. 에디터에서만 동작한다.
	 * 가져온 어셋은 저장하기 전까지 메모리에만 있다. 에디터를 닫을 때 저장할지 묻는다.
	 */
	ISEKAI_API void ImportMissingSources();

	/** "없는 어셋" 기록을 지운다. 새로 가져온 어셋을 다음 실행에서 찾게 한다. */
	ISEKAI_API void ResetCache();

	ISEKAI_API USoundBase* Find(const TCHAR* Name);

	/** 한 번 재생한다. PitchJitter가 0.05면 높낮이가 매번 ±5% 달라져 반복이 덜 거슬린다. */
	ISEKAI_API void Play(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, float PitchJitter = 0.f);

	/** 반복 재생을 시작한다. 돌려받은 것을 UPROPERTY에 담아 두었다가 StopLoop에 넘긴다. */
	ISEKAI_API UAudioComponent* StartLoop(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.f, float FadeInSeconds = 0.5f);

	ISEKAI_API void StopLoop(UAudioComponent* Loop, float FadeOutSeconds = 0.5f);
}
