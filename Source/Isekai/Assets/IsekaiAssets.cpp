#include "Assets/IsekaiAssets.h"

#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#endif

namespace IsekaiAssets
{
	int32 ImportMissingFiles(const TCHAR* Folder, const TArray<FString>& Extensions)
	{
#if WITH_EDITOR
		const FString SourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / Folder);

		TArray<FString> ToImport;
		for (const FString& Extension : Extensions)
		{
			TArray<FString> FileNames;
			IFileManager::Get().FindFiles(FileNames, *(SourceDir / FString::Printf(TEXT("*.%s"), *Extension)), true, false);

			for (const FString& FileName : FileNames)
			{
				const FString AssetName = FPaths::GetBaseFilename(FileName);
				const FString PackagePath = FString::Printf(TEXT("/Game/%s/%s"), Folder, *AssetName);
				const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

				const bool bInMemory = FindObject<UObject>(nullptr, *ObjectPath) != nullptr;
				const bool bOnDisk = FPackageName::DoesPackageExist(PackagePath);
				if (!bInMemory && !bOnDisk)
				{
					ToImport.Add(SourceDir / FileName);
				}
			}
		}

		if (ToImport.Num() == 0)
		{
			return 0;
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->Filenames = ToImport;
		ImportData->DestinationPath = FString::Printf(TEXT("/Game/%s"), Folder);
		ImportData->bReplaceExisting = false;

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		const TArray<UObject*> Imported = AssetTools.ImportAssetsAutomated(ImportData);

		UE_LOG(LogTemp, Display, TEXT("IsekaiAssets: imported %d of %d files from Content/%s."), Imported.Num(), ToImport.Num(), Folder);
		return Imported.Num();
#else
		return 0;
#endif
	}

	UTexture2D* FindTexture(const TCHAR* Folder, const TCHAR* Name)
	{
		const FString PackagePath = FString::Printf(TEXT("/Game/%s/%s"), Folder, Name);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, Name);

		if (UTexture2D* InMemory = FindObject<UTexture2D>(nullptr, *ObjectPath))
		{
			return InMemory;
		}

		// 없는 어셋을 불러오려 하면 경고가 쌓이므로 파일이 있는지부터 본다.
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}

		return LoadObject<UTexture2D>(nullptr, *ObjectPath);
	}
}
