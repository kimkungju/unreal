#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * Content 아래 폴더에 놓인 원본 파일(그림, 소리)을 다루는 도우미.
 */
namespace IsekaiAssets
{
	/**
	 * Content/<Folder>에 있는 원본 파일 중 아직 어셋이 아닌 것을 가져온다. 에디터에서만 동작한다.
	 * Extensions 예: { TEXT("png"), TEXT("jpg") }
	 * 가져온 개수를 돌려준다.
	 */
	ISEKAI_API int32 ImportMissingFiles(const TCHAR* Folder, const TArray<FString>& Extensions);

	/** /Game/<Folder>/<Name> 텍스처를 찾는다. 없으면 nullptr. 방금 가져와 저장하지 않은 것도 찾는다. */
	ISEKAI_API UTexture2D* FindTexture(const TCHAR* Folder, const TCHAR* Name);
}
