#include "Dialogue/IsekaiObjectiveComponent.h"

UIsekaiObjectiveComponent::UIsekaiObjectiveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitialObjective = FText::FromString(TEXT("가까이 있는 사람에게 말을 걸자"));
}

void UIsekaiObjectiveComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentObjective = InitialObjective;
}
