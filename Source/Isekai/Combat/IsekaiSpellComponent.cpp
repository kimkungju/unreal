#include "Combat/IsekaiSpellComponent.h"

#include "Audio/IsekaiAudio.h"

#include "Combat/IsekaiSpellProjectile.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UIsekaiSpellComponent::UIsekaiSpellComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	AttackProjectileClass = AIsekaiSpellProjectile::StaticClass();
}

void UIsekaiSpellComponent::BeginPlay()
{
	Super::BeginPlay();
	Mana = MaxMana;
}

void UIsekaiSpellComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Mana = FMath::Min(MaxMana, Mana + ManaRegenPerSecond * DeltaTime);
	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
	DashCooldownRemaining = FMath::Max(0.f, DashCooldownRemaining - DeltaTime);
}

bool UIsekaiSpellComponent::CastAttack()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !AttackProjectileClass)
	{
		return false;
	}

	if (CooldownRemaining > 0.f || Mana < AttackManaCost)
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// 조준점이 가리키는 곳을 찾는다. 3인칭에서는 카메라가 몸 뒤에 있어서
	// 눈에서 곧장 쏘면 조준점과 어긋난다. 카메라에서 선을 쏴 닿는 곳을 겨눈다.
	FVector AimDirection = EyeRotation.Vector();

	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PC)
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

		// 카메라와 몸 사이에 있는 것은 건너뛴다.
		const FVector CameraForward = CameraRotation.Vector();
		const FVector TraceStart = CameraLocation + CameraForward * FVector::Dist(CameraLocation, EyeLocation);
		const FVector TraceEnd = CameraLocation + CameraForward * 10000.f;

		FCollisionQueryParams AimParams(SCENE_QUERY_STAT(IsekaiSpellAim), false, Owner);
		FHitResult AimHit;
		const FVector AimPoint = World->LineTraceSingleByChannel(AimHit, TraceStart, TraceEnd, ECC_Visibility, AimParams)
			? AimHit.ImpactPoint
			: TraceEnd;

		const FVector ToAimPoint = AimPoint - EyeLocation;
		if (!ToAimPoint.IsNearlyZero())
		{
			AimDirection = ToAimPoint.GetSafeNormal();
		}
	}

	// 눈 바로 앞에서 생기면 화면을 가리므로 조금 앞에서 시작한다.
	const FVector SpawnLocation = EyeLocation + AimDirection * 60.f;
	const FRotator SpawnRotation = AimDirection.Rotation();

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Cast<APawn>(Owner);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!World->SpawnActor<AIsekaiSpellProjectile>(AttackProjectileClass, SpawnLocation, SpawnRotation, Params))
	{
		return false;
	}

	Mana -= AttackManaCost;
	CooldownRemaining = AttackCooldown;
	IsekaiAudio::Play(this, TEXT("SFX_Cast"), 0.8f, 0.06f);
	OnAttackCast.Broadcast();
	return true;
}

bool UIsekaiSpellComponent::TrySpendMana(float Amount)
{
	if (Mana < Amount)
	{
		return false;
	}

	Mana -= Amount;
	return true;
}

bool UIsekaiSpellComponent::CastDash()
{
	ACharacter* Caster = Cast<ACharacter>(GetOwner());
	if (!Caster || DashCooldownRemaining > 0.f || Mana < DashManaCost)
	{
		return false;
	}

	FVector Direction = Caster->GetLastMovementInputVector();
	Direction.Z = 0.0;
	if (!Direction.Normalize())
	{
		Direction = Caster->GetActorForwardVector();
	}

	Caster->LaunchCharacter(Direction * DashSpeed + FVector(0.f, 0.f, DashLift), true, true);

	Mana -= DashManaCost;
	DashCooldownRemaining = DashCooldown;
	IsekaiAudio::Play(this, TEXT("SFX_Dash"), 0.9f, 0.05f);
	OnDashCast.Broadcast();
	return true;
}
