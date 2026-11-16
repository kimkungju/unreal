#include "World/IsekaiBrazier.h"

#include "Audio/IsekaiAudio.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/IsekaiCharacter.h"
#include "UI/IsekaiHUD.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiBrazier::AIsekaiBrazier()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	// 받침: 가는 기둥. 발이 원점에 닿는다.
	Stand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stand"));
	SetRootComponent(Stand);
	Stand->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.1f));
	Stand->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	if (CylinderMesh.Succeeded())
	{
		Stand->SetStaticMesh(CylinderMesh.Object);
	}

	// 그릇: 뒤집은 원뿔. 받침의 크기를 따라가지 않게 절대 크기로 둔다.
	Bowl = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bowl"));
	Bowl->SetupAttachment(Stand);
	Bowl->SetUsingAbsoluteScale(true);
	Bowl->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Bowl->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
	Bowl->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.4f));
	Bowl->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	if (ConeMesh.Succeeded())
	{
		Bowl->SetStaticMesh(ConeMesh.Object);
	}

	Flame = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Stand);
	Flame->SetUsingAbsoluteScale(true);
	Flame->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Flame->SetIntensity(9000.f);
	Flame->SetAttenuationRadius(1200.f);
	Flame->SetLightColor(FLinearColor(1.f, 0.6f, 0.3f));
	Flame->SetVisibility(false);
}

FText AIsekaiBrazier::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("불 옮기기"));
}

bool AIsekaiBrazier::CanInteract_Implementation(AActor* Interactor) const
{
	return !bLit;
}

void AIsekaiBrazier::Interact_Implementation(AActor* Interactor)
{
	const AIsekaiCharacter* Character = Cast<AIsekaiCharacter>(Interactor);
	if (!Character || bLit)
	{
		return;
	}

	if (!Character->IsLightOn())
	{
		// 왜 안 붙는지 알려 준다.
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (AIsekaiHUD* HUD = PC ? Cast<AIsekaiHUD>(PC->GetHUD()) : nullptr)
		{
			HUD->ShowBark(FText::GetEmpty(), FText::FromString(TEXT("손끝에 빛이 없다. 먼저 F로 빛을 띄우자.")), 3.f);
		}
		return;
	}

	bLit = true;
	Flame->SetVisibility(true);
	IsekaiAudio::Play(this, TEXT("SFX_Impact"), 0.6f, 0.1f);
}
