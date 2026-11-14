#include "Dialogue/IsekaiExaminable.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiExaminable::AIsekaiExaminable()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	PromptText = FText::FromString(TEXT("살펴보기"));
}

FText AIsekaiExaminable::GetInteractionPrompt_Implementation() const
{
	return PromptText;
}

bool AIsekaiExaminable::CanInteract_Implementation(AActor* Interactor) const
{
	return Lines.Num() > 0;
}

void AIsekaiExaminable::Interact_Implementation(AActor* Interactor)
{
	UIsekaiDialogueComponent* Dialogue = Interactor ? Interactor->FindComponentByClass<UIsekaiDialogueComponent>() : nullptr;
	if (Dialogue && Dialogue->StartDialogue(Lines))
	{
		Dialogue->SetObjectiveOnEnd(ObjectiveAfter);
	}
}
