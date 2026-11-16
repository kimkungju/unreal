#include "Audio/IsekaiAudio.h"

#include "Assets/IsekaiAssets.h"

// Content/Audio에 놓인 WAV를 어셋으로 가져온다.
// 사람이 Import 버튼을 누르지 않아도 Play를 누르면 소리가 나게 하려는 것이다.

namespace IsekaiAudio
{
	void ImportMissingSources()
	{
		if (IsekaiAssets::ImportMissingFiles(TEXT("Audio"), { TEXT("wav") }) > 0)
		{
			ResetCache();
		}
	}
}
