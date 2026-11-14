#include "Player/IsekaiCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/IsekaiInteractionComponent.h"

AIsekaiCharacter::AIsekaiCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	Interaction = CreateDefaultSubobject<UIsekaiInteractionComponent>(TEXT("Interaction"));
	Spells = CreateDefaultSubobject<UIsekaiSpellComponent>(TEXT("Spells"));
	Dialogue = CreateDefaultSubobject<UIsekaiDialogueComponent>(TEXT("Dialogue"));
}

void AIsekaiCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AIsekaiCharacter::BuildDefaultInput()
{
	if (MappingContext)
	{
		return;
	}

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	CastAttackAction = MakeAction(TEXT("IA_CastAttack"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// 이동 값은 X가 좌우, Y가 앞뒤다. 키 입력은 X로 들어오므로
	// 앞뒤 키는 축을 바꾸고(Swizzle), 반대 방향 키는 부호를 뒤집는다(Negate).
	auto MapMoveKey = [this](FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};

	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);

	// 마우스를 위로 올리면 위를 보도록 Y만 뒤집는다.
	FEnhancedActionKeyMapping& LookMapping = MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	UInputModifierNegate* LookNegate = NewObject<UInputModifierNegate>(MappingContext);
	LookNegate->bX = false;
	LookNegate->bY = true;
	LookNegate->bZ = false;
	LookMapping.Modifiers.Add(LookNegate);

	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(CastAttackAction, EKeys::LeftMouseButton);
}

void AIsekaiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildDefaultInput();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(Input, TEXT("Enhanced Input is disabled. Check Config/DefaultInput.ini.")))
	{
		return;
	}

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIsekaiCharacter::OnMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIsekaiCharacter::OnLook);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnJumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AIsekaiCharacter::OnSprintCompleted);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnInteract);
	Input->BindAction(CastAttackAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnCastAttack);
}

void AIsekaiCharacter::PawnClientRestart()
{
	// Super가 입력 컴포넌트를 만들고 SetupPlayerInputComponent를 부른다.
	Super::PawnClientRestart();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer || !MappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->RemoveMappingContext(MappingContext);
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

bool AIsekaiCharacter::IsInDialogue() const
{
	return Dialogue->IsActive();
}

void AIsekaiCharacter::OnJumpStarted()
{
	if (!IsInDialogue())
	{
		Jump();
	}
}

void AIsekaiCharacter::OnMove(const FInputActionValue& Value)
{
	// 대화 중에는 움직이거나 둘러보지 않는다.
	if (IsInDialogue())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void AIsekaiCharacter::OnLook(const FInputActionValue& Value)
{
	if (IsInDialogue())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AIsekaiCharacter::OnSprintStarted()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AIsekaiCharacter::OnSprintCompleted()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AIsekaiCharacter::OnInteract()
{
	// 대화 중의 E는 다음 대사로 넘기는 키다.
	if (IsInDialogue())
	{
		Dialogue->Advance();
		return;
	}

	Interaction->TryInteract();
}

void AIsekaiCharacter::OnCastAttack()
{
	if (!IsInDialogue())
	{
		Spells->CastAttack();
	}
}
