#include "Interaction/IsekaiInteractionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/IsekaiInteractable.h"

UIsekaiInteractionComponent::UIsekaiInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UIsekaiInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FocusedActor = nullptr;

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector TraceEnd = EyeLocation + EyeRotation.Vector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IsekaiInteraction), false, Owner);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, ECC_Visibility, Params))
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (HitActor && HitActor->Implements<UIsekaiInteractable>()
		&& IIsekaiInteractable::Execute_CanInteract(HitActor, Owner))
	{
		FocusedActor = HitActor;
	}
}

bool UIsekaiInteractionComponent::TryInteract()
{
	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return false;
	}

	IIsekaiInteractable::Execute_Interact(Target, GetOwner());
	return true;
}

FText UIsekaiInteractionComponent::GetFocusedPrompt() const
{
	const AActor* Target = FocusedActor.Get();
	return Target ? IIsekaiInteractable::Execute_GetInteractionPrompt(Target) : FText::GetEmpty();
}
