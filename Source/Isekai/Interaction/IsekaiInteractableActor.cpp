#include "Interaction/IsekaiInteractableActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

AIsekaiInteractableActor::AIsekaiInteractableActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PromptText = FText::FromString(TEXT("Interact"));
}

FText AIsekaiInteractableActor::GetInteractionPrompt_Implementation() const
{
	return PromptText;
}

bool AIsekaiInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	return bInteractionEnabled;
}

void AIsekaiInteractableActor::Interact_Implementation(AActor* Interactor)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White,
			FString::Printf(TEXT("Interacted: %s"), *GetName()));
	}

	OnInteracted(Interactor);
}
