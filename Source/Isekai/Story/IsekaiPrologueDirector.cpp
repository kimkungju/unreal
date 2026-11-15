#include "Story/IsekaiPrologueDirector.h"

#include "Audio/IsekaiAudio.h"

#include "Combat/IsekaiEnemy.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Dialogue/IsekaiNPC.h"
#include "Dialogue/IsekaiObjectiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Player/IsekaiCharacter.h"
#include "UI/IsekaiHUD.h"
#include "World/IsekaiGravityAnomaly.h"
#include "World/IsekaiTimeOfDay.h"

// 이 파일의 대사와 독백은 흐름을 보기 위한 초안이다.
// 주인공과 세계의 설정이 확정되면 다시 쓴다.

namespace
{
	FText Txt(const TCHAR* Source)
	{
		return FText::FromString(FString(Source));
	}

	FIsekaiDialogueLine MakeLine(const TCHAR* Speaker, const TCHAR* Text)
	{
		FIsekaiDialogueLine Line;
		Line.Speaker = Txt(Speaker);
		Line.Text = Txt(Text);
		return Line;
	}

	const TCHAR* const OpeningLines[] =
	{
		TEXT("비가 오는 밤이었다."),
		TEXT("신호가 바뀐 줄도 모르고, 나는 걷고 있었다."),
		TEXT("빛.  소리.  그리고—"),
		TEXT("떨어지고 있었다."),
	};
}

AIsekaiPrologueDirector::AIsekaiPrologueDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AIsekaiPrologueDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 느린 화면 도중에 Play를 끝내도 다음 실행이 느려지지 않게 한다.
	SetTimeScale(1.f);
	Super::EndPlay(EndPlayReason);
}

void AIsekaiPrologueDirector::SetTimeScale(float Scale)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Scale);
}

void AIsekaiPrologueDirector::SetAmbience(const TCHAR* Name, float Volume, float FadeSeconds)
{
	IsekaiAudio::StopLoop(AmbienceLoop, FadeSeconds);
	AmbienceLoop = Name ? IsekaiAudio::StartLoop(this, Name, Volume, FadeSeconds) : nullptr;
}

void AIsekaiPrologueDirector::SetMusic(const TCHAR* Name, float Volume, float FadeSeconds)
{
	IsekaiAudio::StopLoop(MusicLoop, FadeSeconds);
	MusicLoop = Name ? IsekaiAudio::StartLoop(this, Name, Volume, FadeSeconds) : nullptr;
}

void AIsekaiPrologueDirector::Go(EStep NewStep)
{
	Step = NewStep;
	StepTime = 0.f;
	bFlagA = false;
	bFlagB = false;
}

bool AIsekaiPrologueDirector::Acquire()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	Player = Cast<AIsekaiCharacter>(PC->GetPawn());
	HUD = Cast<AIsekaiHUD>(PC->GetHUD());

	for (TActorIterator<AIsekaiTimeOfDay> It(GetWorld()); It; ++It)
	{
		TimeOfDay = *It;
		break;
	}

	return Player.IsValid() && HUD.IsValid();
}

void AIsekaiPrologueDirector::Begin()
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();

	P->bMoveLocked = true;
	P->bLookLocked = true;
	P->bCanCastAttack = false;
	P->bCanCastDash = false;
	P->GetObjectives()->SetObjective(FText::GetEmpty());

	P->GetSpells()->OnAttackCast.AddUObject(this, &AIsekaiPrologueDirector::HandleAttackCast);
	P->GetSpells()->OnDashCast.AddUObject(this, &AIsekaiPrologueDirector::HandleDashCast);

	// 독백 동안은 하늘 위에 멈춰 둔다.
	P->GetCharacterMovement()->SetMovementMode(MOVE_None);
	P->SetActorLocation(DaisCenter + FVector(-300.f, 0.f, FallHeight));

	H->SetGameplayVisible(false);
	H->SetLetterbox(true);
	H->FadeTo(FLinearColor::Black, 1.f, 0.f);

	// 아침 해가 낮게 뜬 시각에 고정한다.
	if (TimeOfDay.IsValid())
	{
		TimeOfDay->SetTimeOfDay(7.f);
		TimeOfDay->bPaused = true;
	}

	NarrationIndex = 0;
	if (bPlayOpeningNarration)
	{
		// 독백의 배경은 비 오는 밤이다.
		SetAmbience(TEXT("LOOP_Rain"), 0.8f, 1.f);
		Go(EStep::Narration);
	}
	else
	{
		StartFall();
	}
}

void AIsekaiPrologueDirector::StartFall()
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();

	// 흰 빛에서 눈이 뜨이듯 세계가 드러난다.
	H->SetNarration(FText::GetEmpty());
	H->FadeTo(FLinearColor::White, 1.f, 0.f);
	H->FadeTo(FLinearColor::White, 0.f, 1.8f);

	SetAmbience(TEXT("LOOP_Wind"), 1.f, 0.6f);

	P->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);

	// 서쪽 아래를 내려다보게 한다. 폐허와 고갯길, 멀리 마을이 보이는 방향이다.
	if (AController* PlayerController = P->GetController())
	{
		PlayerController->SetControlRotation(FRotator(-50.f, 180.f, 0.f));
	}

	// 떨어지는 동안 둘러보는 것은 허락한다.
	P->bLookLocked = false;
	P->SetTargetFOV(108.f);

	Go(EStep::Fall);
}

void AIsekaiPrologueDirector::StartCombat()
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();

	H->SetLetterbox(false);
	H->SetGameplayVisible(true);
	P->bMoveLocked = false;
	P->bLookLocked = false;
	P->GetObjectives()->SetObjective(Txt(TEXT("살아남아라")));
	P->AddCameraShake(0.6f);

	IsekaiAudio::Play(this, TEXT("SFX_Land"), 0.6f);
	SetMusic(TEXT("MUS_Combat"), 0.7f, 1.5f);

	// 첫 짐승은 동쪽에서 온다.
	FirstEnemy = SpawnEnemy(0.f);

	Go(EStep::TeachAttackApproach);
}

void AIsekaiPrologueDirector::StartSurge()
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();

	P->GetObjectives()->SetObjective(FText::GetEmpty());
	P->bCanCastAttack = false;
	P->bCanCastDash = false;

	H->SetHint(FText::GetEmpty());
	H->SetGameplayVisible(false);
	H->SetNarration(Txt(TEXT("땅이, 나를 놓았다.")));
	H->FadeTo(FLinearColor::White, 1.f, 2.8f);

	SetMusic(nullptr, 1.f, 1.f);
	SetAmbience(TEXT("LOOP_Rumble"), 1.f, 0.4f);

	// 중력이 뒤집힌다. 땅에 붙어 있으면 중력이 작용하지 않으므로 먼저 띄운다.
	if (Anomaly)
	{
		Anomaly->SetGravityScaleInside(-0.15f);
	}
	P->LaunchCharacter(FVector(0.f, 0.f, 350.f), false, true);

	Go(EStep::Surge);
}

void AIsekaiPrologueDirector::Finish()
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();

	H->FadeTo(FLinearColor::Black, 0.f, 2.f);
	H->SetLetterbox(false);
	H->SetGameplayVisible(true);

	P->bMoveLocked = false;
	P->bLookLocked = false;
	P->bCanCastAttack = true;
	P->bCanCastDash = true;
	P->GetObjectives()->SetObjective(Txt(TEXT("마법사를 따라 마을로 (다음 구간은 준비 중)")));

	SetMusic(TEXT("MUS_Ambient"), 0.6f, 3.f);

	if (TimeOfDay.IsValid())
	{
		TimeOfDay->bPaused = false;
	}

	Go(EStep::Done);
	SetActorTickEnabled(false);
}

AIsekaiEnemy* AIsekaiPrologueDirector::SpawnEnemy(float AngleDegrees)
{
	// 단상 가장자리, 기둥 고리 바로 바깥에서 나타난다.
	const float Angle = FMath::DegreesToRadians(AngleDegrees);
	const FVector Location = DaisCenter + FVector(FMath::Cos(Angle) * 2100.f, FMath::Sin(Angle) * 2100.f, 95.f);
	const FRotator Rotation(0.f, AngleDegrees + 180.f, 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIsekaiEnemy* Enemy = GetWorld()->SpawnActor<AIsekaiEnemy>(AIsekaiEnemy::StaticClass(), Location, Rotation, Params);
	if (Enemy)
	{
		Enemy->StartChasing();
		Enemies.Add(Enemy);
	}
	return Enemy;
}

int32 AIsekaiPrologueDirector::CountLivingEnemies() const
{
	int32 Living = 0;
	for (const TWeakObjectPtr<AIsekaiEnemy>& Entry : Enemies)
	{
		if (Entry.IsValid() && !Entry->IsDefeated())
		{
			++Living;
		}
	}
	return Living;
}

void AIsekaiPrologueDirector::LookAtMage(float RealDelta)
{
	AIsekaiCharacter* P = Player.Get();
	AController* PlayerController = P ? P->GetController() : nullptr;
	if (!Mage || !PlayerController)
	{
		return;
	}

	const FVector Target = Mage->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FRotator Wanted = (Target - P->GetPawnViewLocation()).Rotation();
	PlayerController->SetControlRotation(FMath::RInterpTo(PlayerController->GetControlRotation(), Wanted, RealDelta, 4.f));
}

void AIsekaiPrologueDirector::HandleAttackCast()
{
	bAttackCastSeen = true;
}

void AIsekaiPrologueDirector::HandleDashCast()
{
	bDashCastSeen = true;
}

void AIsekaiPrologueDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float RealDelta = FApp::GetDeltaTime();
	StepTime += RealDelta;

	if (Step == EStep::Init)
	{
		if (Acquire())
		{
			Begin();
		}
		return;
	}

	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();
	if (!P || !H)
	{
		return;
	}

	switch (Step)
	{
	case EStep::Narration:
	{
		// 한 줄씩: 나타나고, 머물고, 사라진다.
		if (!bFlagA)
		{
			H->SetNarration(Txt(OpeningLines[NarrationIndex]));
			bFlagA = true;

			// 세 번째 줄("빛. 소리. 그리고—")에서 빗소리가 충격음과 함께 끊긴다.
			if (NarrationIndex == 2)
			{
				SetAmbience(nullptr, 1.f, 0.05f);
				IsekaiAudio::Play(this, TEXT("SFX_Land"), 1.f);
			}
		}
		if (!bFlagB && StepTime > 2.2f)
		{
			H->SetNarration(FText::GetEmpty());
			bFlagB = true;
		}
		if (StepTime > 2.9f)
		{
			++NarrationIndex;
			if (NarrationIndex >= static_cast<int32>(UE_ARRAY_COUNT(OpeningLines)))
			{
				StartFall();
			}
			else
			{
				Go(EStep::Narration);
			}
		}
		break;
	}

	case EStep::Fall:
	{
		P->AddCameraShake(0.22f);

		if (!bFlagA && StepTime > 1.6f)
		{
			H->SetNarration(Txt(TEXT("하늘이었다.  내가 아는 어떤 하늘도 아닌.")));
			bFlagA = true;
		}
		if (!bFlagB && StepTime > 5.f)
		{
			H->SetNarration(FText::GetEmpty());
			bFlagB = true;
		}

		// 땅이 가까워지면 이상현상이 몸을 받아 준다.
		UCharacterMovementComponent* Movement = P->GetCharacterMovement();
		const double Height = P->GetActorLocation().Z - DaisCenter.Z;
		if (Height < 2200.0)
		{
			Movement->Velocity.Z = FMath::FInterpTo(static_cast<float>(Movement->Velocity.Z), -220.f, RealDelta, 2.5f);
			P->SetTargetFOV(90.f);
		}

		if (Movement->IsMovingOnGround() || StepTime > 40.f)
		{
			H->SetNarration(FText::GetEmpty());
			H->FadeTo(FLinearColor::White, 0.7f, 0.f);
			H->FadeTo(FLinearColor::White, 0.f, 0.7f);

			SetAmbience(nullptr, 1.f, 0.3f);
			IsekaiAudio::Play(this, TEXT("SFX_Land"), 1.f);

			P->AddCameraShake(1.f);
			P->SetTargetFOV(90.f);
			P->bMoveLocked = true;
			P->bLookLocked = true;

			// 떠 있는 돌 위에 내려앉았다면 단상으로 옮긴다. 흰 섬광이 가려 준다.
			if (Height > 120.0)
			{
				Movement->Velocity = FVector::ZeroVector;
				P->SetActorLocation(DaisCenter + FVector(-300.f, 0.f, 95.f));
			}

			// 쓰러지면 돌아올 자리를 여기로 잡는다.
			P->Rest();

			Go(EStep::Land);
		}
		break;
	}

	case EStep::Land:
		if (StepTime > 1.2f)
		{
			Go(EStep::TurnToMage);
		}
		break;

	case EStep::TurnToMage:
		LookAtMage(RealDelta);
		if (StepTime > 1.3f)
		{
			TArray<FIsekaiDialogueLine> Lines;
			Lines.Add(MakeLine(TEXT("???"), TEXT("...사람? 하늘에서 사람이 떨어졌다고?")));
			Lines.Add(MakeLine(TEXT("???"), TEXT("움직이지 마라. 여긴 땅이 사람을 놓아 버리는 곳이다.")));
			Lines.Add(MakeLine(TEXT("나"), TEXT("여기는... 저는 분명히, 길을 건너고 있었는데—")));
			Lines.Add(MakeLine(TEXT("???"), TEXT("말은 나중에. 놈들이 온다. 틈이 열리면 언제나 냄새를 맡고 오지.")));
			P->GetDialogue()->StartDialogue(Lines);
			Go(EStep::DialogueA);
		}
		break;

	case EStep::DialogueA:
		LookAtMage(RealDelta);
		if (!P->GetDialogue()->IsDialogueActive())
		{
			StartCombat();
		}
		break;

	case EStep::TeachAttackApproach:
	{
		if (!FirstEnemy.IsValid() || FirstEnemy->IsDefeated())
		{
			Go(EStep::Wave);
			break;
		}

		// 짐승이 코앞까지 온 순간 시간이 느려지고, 손이 먼저 움직인다.
		const double Distance = FVector::Dist2D(FirstEnemy->GetActorLocation(), P->GetActorLocation());
		if (Distance < 650.0 || StepTime > 6.f)
		{
			SetTimeScale(0.12f);
			IsekaiAudio::Play(this, TEXT("SFX_SlowMo"), 0.9f);
			P->bCanCastAttack = true;
			bAttackCastSeen = false;
			H->SetHint(Txt(TEXT("좌클릭  —  손끝의 열을 쏘아라")));
			Go(EStep::TeachAttackSlow);
		}
		break;
	}

	case EStep::TeachAttackSlow:
		if (bAttackCastSeen || StepTime > 6.f)
		{
			SetTimeScale(1.f);
			H->SetHint(FText::GetEmpty());
			P->GetObjectives()->SetObjective(Txt(TEXT("짐승을 쓰러뜨려라")));
			WaveSpawned = 0;
			Go(EStep::Wave);
		}
		break;

	case EStep::Wave:
	{
		// 세 방향에서 차례로 나타난다. 기둥 사이의 빈자리 각도다.
		static const float WaveAngles[] = { 72.f, 180.f, 288.f };
		const int32 WaveSize = static_cast<int32>(UE_ARRAY_COUNT(WaveAngles));
		if (WaveSpawned < WaveSize && StepTime > 2.f + WaveSpawned * 1.6f)
		{
			SpawnEnemy(WaveAngles[WaveSpawned]);
			P->AddCameraShake(0.35f);
			++WaveSpawned;
		}

		if (WaveSpawned >= WaveSize && CountLivingEnemies() == 0)
		{
			Go(EStep::EliteApproach);
		}
		break;
	}

	case EStep::EliteApproach:
	{
		if (!bFlagA)
		{
			// 숨 돌릴 틈을 잠깐 준 뒤 큰 놈이 온다.
			if (StepTime > 2.f)
			{
				Elite = SpawnEnemy(0.f);
				if (Elite.IsValid())
				{
					Elite->MakeElite();
				}
				P->bCanCastDash = true;
				P->AddCameraShake(0.8f);
				IsekaiAudio::Play(this, TEXT("SFX_Land"), 0.8f);
				P->GetObjectives()->SetObjective(Txt(TEXT("큰 놈이 온다")));
				bFlagA = true;
			}
			break;
		}

		if (!Elite.IsValid() || Elite->IsDefeated())
		{
			StartSurge();
			break;
		}

		// 큰 놈이 처음 몸을 부풀리는 순간을 잡아 피하는 법을 알려 준다.
		if (Elite->IsWindingUp())
		{
			SetTimeScale(0.15f);
			IsekaiAudio::Play(this, TEXT("SFX_SlowMo"), 0.9f);
			bDashCastSeen = false;
			H->SetHint(Txt(TEXT("우클릭  —  몸을 날려 피하라")));
			Go(EStep::TeachDashSlow);
		}
		break;
	}

	case EStep::TeachDashSlow:
		if (bDashCastSeen || StepTime > 2.5f)
		{
			SetTimeScale(1.f);
			H->SetHint(FText::GetEmpty());
			Go(EStep::EliteFight);
		}
		break;

	case EStep::EliteFight:
		if (!Elite.IsValid() || Elite->IsDefeated())
		{
			StartSurge();
		}
		break;

	case EStep::Surge:
		P->AddCameraShake(0.9f);
		if (StepTime > 3.2f)
		{
			if (Anomaly)
			{
				Anomaly->SetGravityScaleInside(0.25f);
			}

			// 화면이 하얗게 덮인 사이에 마법사 앞으로 옮겨 둔다.
			P->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			P->SetActorLocation(DaisCenter + FVector(-900.f, 0.f, 100.f));
			P->bMoveLocked = true;
			P->bLookLocked = true;

			H->SetNarration(FText::GetEmpty());
			H->SetLetterbox(true);
			H->FadeTo(FLinearColor::White, 0.f, 2.f);
			SetAmbience(nullptr, 1.f, 1.5f);

			Go(EStep::SurgeRecover);
		}
		break;

	case EStep::SurgeRecover:
		LookAtMage(RealDelta);
		if (StepTime > 2.4f)
		{
			TArray<FIsekaiDialogueLine> Lines;
			Lines.Add(MakeLine(TEXT("마법사"), TEXT("...살아 있나. 처음 치고는 쓸 만했다.")));
			Lines.Add(MakeLine(TEXT("나"), TEXT("방금 그건... 제가 한 건가요?")));
			Lines.Add(MakeLine(TEXT("마법사"), TEXT("네가 한 거다. 그리고 이 틈이 열린 것도, 아마 너 때문이고.")));
			Lines.Add(MakeLine(TEXT("마법사"), TEXT("이름은 걸으면서 듣지. 해가 지기 전에 마을에 닿아야 한다.")));
			P->GetDialogue()->StartDialogue(Lines);
			Go(EStep::DialogueB);
		}
		break;

	case EStep::DialogueB:
		LookAtMage(RealDelta);
		if (!P->GetDialogue()->IsDialogueActive())
		{
			H->FadeTo(FLinearColor::Black, 1.f, 1.5f);
			Go(EStep::TitleIn);
		}
		break;

	case EStep::TitleIn:
		if (StepTime > 1.8f)
		{
			// 제목은 아직 정해지지 않았다. 지금은 프로젝트의 임시 이름을 쓴다.
			H->SetTitle(Txt(TEXT("ISEKAI")), Txt(TEXT("프롤로그   ·   떨어진 자")));
			IsekaiAudio::Play(this, TEXT("SFX_Title"), 1.f);
			Go(EStep::TitleHold);
		}
		break;

	case EStep::TitleHold:
		if (StepTime > 4.5f)
		{
			H->SetTitle(FText::GetEmpty(), FText::GetEmpty());
			Go(EStep::TitleOut);
		}
		break;

	case EStep::TitleOut:
		if (StepTime > 1.4f)
		{
			Finish();
		}
		break;

	default:
		break;
	}
}
