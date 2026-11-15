#include "Player/IsekaiCharacter.h"

#include "Audio/IsekaiAudio.h"

#include "Camera/CameraComponent.h"
#include "Combat/IsekaiHealthComponent.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Dialogue/IsekaiObjectiveComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/IsekaiInteractionComponent.h"
#include "Misc/App.h"
#include "UObject/ConstructorHelpers.h"

AIsekaiCharacter::AIsekaiCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 점프: 기본값보다 무겁게 떨어지고 그만큼 세게 뛴다. 높이는 비슷하고 떠 있는 시간이 짧다.
	// (엔진 기본값은 약 0.86초, 이 값은 약 0.74초. 뛰는 높이는 약 115cm.)
	GetCharacterMovement()->GravityScale = 1.7f;
	GetCharacterMovement()->JumpZVelocity = 620.f;
	GetCharacterMovement()->AirControl = 0.45f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	// 1인칭과 3인칭이 같은 카메라를 쓴다. 팔이 마우스를 따라 돌고 카메라는 그 끝에 매달린다.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 3인칭에서 보이는 임시 몸. 1인칭에서는 숨긴다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -24.f));
	BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.38f, 1.28f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetVisibility(false);
	if (BodyShape.Succeeded())
	{
		BodyMesh->SetStaticMesh(BodyShape.Object);
	}

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 58.f));
	HeadMesh->SetRelativeScale3D(FVector(0.36f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetVisibility(false);
	if (HeadShape.Succeeded())
	{
		HeadMesh->SetStaticMesh(HeadShape.Object);
	}

	Interaction = CreateDefaultSubobject<UIsekaiInteractionComponent>(TEXT("Interaction"));
	Spells = CreateDefaultSubobject<UIsekaiSpellComponent>(TEXT("Spells"));
	Dialogue = CreateDefaultSubobject<UIsekaiDialogueComponent>(TEXT("Dialogue"));
	Health = CreateDefaultSubobject<UIsekaiHealthComponent>(TEXT("Health"));
	Objectives = CreateDefaultSubobject<UIsekaiObjectiveComponent>(TEXT("Objectives"));

	SpellLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SpellLight"));
	SpellLight->SetupAttachment(GetCapsuleComponent());
	SpellLight->SetRelativeLocation(FVector(40.f, 25.f, 50.f));
	SpellLight->SetIntensity(5000.f);
	SpellLight->SetAttenuationRadius(1800.f);
	SpellLight->SetLightColor(FLinearColor(0.75f, 0.85f, 1.f));
	SpellLight->SetCastShadows(false);
	SpellLight->SetVisibility(false);
}

void AIsekaiCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	RespawnTransform = GetActorTransform();
	Health->OnDied.AddDynamic(this, &AIsekaiCharacter::HandleDied);
	Health->OnHealthChanged.AddDynamic(this, &AIsekaiCharacter::HandleHealthChanged);

	CameraBaseLocation = Camera->GetRelativeLocation();
	TargetFOV = Camera->FieldOfView;
	Spells->OnAttackCast.AddUObject(this, &AIsekaiCharacter::HandleAttackCast);
}

void AIsekaiCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 느린 화면(시간 배율) 중에도 흔들림과 시야각은 실제 시간으로 움직인다.
	const float RealDelta = FApp::GetDeltaTime();

	if (ShakeStrength > 0.01f)
	{
		const FVector Jitter(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f));
		Camera->SetRelativeLocation(CameraBaseLocation + Jitter * ShakeStrength * 7.f);
		ShakeStrength = FMath::FInterpTo(ShakeStrength, 0.f, RealDelta, 6.f);
	}
	else if (ShakeStrength > 0.f)
	{
		ShakeStrength = 0.f;
		Camera->SetRelativeLocation(CameraBaseLocation);
	}

	if (!FMath::IsNearlyEqual(Camera->FieldOfView, TargetFOV, 0.05f))
	{
		Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, TargetFOV, RealDelta, 3.f));
	}

	// 시점 전환: 팔 길이를 부드럽게 늘이고 줄인다.
	const float WantedArm = bThirdPerson ? ThirdPersonDistance : 0.f;
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, WantedArm, RealDelta, 9.f);

	// 카메라가 몸 안에 있을 때 몸이 보이면 화면을 가린다. 충분히 멀어진 뒤에 보여 준다.
	const bool bShowBody = CameraBoom->TargetArmLength > 110.f;
	if (bShowBody != bBodyShown)
	{
		bBodyShown = bShowBody;
		BodyMesh->SetVisibility(bShowBody);
		HeadMesh->SetVisibility(bShowBody);
	}

	// 착지 직전에 누른 점프를 기억했다가 뛸 수 있게 되는 순간 뛴다.
	if (JumpBufferLeft > 0.f)
	{
		JumpBufferLeft -= RealDelta;
		if (CanJump() && !IsInDialogue() && !bMoveLocked)
		{
			Jump();
			JumpBufferLeft = 0.f;
		}
	}
}

void AIsekaiCharacter::SetThirdPerson(bool bInThirdPerson)
{
	bThirdPerson = bInThirdPerson;

	// 1인칭: 몸이 늘 카메라와 같은 쪽을 본다.
	// 3인칭: 몸은 움직이는 쪽을 보고, 카메라는 마우스로 따로 돌린다.
	bUseControllerRotationYaw = !bThirdPerson;
	GetCharacterMovement()->bOrientRotationToMovement = bThirdPerson;

	// 3인칭에서는 카메라가 벽을 뚫지 않게 한다.
	CameraBoom->bDoCollisionTest = bThirdPerson;
}

bool AIsekaiCharacter::IsLightOn() const
{
	return SpellLight->IsVisible();
}

void AIsekaiCharacter::OnToggleView()
{
	SetThirdPerson(!bThirdPerson);
}

void AIsekaiCharacter::AddCameraShake(float Strength)
{
	ShakeStrength = FMath::Max(ShakeStrength, Strength);
}

void AIsekaiCharacter::HandleAttackCast()
{
	AddCameraShake(0.2f);
}

void AIsekaiCharacter::HandleHealthChanged(float NewHealth, float Delta)
{
	if (Delta < 0.f && GetWorld())
	{
		LastDamageTime = GetWorld()->GetTimeSeconds();
		AddCameraShake(0.7f);
		IsekaiAudio::Play(this, TEXT("SFX_Hurt"), 1.f, 0.05f);
	}
}

void AIsekaiCharacter::Rest()
{
	Health->ResetHealth();
	Spells->RestoreMana();
	RespawnTransform = GetActorTransform();
}

void AIsekaiCharacter::HandleDied(AActor* Killer)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("You were defeated. Back to the last rest point."));
	}

	Dialogue->EndDialogue();
	GetCharacterMovement()->StopMovementImmediately();
	TeleportTo(RespawnTransform.GetLocation(), RespawnTransform.Rotator());

	if (AController* MyController = GetController())
	{
		MyController->SetControlRotation(RespawnTransform.Rotator());
	}

	Health->ResetHealth();
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
	CastDashAction = MakeAction(TEXT("IA_CastDash"), EInputActionValueType::Boolean);
	ToggleLightAction = MakeAction(TEXT("IA_ToggleLight"), EInputActionValueType::Boolean);
	ToggleViewAction = MakeAction(TEXT("IA_ToggleView"), EInputActionValueType::Boolean);
	Choice1Action = MakeAction(TEXT("IA_Choice1"), EInputActionValueType::Boolean);
	Choice2Action = MakeAction(TEXT("IA_Choice2"), EInputActionValueType::Boolean);
	Choice3Action = MakeAction(TEXT("IA_Choice3"), EInputActionValueType::Boolean);

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
	MappingContext->MapKey(CastDashAction, EKeys::RightMouseButton);
	MappingContext->MapKey(ToggleLightAction, EKeys::F);
	MappingContext->MapKey(ToggleViewAction, EKeys::V);
	MappingContext->MapKey(Choice1Action, EKeys::One);
	MappingContext->MapKey(Choice2Action, EKeys::Two);
	MappingContext->MapKey(Choice3Action, EKeys::Three);
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
	Input->BindAction(CastDashAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnCastDash);
	Input->BindAction(ToggleLightAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnToggleLight);
	Input->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AIsekaiCharacter::OnToggleView);
	Input->BindAction(Choice1Action, ETriggerEvent::Started, this, &AIsekaiCharacter::OnChoice1);
	Input->BindAction(Choice2Action, ETriggerEvent::Started, this, &AIsekaiCharacter::OnChoice2);
	Input->BindAction(Choice3Action, ETriggerEvent::Started, this, &AIsekaiCharacter::OnChoice3);
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
	return Dialogue->IsDialogueActive();
}

void AIsekaiCharacter::OnJumpStarted()
{
	if (IsInDialogue() || bMoveLocked)
	{
		return;
	}

	if (CanJump())
	{
		Jump();
	}
	else
	{
		// 아직 공중이다. 곧 착지할지 모르니 잠깐 기억해 둔다.
		JumpBufferLeft = JumpBufferSeconds;
	}
}

void AIsekaiCharacter::OnMove(const FInputActionValue& Value)
{
	// 대화나 연출 중에는 움직이지 않는다.
	if (IsInDialogue() || bMoveLocked)
	{
		return;
	}

	// 카메라가 보는 방향을 기준으로 움직인다. 3인칭에서는 몸의 방향과 카메라의 방향이 다르다.
	const FRotator YawOnly(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AIsekaiCharacter::OnLook(const FInputActionValue& Value)
{
	if (IsInDialogue() || bLookLocked)
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

	if (bMoveLocked)
	{
		return;
	}

	Interaction->TryInteract();
}

void AIsekaiCharacter::OnCastAttack()
{
	if (IsInDialogue() || !bCanCastAttack)
	{
		return;
	}

	// 3인칭에서는 쏘는 순간 몸을 조준 방향으로 돌린다.
	if (bThirdPerson)
	{
		SetActorRotation(FRotator(0.f, GetControlRotation().Yaw, 0.f));
	}

	Spells->CastAttack();
}

void AIsekaiCharacter::OnCastDash()
{
	if (!IsInDialogue() && bCanCastDash && !bMoveLocked)
	{
		Spells->CastDash();
	}
}

void AIsekaiCharacter::OnToggleLight()
{
	if (IsInDialogue())
	{
		return;
	}

	// 끄는 데는 마나가 들지 않는다.
	if (SpellLight->IsVisible())
	{
		SpellLight->SetVisibility(false);
	}
	else if (Spells->TrySpendMana(Spells->LightManaCost))
	{
		SpellLight->SetVisibility(true);
	}
}

void AIsekaiCharacter::OnChoice1()
{
	Dialogue->Choose(0);
}

void AIsekaiCharacter::OnChoice2()
{
	Dialogue->Choose(1);
}

void AIsekaiCharacter::OnChoice3()
{
	Dialogue->Choose(2);
}
