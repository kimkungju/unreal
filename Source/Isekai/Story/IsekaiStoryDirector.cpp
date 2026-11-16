#include "Story/IsekaiStoryDirector.h"

#include "Assets/IsekaiAssets.h"
#include "Audio/IsekaiAudio.h"
#include "Combat/IsekaiEnemy.h"
#include "Combat/IsekaiHealthComponent.h"
#include "Combat/IsekaiSpellComponent.h"
#include "Combat/IsekaiTargetDummy.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Dialogue/IsekaiDialogueComponent.h"
#include "Dialogue/IsekaiExaminable.h"
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
#include "World/IsekaiBrazier.h"
#include "World/IsekaiTimeOfDay.h"

// 이 파일의 대사, 자막, 그림 설명은 모두 초안이다.
// 그림 파일은 Content/Story/Cut_01.png ~ Cut_05.png. 없으면 설명 문구가 대신 뜬다.

namespace
{
	const float StandHeight = 92.f;

	FText Txt(const TCHAR* Source)
	{
		return FText::FromString(FString(Source));
	}

	FIsekaiDialogueLine Line(const TCHAR* Speaker, const TCHAR* Text)
	{
		FIsekaiDialogueLine Result;
		Result.Speaker = Txt(Speaker);
		Result.Text = Txt(Text);
		return Result;
	}

	enum ECutscene : int32
	{
		CutsceneOpening = 1,
		CutsceneDeparture = 2,
	};

	const TCHAR* const Me = TEXT("나");
	const TCHAR* const SeraName = TEXT("세라");
	const TCHAR* const ElderName = TEXT("촌장");

	const FName FlagTruth(TEXT("ToldTruthToSera"));
	const FName FlagHid(TEXT("HidOriginFromSera"));
	const FName FlagForVillage(TEXT("LeftForVillage"));
	const FName FlagForHome(TEXT("LeftToFindWayHome"));
}

AIsekaiStoryDirector::AIsekaiStoryDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AIsekaiStoryDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::EndPlay(EndPlayReason);
}

// ===========================================================================
// 도우미
// ===========================================================================

bool AIsekaiStoryDirector::Acquire()
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

	return Player.IsValid() && HUD.IsValid() && Sera;
}

void AIsekaiStoryDirector::SetFreeControl(bool bFree)
{
	AIsekaiCharacter* P = Player.Get();
	P->bMoveLocked = !bFree;
	P->bLookLocked = !bFree;
	P->bCanCastAttack = bFree;
	P->bCanCastDash = bFree;
	HUD->SetLetterbox(!bFree);
	HUD->SetGameplayVisible(bFree);
}

void AIsekaiStoryDirector::PlacePlayer(const FVector& Floor, const FVector& LookAtPoint)
{
	AIsekaiCharacter* P = Player.Get();
	P->GetCharacterMovement()->StopMovementImmediately();
	P->SetActorLocation(Floor + FVector(0.f, 0.f, StandHeight), false, nullptr, ETeleportType::TeleportPhysics);

	if (AController* PC = P->GetController())
	{
		FRotator Facing = (LookAtPoint - (Floor + FVector(0.f, 0.f, 150.f))).Rotation();
		Facing.Roll = 0.f;
		PC->SetControlRotation(Facing);
	}
}

void AIsekaiStoryDirector::PlaceNPC(AIsekaiNPC* NPC, const FVector& Floor, const FVector& LookAtPoint)
{
	if (!NPC)
	{
		return;
	}

	NPC->StopWalking();
	NPC->GetCharacterMovement()->StopMovementImmediately();
	NPC->SetActorLocation(Floor + FVector(0.f, 0.f, StandHeight), false, nullptr, ETeleportType::TeleportPhysics);
	NPC->SetActorRotation(FRotator(0.f, (LookAtPoint - Floor).Rotation().Yaw, 0.f));
}

void AIsekaiStoryDirector::LookAt(AActor* Target, float RealDelta)
{
	AIsekaiCharacter* P = Player.Get();
	AController* PC = P ? P->GetController() : nullptr;
	if (!Target || !PC)
	{
		return;
	}

	const FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 55.f);
	const FRotator Wanted = (Aim - P->GetPawnViewLocation()).Rotation();
	PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Wanted, RealDelta, 4.f));

	// 말하는 쪽도 플레이어를 본다.
	if (AIsekaiNPC* NPC = Cast<AIsekaiNPC>(Target))
	{
		if (!NPC->IsWalking())
		{
			const float Yaw = (P->GetActorLocation() - NPC->GetActorLocation()).Rotation().Yaw;
			NPC->SetActorRotation(FMath::RInterpTo(NPC->GetActorRotation(), FRotator(0.f, Yaw, 0.f), RealDelta, 5.f));
		}
	}
}

void AIsekaiStoryDirector::SetObjective(const TCHAR* Text)
{
	Player->GetObjectives()->SetObjective(Text ? Txt(Text) : FText::GetEmpty());
}

void AIsekaiStoryDirector::Bark(const TCHAR* Speaker, const TCHAR* Text, float Duration)
{
	HUD->ShowBark(Txt(Speaker), Txt(Text), Duration);
}

void AIsekaiStoryDirector::Toast(const TCHAR* Header, const TCHAR* Body)
{
	HUD->ShowToast(Txt(Header), Txt(Body));
	IsekaiAudio::Play(this, TEXT("SFX_Impact"), 0.5f);
}

void AIsekaiStoryDirector::SetHour(float Hour, bool bFlow)
{
	if (TimeOfDay.IsValid())
	{
		TimeOfDay->SetTimeOfDay(Hour);
		TimeOfDay->bPaused = !bFlow;
	}
}

void AIsekaiStoryDirector::SetAmbience(const TCHAR* Name, float Volume, float FadeSeconds)
{
	IsekaiAudio::StopLoop(AmbienceLoop, FadeSeconds);
	AmbienceLoop = Name ? IsekaiAudio::StartLoop(this, Name, Volume, FadeSeconds) : nullptr;
}

void AIsekaiStoryDirector::SetMusic(const TCHAR* Name, float Volume, float FadeSeconds)
{
	IsekaiAudio::StopLoop(MusicLoop, FadeSeconds);
	MusicLoop = Name ? IsekaiAudio::StartLoop(this, Name, Volume, FadeSeconds) : nullptr;
}

AIsekaiEnemy* AIsekaiStoryDirector::SpawnEnemy(const FVector& Floor, const FVector& LookAtPoint)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FRotator Rotation(0.f, (LookAtPoint - Floor).Rotation().Yaw, 0.f);
	AIsekaiEnemy* Enemy = GetWorld()->SpawnActor<AIsekaiEnemy>(AIsekaiEnemy::StaticClass(),
		Floor + FVector(0.f, 0.f, 95.f), Rotation, Params);
	if (Enemy)
	{
		Enemies.Add(Enemy);
	}
	return Enemy;
}

int32 AIsekaiStoryDirector::CountLiving() const
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

// ===========================================================================
// 공용 흐름
// ===========================================================================

void AIsekaiStoryDirector::Go(EStep NewStep)
{
	Step = NewStep;
	StepTime = 0.f;
	bFlagA = false;
	Enter(NewStep);
}

void AIsekaiStoryDirector::Talk(const TArray<FIsekaiDialogueLine>& Lines, AActor* LookTarget, EStep Next)
{
	AIsekaiCharacter* P = Player.Get();
	P->bMoveLocked = true;
	P->bLookLocked = true;
	HUD->SetLetterbox(true);

	TalkLookAt = LookTarget;
	NextStep = Next;
	P->GetDialogue()->StartDialogue(Lines);

	Step = EStep::Talk;
	StepTime = 0.f;
	bFlagA = false;
}

void AIsekaiStoryDirector::Transition(const TCHAR* Caption, float Hour, EStep Next)
{
	TransitionCaption = Caption ? FString(Caption) : FString();
	TransitionHour = Hour;
	NextStep = Next;

	HUD->FadeTo(FLinearColor::Black, 1.f, 1.f);

	Step = EStep::Transition;
	StepTime = 0.f;
	bFlagA = false;
}

void AIsekaiStoryDirector::WaitThen(float Seconds, EStep Next)
{
	WaitSeconds = Seconds;
	NextStep = Next;
	Step = EStep::Wait;
	StepTime = 0.f;
	bFlagA = false;
}

void AIsekaiStoryDirector::PlayCutscene(const TArray<FShot>& InShots, int32 InCutsceneId, EStep Next)
{
	Shots = InShots;
	CutsceneId = InCutsceneId;
	NextStep = Next;

	AIsekaiCharacter* P = Player.Get();
	P->bMoveLocked = true;
	P->bLookLocked = true;
	HUD->SetGameplayVisible(false);
	HUD->BeginCutscene();
	HUD->FadeTo(FLinearColor::Black, 0.f, 0.6f);

	Step = EStep::Cutscene;
	StepTime = 0.f;
	StartShot(0);
}

void AIsekaiStoryDirector::StartShot(int32 Index)
{
	ShotIndex = Index;
	LineIndex = 0;
	LineTime = -1.0f;
	bLineShown = false;

	const FShot& Shot = Shots[Index];

	// 자막이 다 나올 동안 천천히 움직이도록 길이를 맞춘다.
	float Duration = 1.5f;
	for (const FString& Text : Shot.Lines)
	{
		Duration += 2.f + Text.Len() * 0.085f + 0.45f;
	}

	UTexture2D* Texture = IsekaiAssets::FindTexture(TEXT("Story"), *Shot.Image);
	const FText Placeholder = FText::FromString(FString::Printf(TEXT("%s\n\nContent/Story/%s.png"), *Shot.Placeholder, *Shot.Image));
	HUD->ShowCutsceneShot(Texture, Placeholder, Shot.PanFrom, Shot.PanTo, Shot.ZoomFrom, Shot.ZoomTo, Duration);
}

void AIsekaiStoryDirector::TickCutscene(float RealDelta)
{
	const FShot& Shot = Shots[ShotIndex];
	LineTime += RealDelta;

	if (!bLineShown && LineTime >= 0.f)
	{
		HUD->SetCutsceneSubtitle(FText::FromString(Shot.Lines[LineIndex]));
		bLineShown = true;
		OnCutsceneLine(ShotIndex, LineIndex);
	}

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const bool bSkipPressed = PC && PC->WasInputKeyJustPressed(EKeys::E);
	const float LineDuration = 2.f + Shot.Lines[LineIndex].Len() * 0.085f;

	if (bLineShown && (LineTime > LineDuration || bSkipPressed))
	{
		HUD->SetCutsceneSubtitle(FText::GetEmpty());
		bLineShown = false;
		LineTime = -0.45f;
		++LineIndex;

		if (LineIndex >= Shot.Lines.Num())
		{
			if (ShotIndex + 1 < Shots.Num())
			{
				StartShot(ShotIndex + 1);
			}
			else
			{
				// 마지막 장: 검게 덮은 뒤 다음으로.
				HUD->FadeTo(FLinearColor::Black, 1.f, 0.9f);
				WaitThen(1.1f, NextStep);
			}
		}
	}
}

void AIsekaiStoryDirector::OnCutsceneLine(int32 Shot, int32 LineNumber)
{
	// 그림과 자막에 맞춰 소리를 바꾼다.
	if (CutsceneId == CutsceneOpening)
	{
		if (Shot == 0 && LineNumber == 0)
		{
			SetAmbience(TEXT("LOOP_Rain"), 0.8f, 1.f);
		}
		else if (Shot == 1 && LineNumber == 1)
		{
			// "빛. 소리. 그리고—" 에서 빗소리가 충격음과 함께 끊긴다.
			SetAmbience(nullptr, 1.f, 0.05f);
			IsekaiAudio::Play(this, TEXT("SFX_Land"), 1.f);
		}
		else if (Shot == 2 && LineNumber == 0)
		{
			SetAmbience(TEXT("LOOP_Wind"), 0.9f, 0.6f);
		}
		else if (Shot == 3 && LineNumber == 0)
		{
			SetAmbience(nullptr, 1.f, 1.5f);
			IsekaiAudio::Play(this, TEXT("SFX_Land"), 0.6f);
		}
	}
	else if (CutsceneId == CutsceneDeparture)
	{
		if (Shot == 0 && LineNumber == 0)
		{
			SetMusic(TEXT("MUS_Ambient"), 0.7f, 2.f);
		}
	}
}

// ===========================================================================
// 시작
// ===========================================================================

void AIsekaiStoryDirector::Begin()
{
	AIsekaiCharacter* P = Player.Get();

	P->GetObjectives()->SetObjective(FText::GetEmpty());
	P->bAttackLearned = false;
	P->bDashLearned = false;
	P->bLightLearned = false;

	// 중간부터 시험할 때는 그 앞에서 배웠을 것들을 미리 채운다.
	switch (StartAt)
	{
	case EIsekaiStoryStart::Opening:
		HUD->FadeTo(FLinearColor::Black, 1.f, 0.f);
		Go(EStep::Opening);
		break;

	case EIsekaiStoryStart::Wake:
		HUD->FadeTo(FLinearColor::Black, 1.f, 0.f);
		Go(EStep::Wake);
		break;

	case EIsekaiStoryStart::Lesson1:
		SetHour(9.5f, false);
		PlacePlayer(YardCenter - TowardVillage * 300.f, YardCenter);
		Go(EStep::Lesson1Intro);
		break;

	case EIsekaiStoryStart::Lesson2:
		P->bAttackLearned = true;
		SetHour(15.f, false);
		Go(EStep::Lesson2Intro);
		break;

	case EIsekaiStoryStart::Lesson3:
		P->bAttackLearned = true;
		P->bDashLearned = true;
		SetHour(21.f, false);
		Go(EStep::Lesson3Intro);
		break;

	case EIsekaiStoryStart::VillageAttack:
		P->bAttackLearned = true;
		P->bDashLearned = true;
		P->bLightLearned = true;
		SetHour(21.5f, false);
		PlacePlayer(YardCenter, YardCenter + TowardVillage * 1000.f);
		PlaceNPC(Sera, YardFire + FVector(200.f, 0.f, 0.f), YardCenter);
		Go(EStep::AttackStart);
		break;

	case EIsekaiStoryStart::Departure:
		P->bAttackLearned = true;
		P->bDashLearned = true;
		P->bLightLearned = true;
		HUD->FadeTo(FLinearColor::Black, 1.f, 0.f);
		Go(EStep::Departure);
		break;
	}
}

// ===========================================================================
// 단계에 들어설 때 한 번
// ===========================================================================

void AIsekaiStoryDirector::Enter(EStep NewStep)
{
	AIsekaiCharacter* P = Player.Get();
	AIsekaiHUD* H = HUD.Get();
	const FVector SeraYardSpot = YardCenter - TowardVillage * 450.f + FVector(0.f, -350.f, 0.f);

	switch (NewStep)
	{
	// ---------------- 그림 컷: 사고에서 발견까지 ----------------
	case EStep::Opening:
	{
		TArray<FShot> Opening;

		FShot Shot1;
		Shot1.Image = TEXT("Cut_01");
		Shot1.Placeholder = TEXT("비 오는 밤의 도시 횡단보도. 젖은 아스팔트에 신호등 불빛이 번진다.\n우산 없이 걷는 사람의 뒷모습.");
		Shot1.Lines = { TEXT("비가 오는 밤이었다."), TEXT("신호가 바뀐 줄도 모르고, 나는 걷고 있었다.") };
		Shot1.PanFrom = FVector2D(-30.f, 0.f);
		Shot1.PanTo = FVector2D(30.f, 0.f);
		Opening.Add(Shot1);

		FShot Shot2;
		Shot2.Image = TEXT("Cut_02");
		Shot2.Placeholder = TEXT("화면을 가득 채운 하얀 헤드라이트. 빗줄기가 빛 속에서 멈춘 듯 보인다.");
		Shot2.Lines = { TEXT("눈부신 빛."), TEXT("소리.  그리고—") };
		Shot2.ZoomFrom = 1.f;
		Shot2.ZoomTo = 1.35f;
		Opening.Add(Shot2);

		FShot Shot3;
		Shot3.Image = TEXT("Cut_03");
		Shot3.Placeholder = TEXT("구름 위 낯선 하늘에서 떨어지는 사람. 아래로 폐허와 공중에 뜬 바위들,\n멀리 산맥과 두 개의 달.");
		Shot3.Lines = { TEXT("떨어지고 있었다."), TEXT("내가 아는 어떤 하늘도 아닌 곳으로.") };
		Shot3.PanFrom = FVector2D(0.f, -40.f);
		Shot3.PanTo = FVector2D(0.f, 40.f);
		Opening.Add(Shot3);

		FShot Shot4;
		Shot4.Image = TEXT("Cut_04");
		Shot4.Placeholder = TEXT("부서진 돌기둥 사이에 쓰러진 주인공. 그 위로 몸을 숙인 망토 차림의 여자(세라).\n등 뒤에서 아침 햇빛.");
		Shot4.Lines = { TEXT("눈이 감기기 직전, 누군가의 목소리를 들었다."), TEXT("\"...하늘에서, 사람이?\"") };
		Shot4.ZoomFrom = 1.2f;
		Shot4.ZoomTo = 1.05f;
		Opening.Add(Shot4);

		PlayCutscene(Opening, CutsceneOpening, EStep::Wake);
		break;
	}

	// ---------------- 집에서 깨어남 ----------------
	case EStep::Wake:
		H->EndCutscene();
		SetHour(9.f, false);
		SetFreeControl(false);
		P->SetThirdPerson(false);

		PlacePlayer(WakeSpot, WakeSpot + FVector(0.f, 0.f, 500.f) + TowardVillage * 60.f);
		PlaceNPC(Sera, SeraBedside, WakeSpot);
		P->Rest();

		// 천천히 눈을 뜬다.
		H->FadeTo(FLinearColor::Black, 0.f, 3.f);
		SetMusic(TEXT("MUS_Ambient"), 0.45f, 4.f);
		break;

	case EStep::WakeReply:
	{
		TArray<FIsekaiDialogueLine> Lines;
		if (P->GetDialogue()->GetLastChoice() == 0)
		{
			P->SetStoryFlag(FlagTruth);
			Lines.Add(Line(SeraName, TEXT("...다른 세계. 틈 너머에서 온 자가 있었다는 옛 기록은 읽은 적이 있다. 믿지는 않았지만.")));
			Lines.Add(Line(SeraName, TEXT("그 말은 마을 사람들 앞에서는 하지 마라. 틈을 반기는 사람은 없다.")));
		}
		else
		{
			P->SetStoryFlag(FlagHid);
			Lines.Add(Line(SeraName, TEXT("그래. 그 높이에서 떨어졌으면 그럴 수도 있지.")));
			Lines.Add(Line(SeraName, TEXT("...다만 네 머리가 잊은 걸, 네 마력은 기억하고 있는 것 같더군.")));
		}
		Lines.Add(Line(SeraName, TEXT("어쨌든 문제가 하나 있다. 너한테서 마력이 새고 있어. 그릇은 큰데 뚜껑이 없는 셈이지.")));
		Lines.Add(Line(SeraName, TEXT("다루는 법을 모르면 몸이 먼저 상한다. 내가 가르쳐 주마.")));
		Lines.Add(Line(SeraName, TEXT("대신 장작 패기와 물 긷기는 네 몫이다. 몸을 추스르면 마당으로 나와라.")));
		Talk(Lines, Sera, EStep::LeaveRoom);
		break;
	}

	// 세라가 먼저 나가는 사이 잠깐 눈을 감는다.
	case EStep::LeaveRoom:
		Transition(nullptr, 9.2f, EStep::GoToYard);
		break;

	case EStep::GoToYard:
		SetFreeControl(true);
		PlaceNPC(Sera, SeraYardSpot, YardCenter);
		SetObjective(TEXT("마당으로 나가 세라를 만나라"));
		break;

	// ---------------- 수련 1: 마법탄 ----------------
	case EStep::Lesson1Intro:
	{
		PlaceNPC(Sera, SeraYardSpot, YardCenter);
		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(SeraName, TEXT("왔나. 그럼 첫 번째다.")));
		Lines.Add(Line(SeraName, TEXT("몸 안에서 새는 걸 손끝으로 모아서, 한 점으로 내보낸다. 생각보다 단순하다.")));
		Lines.Add(Line(SeraName, TEXT("저 수정 셋을 깨 봐라. 마력이 바닥나면 숨을 고르면 다시 찬다.")));
		Talk(Lines, Sera, EStep::Lesson1);
		break;
	}

	case EStep::Lesson1:
	{
		SetFreeControl(true);
		P->bAttackLearned = true;
		Toast(TEXT("새 기술"), TEXT("마법탄  ·  좌클릭"));
		SetObjective(TEXT("수정을 깨라  0 / 3"));

		Crystals.Reset();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, TowardVillage);
		const FVector Spots[3] =
		{
			YardCenter + TowardVillage * 900.f - Side * 500.f,
			YardCenter + TowardVillage * 1150.f,
			YardCenter + TowardVillage * 900.f + Side * 500.f,
		};
		for (const FVector& Spot : Spots)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AIsekaiTargetDummy* Crystal = GetWorld()->SpawnActor<AIsekaiTargetDummy>(AIsekaiTargetDummy::StaticClass(),
				Spot + FVector(0.f, 0.f, 90.f), FRotator::ZeroRotator, Params);
			if (Crystal)
			{
				Crystal->bRespawn = false;
				Crystals.Add(Crystal);
			}
		}
		break;
	}

	case EStep::Lesson1Done:
	{
		P->GetSpells()->MaxMana = 120.f;
		P->GetSpells()->RestoreMana();
		Toast(TEXT("성장"), TEXT("최대 마나  100 → 120"));

		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(SeraName, TEXT("...나쁘지 않다. 처음 치고는.")));
		Lines.Add(Line(Me, TEXT("손이 아직도 따끔거려요.")));
		Lines.Add(Line(SeraName, TEXT("새던 게 길을 찾은 거다. 그 따끔거림이 익숙해질 때까지 매일 한다.")));
		Talk(Lines, Sera, EStep::Skip1);
		break;
	}

	// ---------------- 수련 2: 가속 ----------------
	case EStep::Skip1:
		Transition(TEXT("보름 뒤"), 15.f, EStep::Lesson2Intro);
		break;

	case EStep::Lesson2Intro:
		{
			PlacePlayer(YardCenter - TowardVillage * 200.f, YardCenter + TowardVillage * 600.f);
			PlaceNPC(Sera, SeraYardSpot, YardCenter);

			TArray<FIsekaiDialogueLine> Lines;
			Lines.Add(Line(SeraName, TEXT("피하는 법을 모르면 쏘는 법도 소용없다.")));
			Lines.Add(Line(SeraName, TEXT("이 녀석은 내가 흙으로 빚은 연습 상대다. 맞으면 아프긴 해도 죽지는 않는다.")));
			Lines.Add(Line(SeraName, TEXT("몸을 일으키고 눈이 붉어지는 순간이 틈이다. 우클릭으로 몸을 날려 피해라. 세 번.")));
			Talk(Lines, Sera, EStep::Lesson2Dodge);
		}
		break;

	case EStep::Lesson2Dodge:
	{
		SetFreeControl(true);
		P->bDashLearned = true;
		Toast(TEXT("새 기술"), TEXT("가속  ·  우클릭"));
		DodgeCount = 0;
		SetObjective(TEXT("연습 상대의 공격을 피하라  0 / 3"));

		AIsekaiEnemy* NewGolem = SpawnEnemy(YardCenter + TowardVillage * 900.f, YardCenter);
		if (NewGolem)
		{
			NewGolem->MakeTrainingGolem();
			NewGolem->StartChasing();
			NewGolem->OnAttackResolved.AddUObject(this, &AIsekaiStoryDirector::HandleGolemAttack);
		}
		Golem = NewGolem;
		break;
	}

	case EStep::Lesson2Defeat:
		if (Golem.IsValid())
		{
			Golem->SetInvulnerable(false);
		}
		Bark(SeraName, TEXT("됐다. 이제 부숴 봐라."), 3.f);
		SetObjective(TEXT("연습 상대를 쓰러뜨려라"));
		break;

	case EStep::Lesson2Done:
	{
		P->GetHealthComponent()->MaxHealth = 120.f;
		P->GetHealthComponent()->ResetHealth();
		Toast(TEXT("성장"), TEXT("최대 체력  100 → 120"));

		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(SeraName, TEXT("맞기 전에 움직이는군. 보름 전엔 맞고 나서 움직였는데.")));
		Lines.Add(Line(Me, TEXT("칭찬이죠?")));
		Lines.Add(Line(SeraName, TEXT("기록이다. 측량사는 칭찬 대신 기록을 한다.")));
		Talk(Lines, Sera, EStep::Skip2);
		break;
	}

	// ---------------- 수련 3: 빛 ----------------
	case EStep::Skip2:
		Transition(TEXT("한 달 뒤, 밤"), 21.f, EStep::Lesson3Intro);
		break;

	case EStep::Lesson3Intro:
		{
			PlacePlayer(YardCenter - TowardVillage * 200.f, YardCenter + TowardVillage * 600.f);
			PlaceNPC(Sera, SeraYardSpot, YardCenter);

			TArray<FIsekaiDialogueLine> Lines;
			Lines.Add(Line(SeraName, TEXT("마지막은 쏘는 게 아니라 붙잡는 연습이다.")));
			Lines.Add(Line(SeraName, TEXT("F로 손끝에 빛을 띄워라. 그 빛을 꺼뜨리지 말고 마당의 화로 셋에 옮겨 붙여.")));
			Talk(Lines, Sera, EStep::Lesson3);
		}
		break;

	case EStep::Lesson3:
	{
		SetFreeControl(true);
		P->bLightLearned = true;
		Toast(TEXT("새 기술"), TEXT("빛  ·  F"));
		SetObjective(TEXT("화로에 불을 옮겨라  0 / 3"));

		Braziers.Reset();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, TowardVillage);
		const FVector Spots[3] =
		{
			YardCenter + TowardVillage * 1000.f,
			YardCenter + Side * 1000.f,
			YardCenter - Side * 1000.f,
		};
		for (const FVector& Spot : Spots)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AIsekaiBrazier* Brazier = GetWorld()->SpawnActor<AIsekaiBrazier>(AIsekaiBrazier::StaticClass(), Spot, FRotator::ZeroRotator, Params))
			{
				Braziers.Add(Brazier);
			}
		}
		break;
	}

	case EStep::Lesson3Done:
	{
		P->GetSpells()->MaxMana = 150.f;
		P->GetSpells()->RestoreMana();
		Toast(TEXT("성장"), TEXT("최대 마나  120 → 150"));

		PlaceNPC(Sera, YardFire + FVector(180.f, 120.f, 0.f), YardFire);

		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(SeraName, TEXT("이제 혼자서도 몸은 지키겠군. 앉아라.")));
		Lines.Add(Line(Me, TEXT("세라는 왜 저를 거둬 주신 거예요?")));
		Lines.Add(Line(SeraName, TEXT("틈을 재는 게 내 일이다. 틈에서 떨어진 걸 주웠으면 끝까지 재 봐야지.")));
		Lines.Add(Line(SeraName, TEXT("...그런데 요즘 이상하다. 짐승들이 자꾸 마을 가까이 내려온다.")));
		Lines.Add(Line(SeraName, TEXT("마치 무언가를 듣고 찾아오는 것처럼.")));
		Talk(Lines, Sera, EStep::AttackStart);
		break;
	}

	// ---------------- 습격 ----------------
	case EStep::AttackStart:
	{
		SetFreeControl(true);
		SetObjective(TEXT("집을 지켜라"));
		P->AddCameraShake(0.8f);
		IsekaiAudio::Play(this, TEXT("SFX_Land"), 0.9f);
		SetMusic(TEXT("MUS_Combat"), 0.75f, 0.8f);
		Bark(SeraName, TEXT("...왔군. 일어서라! 수련한 대로만 해!"), 4.f);

		Enemies.Reset();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, TowardVillage);
		const FVector Spots[4] =
		{
			YardCenter + TowardVillage * 2200.f + Side * 600.f,
			YardCenter + TowardVillage * 2000.f - Side * 900.f,
			YardCenter + Side * 2200.f,
			YardCenter - Side * 2200.f,
		};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			if (AIsekaiEnemy* Enemy = SpawnEnemy(Spots[Index], YardCenter))
			{
				if (Index == 1)
				{
					Enemy->MakeRanged();
				}
				Enemy->StartChasing();
			}
		}
		break;
	}

	case EStep::AttackElite:
	{
		SetObjective(TEXT("큰 놈이 온다"));
		P->AddCameraShake(0.9f);
		IsekaiAudio::Play(this, TEXT("SFX_Land"), 1.f);
		Bark(SeraName, TEXT("마을 쪽에서 큰 놈이다! 몸을 일으키면 피해!"), 4.f);

		Elite = SpawnEnemy(YardCenter + TowardVillage * 2600.f, YardCenter);
		if (Elite.IsValid())
		{
			Elite->MakeElite();
			Elite->StartChasing();
		}
		break;
	}

	case EStep::Aftermath:
	{
		SetMusic(nullptr, 1.f, 1.f);
		SetHour(22.f, false);
		P->Rest();
		PlacePlayer(YardCenter, YardCenter + TowardVillage * 600.f);
		PlaceNPC(Sera, YardCenter + TowardVillage * 450.f + FVector(0.f, -250.f, 0.f), YardCenter);
		PlaceNPC(Elder, YardCenter + TowardVillage * 600.f + FVector(0.f, 250.f, 0.f), YardCenter);

		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(ElderName, TEXT("세라, 이게 이번 달에만 몇 번째인가. 저 아이가 온 뒤로 짐승들이...")));
		Lines.Add(Line(SeraName, TEXT("...알고 있습니다.")));
		Lines.Add(Line(SeraName, TEXT("틈을 건너온 자의 마력은 짐승을 부른다. 강해질수록 더 멀리까지 들리지.")));
		Lines.Add(Line(SeraName, TEXT("내가 가르친 만큼, 놈들도 더 크게 들은 거다.")));
		if (P->HasStoryFlag(FlagTruth))
		{
			Lines.Add(Line(SeraName, TEXT("네가 처음부터 사실대로 말해 줘서, 나는 이걸 각오하고 있었다.")));
		}
		else
		{
			Lines.Add(Line(SeraName, TEXT("네가 어디서 왔는지 말하지 않아도... 이건 숨길 수가 없더군.")));
		}
		Lines.Add(Line(ElderName, TEXT("그럼 어쩌란 말인가. 마을을 통째로 비우기라도 하라는 건가?")));

		FIsekaiDialogueLine Choice = Line(Me, TEXT("..."));
		Choice.Choices.Add(Txt(TEXT("제가 떠나겠습니다. 이 마을을 위해서.")));
		Choice.Choices.Add(Txt(TEXT("제가 떠나겠습니다. ...어차피 돌아갈 길을 찾아야 하니까.")));
		Lines.Add(Choice);

		H->FadeTo(FLinearColor::Black, 0.f, 1.2f);
		Talk(Lines, Sera, EStep::AftermathReply);
		break;
	}

	case EStep::AftermathReply:
	{
		TArray<FIsekaiDialogueLine> Lines;
		if (P->GetDialogue()->GetLastChoice() == 0)
		{
			P->SetStoryFlag(FlagForVillage);
			Lines.Add(Line(SeraName, TEXT("...그래. 네가 먼저 말할 줄 알았다.")));
		}
		else
		{
			P->SetStoryFlag(FlagForHome);
			Lines.Add(Line(SeraName, TEXT("돌아갈 길이라. 그것도 틈한테 물어볼 수밖에 없겠지.")));
		}
		Lines.Add(Line(SeraName, TEXT("네가 떨어진 동쪽 폐허, 거기 제단에 북쪽을 가리키는 문양이 있었다.")));
		Lines.Add(Line(SeraName, TEXT("북쪽 끝에는 틈을 처음 기록한 탑이 있다. 틈을 닫는 법도, 너를 부른 이유도 거기 있을 거다.")));
		Lines.Add(Line(SeraName, TEXT("나도 간다. 틈을 재는 게 내 일이니까.")));
		Lines.Add(Line(ElderName, TEXT("...미안하구나, 얘야. 이 마을은 너를 지킬 힘이 없어.")));
		Lines.Add(Line(SeraName, TEXT("새벽에 떠난다. 자 둬라.")));
		Talk(Lines, Sera, EStep::Departure);
		break;
	}

	// ---------------- 출발 ----------------
	case EStep::Departure:
	{
		TArray<FShot> Departure;

		FShot Shot;
		Shot.Image = TEXT("Cut_05");
		Shot.Placeholder = TEXT("새벽 안개 속 마을 문. 등을 보이고 선 두 사람(주인공과 세라).\n멀리 동쪽 고갯길 위로 희미하게 뜬 바위들.");
		Shot.Lines.Add(TEXT("다음 날 새벽, 나는 마을을 떠났다."));
		Shot.Lines.Add(P->HasStoryFlag(FlagForHome)
			? TEXT("나를 부른 틈이 무엇인지, 그리고 돌아갈 길이 있는지 알기 위해.")
			: TEXT("나를 받아 준 이곳을, 나로부터 지키기 위해."));
		Shot.PanFrom = FVector2D(0.f, 20.f);
		Shot.PanTo = FVector2D(0.f, -20.f);
		Shot.ZoomFrom = 1.15f;
		Shot.ZoomTo = 1.f;
		Departure.Add(Shot);

		PlayCutscene(Departure, CutsceneDeparture, EStep::DepartureTitle);
		break;
	}

	case EStep::DepartureTitle:
		H->EndCutscene();
		H->SetLetterbox(false);
		H->SetGameplayVisible(false);
		break;

	case EStep::Escort:
		SetFreeControl(true);
		SetHour(6.f, true);
		SetObjective(TEXT("세라를 따라 동쪽 폐허로"));
		Bark(SeraName, TEXT("마을이 안 보일 때까지는 뒤돌아보지 마라. 돌아보면 발이 무거워진다."), 5.f);
		RouteIndex = 0;
		EscortWaitTime = 0.f;
		bAmbushDone = false;
		if (Sera && Route.Num() > 0)
		{
			Sera->WalkSpeed = 420.f;
			Sera->GetCharacterMovement()->MaxWalkSpeed = 420.f;
			Sera->WalkTo(Route[0]);
		}
		break;

	case EStep::AltarWait:
		SetObjective(TEXT("제단을 살펴보라"));
		break;

	case EStep::Ending:
	{
		TArray<FIsekaiDialogueLine> Lines;
		Lines.Add(Line(SeraName, TEXT("북쪽. 역시 그렇군.")));
		if (P->HasStoryFlag(FlagForHome))
		{
			Lines.Add(Line(SeraName, TEXT("돌아갈 길... 네가 찾는 게 거기 있으면 좋겠다.")));
		}
		else
		{
			Lines.Add(Line(SeraName, TEXT("마을에서 멀어질수록 놈들도 마을을 잊는다. 이제 앞만 보면 된다.")));
		}
		Lines.Add(Line(SeraName, TEXT("가자. 해가 지기 전에 고개 아래까지는 내려가야 한다.")));
		Talk(Lines, Sera, EStep::EndTitle);
		break;
	}

	case EStep::EndTitle:
		H->FadeTo(FLinearColor::Black, 1.f, 1.5f);
		SetMusic(nullptr, 1.f, 1.5f);
		break;

	case EStep::Done:
		SetFreeControl(true);
		SetObjective(TEXT("다음 이야기는 준비 중  ·  자유롭게 둘러보기"));
		SetMusic(TEXT("MUS_Ambient"), 0.5f, 3.f);
		SetActorTickEnabled(false);
		break;

	default:
		break;
	}
}

// ===========================================================================
// 매 프레임
// ===========================================================================

void AIsekaiStoryDirector::HandleGolemAttack(bool bHit)
{
	if (Step != EStep::Lesson2Dodge)
	{
		return;
	}

	if (bHit)
	{
		Bark(SeraName, TEXT("늦다. 눈이 붉어지는 순간에 움직여라."), 3.f);
		return;
	}

	++DodgeCount;
	SetObjective(*FString::Printf(TEXT("연습 상대의 공격을 피하라  %d / 3"), DodgeCount));
	if (DodgeCount >= 3)
	{
		Go(EStep::Lesson2Defeat);
	}
}

void AIsekaiStoryDirector::Tick(float DeltaSeconds)
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
	// ---------------- 공용 ----------------
	case EStep::Talk:
		LookAt(TalkLookAt.Get(), RealDelta);
		if (!P->GetDialogue()->IsDialogueActive())
		{
			Go(NextStep);
		}
		break;

	case EStep::Transition:
		if (!bFlagA && StepTime > 1.1f)
		{
			bFlagA = true;
			if (!TransitionCaption.IsEmpty())
			{
				H->SetNarration(FText::FromString(TransitionCaption));
			}
			if (TransitionHour >= 0.f)
			{
				SetHour(TransitionHour, false);
			}
		}
		// 문구가 없으면 잠깐 눈을 감았다 뜨는 정도로 짧게.
		if (StepTime > (TransitionCaption.IsEmpty() ? 1.4f : 3.4f))
		{
			H->SetNarration(FText::GetEmpty());
			H->FadeTo(FLinearColor::Black, 0.f, 1.2f);
			Go(NextStep);
		}
		break;

	case EStep::Wait:
		if (StepTime > WaitSeconds)
		{
			Go(NextStep);
		}
		break;

	case EStep::Cutscene:
		TickCutscene(RealDelta);
		break;

	// ---------------- 집 ----------------
	case EStep::Wake:
		// 천장을 보다가 세라 쪽으로 고개를 돌린다.
		if (StepTime > 3.2f)
		{
			LookAt(Sera, RealDelta);
		}
		if (StepTime > 4.6f)
		{
			TArray<FIsekaiDialogueLine> Lines;
			Lines.Add(Line(SeraName, TEXT("깼나. 사흘을 내리 잤다.")));
			Lines.Add(Line(Me, TEXT("여기는... 저는 분명히, 길을 건너고 있었는데.")));
			Lines.Add(Line(SeraName, TEXT("동쪽 폐허에서 너를 주웠다. 하늘에 금이 가더니, 거기서 네가 떨어지더군.")));
			FIsekaiDialogueLine Question = Line(SeraName, TEXT("나는 세라. 틈을 재고 다니는 측량사다. ...그래서, 너는 어디서 왔지?"));
			Question.Choices.Add(Txt(TEXT("다른 세계에서 왔습니다. 죽었다고 생각했는데, 눈을 뜨니 하늘이었어요.")));
			Question.Choices.Add(Txt(TEXT("...기억이 나지 않습니다.")));
			Lines.Add(Question);
			Talk(Lines, Sera, EStep::WakeReply);
		}
		break;

	case EStep::GoToYard:
		if (FVector::Dist2D(P->GetActorLocation(), Sera->GetActorLocation()) < 700.f)
		{
			Go(EStep::Lesson1Intro);
		}
		break;

	case EStep::Lesson1:
	{
		int32 Broken = 0;
		for (const TWeakObjectPtr<AIsekaiTargetDummy>& Crystal : Crystals)
		{
			if (!Crystal.IsValid() || Crystal->IsBroken())
			{
				++Broken;
			}
		}
		SetObjective(*FString::Printf(TEXT("수정을 깨라  %d / 3"), Broken));
		if (Broken >= 3)
		{
			for (const TWeakObjectPtr<AIsekaiTargetDummy>& Crystal : Crystals)
			{
				if (Crystal.IsValid())
				{
					Crystal->SetLifeSpan(0.1f);
				}
			}
			WaitThen(1.f, EStep::Lesson1Done);
		}
		break;
	}

	case EStep::Lesson2Defeat:
		if (!Golem.IsValid() || Golem->IsDefeated())
		{
			WaitThen(1.f, EStep::Lesson2Done);
		}
		break;

	case EStep::Lesson3:
	{
		int32 Lit = 0;
		for (const TWeakObjectPtr<AIsekaiBrazier>& Brazier : Braziers)
		{
			if (Brazier.IsValid() && Brazier->IsLit())
			{
				++Lit;
			}
		}
		SetObjective(*FString::Printf(TEXT("화로에 불을 옮겨라  %d / 3"), Lit));
		if (Lit >= 3)
		{
			WaitThen(1.2f, EStep::Lesson3Done);
		}
		break;
	}

	// ---------------- 습격 ----------------
	case EStep::AttackStart:
		if (StepTime > 1.f && CountLiving() == 0)
		{
			WaitThen(1.5f, EStep::AttackElite);
		}
		break;

	case EStep::AttackElite:
		if (StepTime > 1.f && (!Elite.IsValid() || Elite->IsDefeated()))
		{
			Transition(nullptr, 22.f, EStep::Aftermath);
		}
		break;

	// ---------------- 출발 ----------------
	case EStep::DepartureTitle:
		if (!bFlagA && StepTime > 0.6f)
		{
			bFlagA = true;
			H->SetTitle(Txt(TEXT("ISEKAI")), Txt(TEXT("1장   ·   북쪽으로")));
			IsekaiAudio::Play(this, TEXT("SFX_Title"), 1.f);
		}
		if (StepTime > 5.2f && StepTime - RealDelta <= 5.2f)
		{
			H->SetTitle(FText::GetEmpty(), FText::GetEmpty());
		}
		if (StepTime > 6.6f)
		{
			PlacePlayer(VillageGate - FVector(300.f, 0.f, 0.f), VillageGate + FVector(2000.f, 0.f, 0.f));
			PlaceNPC(Sera, VillageGate + FVector(0.f, 250.f, 0.f), VillageGate + FVector(2000.f, 0.f, 0.f));
			P->Rest();
			H->FadeTo(FLinearColor::Black, 0.f, 2.f);
			Go(EStep::Escort);
		}
		break;

	case EStep::Escort:
		if (UpdateEscort(RealDelta))
		{
			Sera->StopWalking();
			Go(EStep::AltarWait);
			break;
		}

		// 고갯길 한가운데에 들어서면 매복이 터진다.
		if (!bAmbushDone && Route.IsValidIndex(PassBottomIndex) && Route.IsValidIndex(PassTopIndex))
		{
			const FVector Middle = (Route[PassBottomIndex] + Route[PassTopIndex]) * 0.5;
			if (FVector::Dist2D(P->GetActorLocation(), Middle) < 1200.f)
			{
				StartAmbush();
			}
		}
		break;

	case EStep::Ambush:
		if (StepTime > 1.f && CountLiving() == 0)
		{
			SetMusic(TEXT("MUS_Ambient"), 0.6f, 2.f);
			Bark(SeraName, TEXT("...역시 너를 따라오는군. 서두르자."), 4.f);
			SetObjective(TEXT("세라를 따라 동쪽 폐허로"));
			Step = EStep::Escort;
			StepTime = 0.f;
		}
		break;

	case EStep::AltarWait:
		if (Altar && Altar->WasExamined() && !P->GetDialogue()->IsDialogueActive())
		{
			WaitThen(0.6f, EStep::Ending);
		}
		break;

	case EStep::EndTitle:
		if (!bFlagA && StepTime > 1.8f)
		{
			bFlagA = true;
			H->SetTitle(Txt(TEXT("1장   ·   북쪽으로")), Txt(TEXT("— 다음 이야기는 준비 중 —")));
			IsekaiAudio::Play(this, TEXT("SFX_Title"), 1.f);
		}
		if (StepTime > 6.5f && StepTime - RealDelta <= 6.5f)
		{
			H->SetTitle(FText::GetEmpty(), FText::GetEmpty());
		}
		if (StepTime > 8.f)
		{
			H->FadeTo(FLinearColor::Black, 0.f, 2.f);
			Go(EStep::Done);
		}
		break;

	default:
		break;
	}
}

// ===========================================================================
// 길 안내와 매복
// ===========================================================================

bool AIsekaiStoryDirector::UpdateEscort(float RealDelta)
{
	AIsekaiCharacter* P = Player.Get();
	if (!Sera || Route.Num() == 0)
	{
		return true;
	}

	const double Gap = FVector::Dist2D(P->GetActorLocation(), Sera->GetActorLocation());

	if (RouteIndex >= Route.Num())
	{
		return Gap < 900.0;
	}

	// 플레이어가 너무 뒤처지면 멈춰서 기다린다.
	if (Gap > 1500.0)
	{
		Sera->StopWalking();
		EscortWaitTime += RealDelta;
		if (EscortWaitTime > 7.f)
		{
			Bark(SeraName, TEXT("이쪽이다. 떨어지지 마라."), 3.f);
			EscortWaitTime = 0.f;
		}
		return false;
	}
	EscortWaitTime = 0.f;

	if (!Sera->IsWalking())
	{
		if (FVector::Dist2D(Sera->GetActorLocation(), Route[RouteIndex]) < 220.0)
		{
			OnWaypointReached(RouteIndex);
			++RouteIndex;
		}

		if (RouteIndex < Route.Num())
		{
			Sera->WalkTo(Route[RouteIndex]);
		}
	}

	return false;
}

void AIsekaiStoryDirector::OnWaypointReached(int32 Index)
{
	// 길의 칸마다 한마디씩. 걷는 시간이 비지 않게 한다.
	if (Index == 2)
	{
		Bark(SeraName, TEXT("숲은 등불만 따라가면 된다. 저 등불을 세운 게 나 같은 측량사들이다."), 5.f);
	}
	else if (Index == 3)
	{
		Bark(SeraName, TEXT("네 마력, 수련 전보다 훨씬 크게 울린다. 놈들도 들었겠지."), 5.f);
	}
	else if (Index == PassBottomIndex)
	{
		Bark(SeraName, TEXT("고갯길이다. 좁고 숨을 데가 없다. 붙어서 와라."), 5.f);
	}
	else if (Index == Route.Num() - 2)
	{
		Bark(SeraName, TEXT("저기다. 네가 떨어진 곳. ...돌이 또 떠올랐군. 네가 가까이 오니까."), 5.f);
	}
	else if (Index == Route.Num() - 1)
	{
		Bark(SeraName, TEXT("제단을 봐라. 그날 내가 본 문양이 아직 있을 거다."), 5.f);
	}
}

void AIsekaiStoryDirector::StartAmbush()
{
	AIsekaiCharacter* P = Player.Get();
	bAmbushDone = true;

	const FVector Bottom = Route[PassBottomIndex];
	const FVector Top = Route[PassTopIndex];
	const FVector Uphill = (Top - Bottom).GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Uphill);

	Sera->StopWalking();
	Enemies.Reset();

	// 위쪽 출구에 쏘는 놈 둘. 좁은 오르막에서 날아오는 탄을 피해야 한다.
	const float ShooterOffsets[] = { -250.f, 250.f };
	for (const float Offset : ShooterOffsets)
	{
		if (AIsekaiEnemy* Shooter = SpawnEnemy(Top + Uphill * 500.f + Side * Offset, Bottom))
		{
			Shooter->MakeRanged();
			Shooter->StartChasing();
		}
	}

	// 아래쪽 입구에 덤비는 놈 둘. 물러날 길을 막는다.
	const float BrawlerOffsets[] = { -200.f, 200.f };
	for (const float Offset : BrawlerOffsets)
	{
		if (AIsekaiEnemy* Brawler = SpawnEnemy(Bottom - Uphill * 700.f + Side * Offset, Top))
		{
			Brawler->StartChasing();
		}
	}

	Bark(SeraName, TEXT("매복이다! 날아오는 건 피해라!"), 4.f);
	SetObjective(TEXT("매복을 뚫어라"));
	P->AddCameraShake(0.6f);
	IsekaiAudio::Play(this, TEXT("SFX_Land"), 0.6f);
	SetMusic(TEXT("MUS_Combat"), 0.7f, 1.f);

	Step = EStep::Ambush;
	StepTime = 0.f;
}
