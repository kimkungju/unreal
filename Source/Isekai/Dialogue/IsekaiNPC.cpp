#include "Dialogue/IsekaiNPC.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiNPC::AIsekaiNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// 걸으려면 움직임을 처리해 줄 컨트롤러가 필요하다.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->bRunPhysicsWithNoController = true;

	// 캡슐의 기본 설정은 시선 판정(Visibility)을 통과시킨다. 바라봤을 때 잡히도록 막는다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
	Placeholder->SetupAttachment(GetCapsuleComponent());
	Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Placeholder->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.76f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Placeholder->SetStaticMesh(CylinderMesh.Object);
	}
}

void AIsekaiNPC::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AIsekaiNPC::WalkTo(const FVector& Destination)
{
	WalkDestination = Destination;
	bHasDestination = true;
}

void AIsekaiNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bHasDestination)
	{
		return;
	}

	const FVector ToDestination = WalkDestination - GetActorLocation();
	if (ToDestination.Size2D() < 120.0)
	{
		bHasDestination = false;
		return;
	}

	AddMovementInput(ToDestination.GetSafeNormal2D());
}

FText AIsekaiNPC::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("대화하기"));
}

bool AIsekaiNPC::CanInteract_Implementation(AActor* Interactor) const
{
	return Lines.Num() > 0;
}

void AIsekaiNPC::Interact_Implementation(AActor* Interactor)
{
	UIsekaiDialogueComponent* Dialogue = Interactor ? Interactor->FindComponentByClass<UIsekaiDialogueComponent>() : nullptr;
	if (!Dialogue)
	{
		return;
	}

	TArray<FIsekaiDialogueLine> Resolved = Lines;
	for (FIsekaiDialogueLine& Line : Resolved)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = DisplayName;
		}
	}

	if (Dialogue->StartDialogue(Resolved))
	{
		Dialogue->SetObjectiveOnEnd(ObjectiveAfterTalk);
	}
}
