#include "World/IsekaiCampfire.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Player/IsekaiCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "World/IsekaiTimeOfDay.h"

AIsekaiCampfire::AIsekaiCampfire()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.3f));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	Fire = CreateDefaultSubobject<UPointLightComponent>(TEXT("Fire"));
	Fire->SetupAttachment(Mesh);
	Fire->SetUsingAbsoluteScale(true);
	Fire->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	Fire->SetIntensity(8000.f);
	Fire->SetAttenuationRadius(1500.f);
	Fire->SetLightColor(FLinearColor(1.f, 0.6f, 0.3f));
}

FText AIsekaiCampfire::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("모닥불에서 쉬기"));
}

bool AIsekaiCampfire::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}

void AIsekaiCampfire::Interact_Implementation(AActor* Interactor)
{
	AIsekaiCharacter* Character = Cast<AIsekaiCharacter>(Interactor);
	if (!Character)
	{
		return;
	}

	Character->Rest();

	bool bWasDay = true;
	for (TActorIterator<AIsekaiTimeOfDay> It(GetWorld()); It; ++It)
	{
		const float Hour = It->GetTimeOfDay();
		bWasDay = Hour >= 6.f && Hour < 18.f;
		It->SetTimeOfDay(bWasDay ? 20.f : 6.f);
		break;
	}

	FIsekaiDialogueLine Line;
	Line.Text = FText::FromString(bWasDay
		? TEXT("모닥불 곁에서 해가 질 때까지 쉬었다. 체력과 마나가 회복되었다.")
		: TEXT("모닥불 곁에서 밤을 보냈다. 체력과 마나가 회복되었다."));

	TArray<FIsekaiDialogueLine> RestLines;
	RestLines.Add(Line);
	Character->GetDialogue()->StartDialogue(RestLines);
}
