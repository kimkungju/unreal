#include "Audio/IsekaiAudio.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"

namespace IsekaiAudio
{
	namespace
	{
		TMap<FName, TWeakObjectPtr<USoundBase>> Loaded;
		TSet<FName> Missing;
	}

	void ResetCache()
	{
		Missing.Reset();
	}

	USoundBase* Find(const TCHAR* Name)
	{
		const FName Key(Name);

		if (const TWeakObjectPtr<USoundBase>* Cached = Loaded.Find(Key))
		{
			if (Cached->IsValid())
			{
				return Cached->Get();
			}
		}

		if (Missing.Contains(Key))
		{
			return nullptr;
		}

		const FString PackagePath = FString::Printf(TEXT("/Game/Audio/%s"), Name);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, Name);

		// 방금 가져와 아직 저장하지 않은 어셋은 메모리에만 있다. 그것부터 찾는다.
		USoundBase* Sound = FindObject<USoundBase>(nullptr, *ObjectPath);

		if (!Sound)
		{
			// 없는 어셋을 불러오려 하면 경고가 쌓이므로 파일이 있는지부터 본다.
			if (!FPackageName::DoesPackageExist(PackagePath))
			{
				Missing.Add(Key);
				return nullptr;
			}

			Sound = LoadObject<USoundBase>(nullptr, *ObjectPath);
		}

		if (!Sound)
		{
			Missing.Add(Key);
			return nullptr;
		}

		Loaded.Add(Key, Sound);
		return Sound;
	}

	void Play(const UObject* WorldContext, const TCHAR* Name, float Volume, float PitchJitter)
	{
		USoundBase* Sound = Find(Name);
		if (!Sound || !WorldContext)
		{
			return;
		}

		const float Pitch = 1.f + FMath::FRandRange(-PitchJitter, PitchJitter);
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
	}

	UAudioComponent* StartLoop(const UObject* WorldContext, const TCHAR* Name, float Volume, float FadeInSeconds)
	{
		USoundBase* Sound = Find(Name);
		if (!Sound || !WorldContext)
		{
			return nullptr;
		}

		// 가져온 직후의 사운드 웨이브는 한 번만 재생되게 되어 있다.
		if (USoundWave* Wave = Cast<USoundWave>(Sound))
		{
			Wave->bLooping = true;
		}

		UAudioComponent* Loop = UGameplayStatics::SpawnSound2D(WorldContext, Sound, Volume, 1.f, 0.f, nullptr, false, false);
		if (Loop && FadeInSeconds > 0.f)
		{
			Loop->FadeIn(FadeInSeconds, Volume);
		}
		return Loop;
	}

	void StopLoop(UAudioComponent* Loop, float FadeOutSeconds)
	{
		if (!Loop)
		{
			return;
		}

		if (FadeOutSeconds > 0.f)
		{
			Loop->FadeOut(FadeOutSeconds, 0.f);
		}
		else
		{
			Loop->Stop();
		}
	}
}
