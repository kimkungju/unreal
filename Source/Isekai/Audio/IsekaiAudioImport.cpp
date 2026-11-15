#include "Audio/IsekaiAudio.h"

// Content/Audio에 놓인 WAV를 어셋으로 가져오는 부분. 에디터에서만 쓴다.
// 사람이 Import 버튼을 누르지 않아도 Play를 누르면 소리가 나게 하려는 것이다.

#if WITH_EDITOR

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundBase.h"

namespace IsekaiAudio
{
	void ImportMissingSources()
	{
		const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Audio"));

		TArray<FString> FileNames;
		IFileManager::Get().FindFiles(FileNames, *(SourceDir / TEXT("*.wav")), true, false);

		TArray<FString> ToImport;
		for (const FString& FileName : FileNames)
		{
			const FString AssetName = FPaths::GetBaseFilename(FileName);
			const FString PackagePath = FString::Printf(TEXT("/Game/Audio/%s"), *AssetName);
			const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

			const bool bInMemory = FindObject<USoundBase>(nullptr, *ObjectPath) != nullptr;
			const bool bOnDisk = FPackageName::DoesPackageExist(PackagePath);
			if (!bInMemory && !bOnDisk)
			{
				ToImport.Add(SourceDir / FileName);
			}
		}

		if (ToImport.Num() == 0)
		{
			return;
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->Filenames = ToImport;
		ImportData->DestinationPath = TEXT("/Game/Audio");
		ImportData->bReplaceExisting = false;

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		const TArray<UObject*> Imported = AssetTools.ImportAssetsAutomated(ImportData);

		UE_LOG(LogTemp, Display, TEXT("IsekaiAudio: imported %d of %d sound files from Content/Audio."), Imported.Num(), ToImport.Num());

		ResetCache();
	}
}

#else

namespace IsekaiAudio
{
	void ImportMissingSources()
	{
	}
}

#endif // WITH_EDITOR
