#include "CoreMinimal.h"

// 첫 구역의 그레이박스를 에디터 레벨 안에 만들어 주는 콘솔 명령.
// 에디터 아래쪽 Cmd 칸에 Isekai.BuildGreybox 를 입력하면 실행된다.
//
// 만든 액터에는 모두 IsekaiGreybox 태그가 붙는다. 다시 실행하면 그 태그가 붙은
// 액터를 전부 지우고 새로 만든다. 직접 놓은 액터는 건드리지 않는다.

#if WITH_EDITOR

#include "Combat/IsekaiEnemy.h"
#include "Combat/IsekaiTargetDummy.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dialogue/IsekaiExaminable.h"
#include "Dialogue/IsekaiNPC.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/IConsoleManager.h"
#include "Story/IsekaiPrologueDirector.h"
#include "Math/RandomStream.h"
#include "World/IsekaiCampfire.h"
#include "World/IsekaiFloatingStone.h"
#include "World/IsekaiGravityAnomaly.h"

namespace IsekaiGreybox
{
	const FName BuildTag(TEXT("IsekaiGreybox"));

	/** 1미터 = 100 언리얼 단위 */
	const double Meter = 100.0;

	/** 고갯길을 올라간 뒤의 고지대 높이 */
	const double HighlandZ = 5.0 * Meter;

	/** 고지대가 시작되는 X 좌표 */
	const double HighlandStartX = 560.0 * Meter;

	/** 폐허 단상의 윗면 높이 */
	const double DaisTopZ = HighlandZ + 2.0 * Meter;

	const FVector RuinsCenter(740.0 * Meter, 0.0, 0.0);

	struct FBuilder
	{
		UWorld* World = nullptr;
		UStaticMesh* Cube = nullptr;
		UStaticMesh* Cylinder = nullptr;
		UStaticMesh* Cone = nullptr;
		int32 Count = 0;

		template <typename ActorType>
		ActorType* Spawn(const FVector& Location, const FRotator& Rotation, const FString& Label, const TCHAR* Folder)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ActorType* Actor = World->SpawnActor<ActorType>(ActorType::StaticClass(), Location, Rotation, Params);
			if (Actor)
			{
				Actor->Tags.Add(BuildTag);
				Actor->SetActorLabel(Label);
				Actor->SetFolderPath(FName(Folder));
				++Count;
			}
			return Actor;
		}

		/** 기본 도형 하나를 놓는다. Size는 cm 단위의 실제 크기다. */
		AStaticMeshActor* AddShape(UStaticMesh* Shape, const FVector& Center, const FVector& Size,
			const FRotator& Rotation, const FString& Label, const TCHAR* Folder)
		{
			AStaticMeshActor* Actor = Spawn<AStaticMeshActor>(Center, Rotation, Label, Folder);
			if (Actor)
			{
				Actor->GetStaticMeshComponent()->SetStaticMesh(Shape);
				Actor->SetActorScale3D(Size / 100.0);
			}
			return Actor;
		}

		AStaticMeshActor* AddBox(const FVector& Center, const FVector& Size, const FRotator& Rotation,
			const FString& Label, const TCHAR* Folder)
		{
			return AddShape(Cube, Center, Size, Rotation, Label, Folder);
		}

		/**
		 * 두 점을 잇는 판을 놓는다. 두 점은 판 윗면의 양 끝 가운데다.
		 * 높이가 다르면 경사로가 된다.
		 */
		void AddSlab(const FVector& TopStart, const FVector& TopEnd, double Width, double Thickness,
			const FString& Label, const TCHAR* Folder)
		{
			const FVector Along = TopEnd - TopStart;
			const FRotator Rotation = Along.Rotation();
			const FVector Up = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Z);
			const FVector Center = (TopStart + TopEnd) * 0.5 - Up * (Thickness * 0.5);

			AddBox(Center, FVector(Along.Size(), Width, Thickness), Rotation, Label, Folder);
		}

		void AddTree(const FVector& Base, double TrunkHeight, double CanopyHeight, double CanopyWidth, int32 Index)
		{
			AddShape(Cylinder, Base + FVector(0.0, 0.0, TrunkHeight * 0.5), FVector(60.0, 60.0, TrunkHeight),
				FRotator::ZeroRotator, FString::Printf(TEXT("Tree_%03d_Trunk"), Index), TEXT("Greybox/Trees"));

			AddShape(Cone, Base + FVector(0.0, 0.0, TrunkHeight + CanopyHeight * 0.5 - 100.0),
				FVector(CanopyWidth, CanopyWidth, CanopyHeight),
				FRotator::ZeroRotator, FString::Printf(TEXT("Tree_%03d_Canopy"), Index), TEXT("Greybox/Trees"));
		}

		void AddHouse(const FVector& Base, double Yaw, int32 Index)
		{
			const FRotator Rotation(0.0, Yaw, 0.0);

			AddBox(Base + FVector(0.0, 0.0, 200.0), FVector(800.0, 600.0, 400.0), Rotation,
				FString::Printf(TEXT("House_%02d_Body"), Index), TEXT("Greybox/Village"));

			// 지붕은 몸통보다 조금 넓고 낮은 상자로 표시만 한다.
			AddBox(Base + FVector(0.0, 0.0, 475.0), FVector(900.0, 700.0, 150.0), Rotation,
				FString::Printf(TEXT("House_%02d_Roof"), Index), TEXT("Greybox/Village"));
		}

		void AddLantern(const FVector& Base, int32 Index)
		{
			AddShape(Cylinder, Base + FVector(0.0, 0.0, 150.0), FVector(15.0, 15.0, 300.0),
				FRotator::ZeroRotator, FString::Printf(TEXT("Lantern_%02d_Post"), Index), TEXT("Greybox/Lanterns"));

			APointLight* Light = Spawn<APointLight>(Base + FVector(0.0, 0.0, 320.0), FRotator::ZeroRotator,
				FString::Printf(TEXT("Lantern_%02d_Light"), Index), TEXT("Greybox/Lanterns"));
			if (Light && Light->GetLightComponent())
			{
				ULightComponent* LightComponent = Light->GetLightComponent();
				LightComponent->SetMobility(EComponentMobility::Movable);
				LightComponent->SetIntensity(5000.f);
				LightComponent->SetLightColor(FLinearColor(1.f, 0.75f, 0.45f));

				if (UPointLightComponent* PointComponent = Cast<UPointLightComponent>(LightComponent))
				{
					PointComponent->SetAttenuationRadius(1200.f);
				}
			}
		}

		AIsekaiNPC* AddNPC(const FVector& GroundLocation, const FVector& LookAt, const TCHAR* Name,
			const TArray<TPair<FString, FString>>& InLines)
		{
			const FVector Location = GroundLocation + FVector(0.0, 0.0, 90.0);
			const FRotator Rotation(0.0, (LookAt - Location).Rotation().Yaw, 0.0);

			AIsekaiNPC* NPC = Spawn<AIsekaiNPC>(Location, Rotation, FString::Printf(TEXT("NPC_%s"), Name), TEXT("Greybox/Gameplay"));
			if (NPC)
			{
				NPC->DisplayName = FText::FromString(FString(Name));
				for (const TPair<FString, FString>& Entry : InLines)
				{
					FIsekaiDialogueLine Line;
					Line.Speaker = FText::FromString(Entry.Key);
					Line.Text = FText::FromString(Entry.Value);
					NPC->Lines.Add(Line);
				}
			}
			return NPC;
		}

		void AddEnemy(const FVector& GroundLocation, const FVector& LookAt, int32 Index)
		{
			const FVector Location = GroundLocation + FVector(0.0, 0.0, 90.0);
			const FRotator Rotation(0.0, (LookAt - Location).Rotation().Yaw, 0.0);
			Spawn<AIsekaiEnemy>(Location, Rotation, FString::Printf(TEXT("Enemy_%02d"), Index), TEXT("Greybox/Gameplay"));
		}
	};

	/** 길의 꺾이는 지점들. 마을 → 숲 → 공터 → 고갯길 → 폐허. */
	TArray<FVector> MakePath()
	{
		TArray<FVector> Path;
		Path.Add(FVector(0.0, 0.0, 0.0));                                   // 0 마을 마당
		Path.Add(FVector(60.0 * Meter, 0.0, 0.0));                          // 1 마을 문
		Path.Add(FVector(140.0 * Meter, 40.0 * Meter, 0.0));                // 2
		Path.Add(FVector(220.0 * Meter, -30.0 * Meter, 0.0));               // 3 숲
		Path.Add(FVector(310.0 * Meter, 20.0 * Meter, 0.0));                // 4 공터 (첫 조우)
		Path.Add(FVector(400.0 * Meter, -40.0 * Meter, 0.0));               // 5
		Path.Add(FVector(480.0 * Meter, 0.0, 0.0));                         // 6 고갯길 입구
		Path.Add(FVector(HighlandStartX, 0.0, HighlandZ));                  // 7 고갯길 출구 (시야가 열린다)
		Path.Add(FVector(650.0 * Meter, 50.0 * Meter, HighlandZ));          // 8
		Path.Add(FVector(705.0 * Meter, 0.0, HighlandZ));                   // 9 폐허 앞
		return Path;
	}

	double DistanceToPath(const TArray<FVector>& Path, const FVector& Point)
	{
		const FVector Flat(Point.X, Point.Y, 0.0);
		double Best = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
		{
			const FVector A(Path[Index].X, Path[Index].Y, 0.0);
			const FVector B(Path[Index + 1].X, Path[Index + 1].Y, 0.0);
			Best = FMath::Min(Best, static_cast<double>(FMath::PointDistToSegment(Flat, A, B)));
		}
		return Best;
	}

	void ClearPrevious(UWorld* World)
	{
		TArray<AActor*> ToDestroy;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			const bool bOurs = Actor->Tags.Contains(BuildTag);

			// 기본 템플릿 레벨의 바닥은 우리 지면과 겹치므로 지운다.
			const bool bTemplateFloor = Actor->IsA<AStaticMeshActor>() && Actor->GetActorLabel() == TEXT("Floor");

			if (bOurs || bTemplateFloor)
			{
				ToDestroy.Add(Actor);
			}
		}

		for (AActor* Actor : ToDestroy)
		{
			World->DestroyActor(Actor);
		}
	}

	void Build(UWorld* World)
	{
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Isekai.BuildGreybox: stop Play first. This command only works in the editor level."));
			return;
		}

		FBuilder Builder;
		Builder.World = World;
		Builder.Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Builder.Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		Builder.Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

		if (!Builder.Cube || !Builder.Cylinder || !Builder.Cone)
		{
			UE_LOG(LogTemp, Error, TEXT("Isekai.BuildGreybox: could not load the engine basic shapes."));
			return;
		}

		ClearPrevious(World);

		const TArray<FVector> Path = MakePath();
		FRandomStream Random(20261114);

		// ---------- 지면 ----------

		// 저지대. X -200m ~ 1200m, Y -500m ~ 500m, 윗면이 높이 0.
		Builder.AddBox(FVector(500.0 * Meter, 0.0, -50.0), FVector(1400.0 * Meter, 1000.0 * Meter, 100.0),
			FRotator::ZeroRotator, TEXT("Ground_Lowland"), TEXT("Greybox/Ground"));

		// 고지대. 고갯길 너머는 5m 높다.
		Builder.AddBox(FVector(830.0 * Meter, 0.0, HighlandZ * 0.5), FVector(540.0 * Meter, 1000.0 * Meter, HighlandZ),
			FRotator::ZeroRotator, TEXT("Ground_Highland"), TEXT("Greybox/Ground"));

		// 세계의 끝에서 떨어지지 않게 둘러싼 벽.
		Builder.AddBox(FVector(-200.0 * Meter, 0.0, 1000.0), FVector(200.0, 1000.0 * Meter, 2000.0),
			FRotator::ZeroRotator, TEXT("Boundary_West"), TEXT("Greybox/Ground"));
		Builder.AddBox(FVector(1100.0 * Meter, 0.0, 1000.0), FVector(200.0, 1000.0 * Meter, 3000.0),
			FRotator::ZeroRotator, TEXT("Boundary_East"), TEXT("Greybox/Ground"));
		Builder.AddBox(FVector(450.0 * Meter, 500.0 * Meter, 1000.0), FVector(1300.0 * Meter, 200.0, 3000.0),
			FRotator::ZeroRotator, TEXT("Boundary_North"), TEXT("Greybox/Ground"));
		Builder.AddBox(FVector(450.0 * Meter, -500.0 * Meter, 1000.0), FVector(1300.0 * Meter, 200.0, 3000.0),
			FRotator::ZeroRotator, TEXT("Boundary_South"), TEXT("Greybox/Ground"));

		// ---------- 길 ----------

		for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
		{
			const FVector& A = Path[Index];
			const FVector& B = Path[Index + 1];

			// 높이가 바뀌는 구간은 넓은 경사로를 먼저 깐다.
			if (!FMath::IsNearlyEqual(A.Z, B.Z))
			{
				Builder.AddSlab(A, B, 1400.0, 100.0, FString::Printf(TEXT("Ramp_%02d"), Index), TEXT("Greybox/Ground"));
			}

			// 길 표시. 지면보다 6cm 높은 얇은 판.
			const FVector Lift(0.0, 0.0, 6.0);
			Builder.AddSlab(A + Lift, B + Lift, 500.0, 6.0, FString::Printf(TEXT("Path_%02d"), Index), TEXT("Greybox/Path"));
		}

		// 길을 따라 45m마다 등불을 세운다. 밤에 길잡이가 된다.
		{
			int32 LanternIndex = 0;
			double Carry = 20.0 * Meter;
			for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
			{
				const FVector A = Path[Index];
				const FVector B = Path[Index + 1];
				const double Length = (B - A).Size();
				const FVector Direction = (B - A) / Length;
				const FVector Side = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();

				double Along = Carry;
				while (Along < Length)
				{
					Builder.AddLantern(A + Direction * Along + Side * 400.0, LanternIndex++);
					Along += 45.0 * Meter;
				}
				Carry = Along - Length;
			}
		}

		// ---------- 마을 외곽 ----------

		Builder.AddHouse(FVector(-15.0 * Meter, 22.0 * Meter, 0.0), 10.0, 0);
		Builder.AddHouse(FVector(8.0 * Meter, -30.0 * Meter, 0.0), -5.0, 1);
		Builder.AddHouse(FVector(32.0 * Meter, 26.0 * Meter, 0.0), 80.0, 2);
		Builder.AddHouse(FVector(-28.0 * Meter, -16.0 * Meter, 0.0), 95.0, 3);
		Builder.AddHouse(FVector(48.0 * Meter, -24.0 * Meter, 0.0), 15.0, 4);

		// 마을 문. 기둥 둘과 가로대.
		Builder.AddShape(Builder.Cylinder, FVector(60.0 * Meter, 400.0, 250.0), FVector(50.0, 50.0, 500.0),
			FRotator::ZeroRotator, TEXT("Gate_Post_L"), TEXT("Greybox/Village"));
		Builder.AddShape(Builder.Cylinder, FVector(60.0 * Meter, -400.0, 250.0), FVector(50.0, 50.0, 500.0),
			FRotator::ZeroRotator, TEXT("Gate_Post_R"), TEXT("Greybox/Village"));
		Builder.AddBox(FVector(60.0 * Meter, 0.0, 520.0), FVector(60.0, 950.0, 40.0),
			FRotator::ZeroRotator, TEXT("Gate_Lintel"), TEXT("Greybox/Village"));

		// 마을 울타리. 문 양옆으로 이어진다.
		Builder.AddBox(FVector(60.0 * Meter, 30.0 * Meter, 60.0), FVector(30.0, 51.0 * Meter, 120.0),
			FRotator::ZeroRotator, TEXT("Fence_L"), TEXT("Greybox/Village"));
		Builder.AddBox(FVector(60.0 * Meter, -30.0 * Meter, 60.0), FVector(30.0, 51.0 * Meter, 120.0),
			FRotator::ZeroRotator, TEXT("Fence_R"), TEXT("Greybox/Village"));

		// ---------- 숲 ----------

		{
			int32 TreeIndex = 0;
			int32 Attempts = 0;
			while (TreeIndex < 220 && Attempts < 5000)
			{
				++Attempts;

				const FVector Base(Random.FRandRange(70.f, 540.f) * Meter, Random.FRandRange(-220.f, 220.f) * Meter, 0.0);

				const bool bOnPath = DistanceToPath(Path, Base) < 9.0 * Meter;
				const bool bInClearing = FVector::Dist2D(Base, Path[4]) < 24.0 * Meter;
				const bool bInPass = Base.X > 455.0 * Meter && FMath::Abs(Base.Y) < 50.0 * Meter;
				if (bOnPath || bInClearing || bInPass)
				{
					continue;
				}

				Builder.AddTree(Base, Random.FRandRange(400.f, 700.f), Random.FRandRange(600.f, 900.f),
					Random.FRandRange(400.f, 600.f), TreeIndex++);
			}

			// 고지대의 나무는 드문드문. 폐허가 멀리서 보여야 한다.
			Attempts = 0;
			const int32 HighlandTreeEnd = TreeIndex + 50;
			while (TreeIndex < HighlandTreeEnd && Attempts < 3000)
			{
				++Attempts;

				const FVector Base(Random.FRandRange(585.f, 1000.f) * Meter, Random.FRandRange(-250.f, 250.f) * Meter, HighlandZ);

				const bool bOnPath = DistanceToPath(Path, Base) < 14.0 * Meter;
				const bool bNearRuins = FVector::Dist2D(Base, RuinsCenter) < 50.0 * Meter;
				if (bOnPath || bNearRuins)
				{
					continue;
				}

				Builder.AddTree(Base, Random.FRandRange(400.f, 700.f), Random.FRandRange(600.f, 900.f),
					Random.FRandRange(400.f, 600.f), TreeIndex++);
			}
		}

		// 바위. 길가의 엄폐물이자 시선을 끊는 덩어리.
		{
			int32 RockIndex = 0;
			int32 Attempts = 0;
			while (RockIndex < 50 && Attempts < 2000)
			{
				++Attempts;

				const FVector Base(Random.FRandRange(80.f, 450.f) * Meter, Random.FRandRange(-120.f, 120.f) * Meter, 0.0);
				const double PathDistance = DistanceToPath(Path, Base);
				if (PathDistance < 6.0 * Meter || PathDistance > 40.0 * Meter)
				{
					continue;
				}

				const double Size = Random.FRandRange(120.f, 450.f);
				const FRotator Rotation(Random.FRandRange(-15.f, 15.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-15.f, 15.f));
				Builder.AddBox(Base + FVector(0.0, 0.0, Size * 0.3), FVector(Size * 1.3, Size, Size * 0.8), Rotation,
					FString::Printf(TEXT("Rock_%02d"), RockIndex++), TEXT("Greybox/Rocks"));
			}
		}

		// ---------- 고갯길 ----------

		// 경사로 양옆을 높은 절벽으로 막아 시야를 닫는다. 출구에서 한 번에 열린다.
		Builder.AddBox(FVector(520.0 * Meter, 27.0 * Meter, 700.0), FVector(120.0 * Meter, 40.0 * Meter, 1400.0),
			FRotator::ZeroRotator, TEXT("Cliff_L"), TEXT("Greybox/Pass"));
		Builder.AddBox(FVector(520.0 * Meter, -27.0 * Meter, 700.0), FVector(120.0 * Meter, 40.0 * Meter, 1400.0),
			FRotator::ZeroRotator, TEXT("Cliff_R"), TEXT("Greybox/Pass"));

		// 절벽 윗선이 일직선이 되지 않도록 덩어리를 얹는다.
		Builder.AddBox(FVector(500.0 * Meter, 30.0 * Meter, 1600.0), FVector(50.0 * Meter, 30.0 * Meter, 600.0),
			FRotator(0.0, 12.0, 4.0), TEXT("Cliff_L_Top"), TEXT("Greybox/Pass"));
		Builder.AddBox(FVector(540.0 * Meter, -32.0 * Meter, 1700.0), FVector(60.0 * Meter, 28.0 * Meter, 800.0),
			FRotator(0.0, -9.0, -5.0), TEXT("Cliff_R_Top"), TEXT("Greybox/Pass"));

		// ---------- 폐허와 이상현상 ----------

		// 단상. 고지대보다 2m 높다.
		Builder.AddBox(FVector(RuinsCenter.X, 0.0, HighlandZ + 100.0), FVector(50.0 * Meter, 50.0 * Meter, 200.0),
			FRotator::ZeroRotator, TEXT("Ruins_Dais"), TEXT("Greybox/Ruins"));

		// 단상으로 오르는 경사로.
		Builder.AddSlab(FVector(705.0 * Meter, 0.0, HighlandZ), FVector(715.0 * Meter, 0.0, DaisTopZ), 1000.0, 100.0,
			TEXT("Ruins_Ramp"), TEXT("Greybox/Ruins"));

		// 단상을 둘러싼 기둥 10개. 높이가 제각각인 것은 부러졌다는 표시다.
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const double Angle = FMath::DegreesToRadians(18.0 + 36.0 * Index);
			const double Height = Random.FRandRange(350.f, 950.f);
			const FVector Base(RuinsCenter.X + FMath::Cos(Angle) * 2000.0, FMath::Sin(Angle) * 2000.0, DaisTopZ);

			Builder.AddShape(Builder.Cylinder, Base + FVector(0.0, 0.0, Height * 0.5), FVector(150.0, 150.0, Height),
				FRotator::ZeroRotator, FString::Printf(TEXT("Ruins_Column_%02d"), Index), TEXT("Greybox/Ruins"));
		}

		// 가운데의 제단. 살펴보면 첫 구역이 끝난다. 대사는 임시 내용이다.
		if (AIsekaiExaminable* Altar = Builder.Spawn<AIsekaiExaminable>(FVector(RuinsCenter.X + 600.0, 0.0, DaisTopZ + 60.0),
			FRotator::ZeroRotator, TEXT("Ruins_Altar"), TEXT("Greybox/Ruins")))
		{
			Altar->SetActorScale3D(FVector(2.0, 3.0, 1.2));
			Altar->PromptText = FText::FromString(TEXT("제단 살펴보기"));

			auto AddAltarLine = [Altar](const TCHAR* Speaker, const TCHAR* Text)
			{
				FIsekaiDialogueLine Line;
				Line.Speaker = FText::FromString(FString(Speaker));
				Line.Text = FText::FromString(FString(Text));
				Altar->Lines.Add(Line);
			};

			AddAltarLine(TEXT(""), TEXT("제단 위의 공기가 일그러져 있다. 손을 뻗자 손끝부터 가벼워진다."));
			AddAltarLine(TEXT("나"), TEXT("...이 감각. 내가 이쪽으로 넘어오던 순간과 같아."));
			AddAltarLine(TEXT(""), TEXT("돌 표면에 낯선 문자가 새겨져 있다. 같은 문양이 북쪽을 가리키고 있다."));
		}

		// 무너진 벽.
		Builder.AddBox(FVector(RuinsCenter.X + 1200.0, 900.0, DaisTopZ + 150.0), FVector(600.0, 60.0, 300.0),
			FRotator(0.0, 25.0, 0.0), TEXT("Ruins_Wall_A"), TEXT("Greybox/Ruins"));
		Builder.AddBox(FVector(RuinsCenter.X - 300.0, -1300.0, DaisTopZ + 100.0), FVector(700.0, 60.0, 200.0),
			FRotator(0.0, -15.0, 0.0), TEXT("Ruins_Wall_B"), TEXT("Greybox/Ruins"));
		Builder.AddBox(FVector(RuinsCenter.X + 900.0, -800.0, DaisTopZ + 40.0), FVector(400.0, 60.0, 250.0),
			FRotator(0.0, 40.0, 78.0), TEXT("Ruins_Wall_Fallen"), TEXT("Greybox/Ruins"));

		// 떠 있는 돌. 낮은 것부터 높은 것까지 섞어 밟고 오를 수 있게 한다.
		for (int32 Index = 0; Index < 16; ++Index)
		{
			const double Angle = Random.FRandRange(0.f, 2.f * PI);
			const double Radius = Random.FRandRange(300.f, 2600.f);
			const double Height = 180.0 + Index * 95.0 + Random.FRandRange(-40.f, 40.f);
			const FVector Location(RuinsCenter.X + FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, DaisTopZ + Height);
			const FRotator Rotation(Random.FRandRange(-20.f, 20.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-20.f, 20.f));

			AIsekaiFloatingStone* Stone = Builder.Spawn<AIsekaiFloatingStone>(Location, Rotation,
				FString::Printf(TEXT("FloatingStone_%02d"), Index), TEXT("Greybox/Anomaly"));
			if (Stone)
			{
				const double Size = Random.FRandRange(1.2f, 3.5f);
				Stone->SetActorScale3D(FVector(Size, Size * Random.FRandRange(0.7f, 1.1f), Size * Random.FRandRange(0.35f, 0.7f)));
			}
		}

		// 중력이 약해지는 지대. 단상 전체와 그 위 공간을 덮는다.
		AIsekaiGravityAnomaly* AnomalyActor = Builder.Spawn<AIsekaiGravityAnomaly>(
			FVector(RuinsCenter.X, 0.0, DaisTopZ + 1500.0), FRotator::ZeroRotator, TEXT("GravityAnomaly"), TEXT("Greybox/Anomaly"));
		if (AnomalyActor)
		{
			AnomalyActor->GetZone()->SetBoxExtent(FVector(2800.0, 2800.0, 1600.0));
		}

		// ---------- 플레이 요소 ----------

		// 프롤로그는 폐허 위 하늘에서 떨어지며 시작한다.
		const FVector DaisCenter(RuinsCenter.X, 0.0, DaisTopZ);
		const FVector StartLocation = DaisCenter + FVector(-300.0, 0.0, 26000.0);
		const FRotator StartRotation(0.0, 180.0, 0.0);

		// 시작 지점. 레벨에 이미 있으면 옮기고, 없으면 만든다.
		{
			APlayerStart* PlayerStart = nullptr;
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				PlayerStart = *It;
				break;
			}

			if (PlayerStart)
			{
				PlayerStart->SetActorLocationAndRotation(StartLocation, StartRotation);
			}
			else
			{
				Builder.Spawn<APlayerStart>(StartLocation, StartRotation, TEXT("PlayerStart"), TEXT("Greybox/Gameplay"));
			}
		}

		// 프롤로그의 마법사와 진행 담당. 마법사의 대사는 진행 담당이 직접 띄우므로 비워 둔다.
		{
			const TArray<TPair<FString, FString>> NoLines;
			AIsekaiNPC* MageActor = Builder.AddNPC(DaisCenter + FVector(-1700.0, 300.0, 0.0), DaisCenter, TEXT("마법사"), NoLines);

			AIsekaiPrologueDirector* Director = Builder.Spawn<AIsekaiPrologueDirector>(DaisCenter + FVector(0.0, 0.0, 300.0),
				FRotator::ZeroRotator, TEXT("PrologueDirector"), TEXT("Greybox/Gameplay"));
			if (Director)
			{
				Director->Mage = MageActor;
				Director->Anomaly = AnomalyActor;
				Director->DaisCenter = DaisCenter;

				// 1장에서 마법사가 걷는 길: 폐허 앞에서 고갯길을 내려가 숲의 모닥불까지.
				Director->RouteToCamp.Reset();
				Director->RouteToCamp.Add(Path[9]);
				Director->RouteToCamp.Add(Path[8]);
				Director->RouteToCamp.Add(Path[7]);
				Director->RouteToCamp.Add(Path[6]);
				Director->RouteToCamp.Add(Path[5]);
				Director->RouteToCamp.Add(Path[4]);
				Director->RouteToCamp.Add(Path[3] + FVector(1800.0, 350.0, 0.0));
				Director->PassTopIndex = 2;
				Director->PassBottomIndex = 3;
			}
		}

		// 마을 주민. 대사는 임시 내용이다.
		{
			TArray<TPair<FString, FString>> Lines;
			Lines.Emplace(TEXT(""), TEXT("고갯길 쪽으로 가려고? 요즘 그쪽 짐승들이 부쩍 사나워졌어."));
			Lines.Emplace(TEXT(""), TEXT("그래도 가겠다면 하나만 기억해. 놈들이 몸을 부풀리면 바로 뒤로 물러나. 그 직후가 빈틈이야."));
			Builder.AddNPC(FVector(50.0 * Meter, 600.0, 0.0), FVector(40.0 * Meter, 0.0, 0.0), TEXT("마을 주민"), Lines);
		}

		// 모닥불. 쉬면 회복하고, 쓰러졌을 때 여기로 돌아온다.
		// 첫 조우 직전과, 마지막 조우 직전에 하나씩 둔다.
		Builder.Spawn<AIsekaiCampfire>(Path[3] + FVector(1500.0, 0.0, 15.0), FRotator::ZeroRotator,
			TEXT("Campfire_Forest"), TEXT("Greybox/Gameplay"));
		Builder.Spawn<AIsekaiCampfire>(FVector(640.0 * Meter, 3600.0, HighlandZ + 15.0), FRotator::ZeroRotator,
			TEXT("Campfire_Highland"), TEXT("Greybox/Gameplay"));

		// 마법 연습용 허수아비.
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Builder.Spawn<AIsekaiTargetDummy>(FVector((20.0 + Index * 4.0) * Meter, -14.0 * Meter, 90.0), FRotator::ZeroRotator,
				FString::Printf(TEXT("Dummy_%02d"), Index), TEXT("Greybox/Gameplay"));
		}

		// 적은 레벨에 놓지 않는다. 이야기 진행이 장면에 맞춰 직접 내보낸다.

		// 폐허의 짐승은 프롤로그 진행에 맞춰 실행 중에 나타난다. 여기서는 놓지 않는다.

		// ---------- 마무리 ----------

		bool bHasSun = false;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			bHasSun = true;
			break;
		}
		if (!bHasSun)
		{
			UE_LOG(LogTemp, Warning, TEXT("Isekai.BuildGreybox: this level has no sun. Create the level from the Basic template (File > New Level > Basic) and run again."));
		}

		World->MarkPackageDirty();
		UE_LOG(LogTemp, Display, TEXT("Isekai.BuildGreybox: placed %d actors. Save the level with Ctrl+S."), Builder.Count);
	}

	FAutoConsoleCommandWithWorld BuildCommand(
		TEXT("Isekai.BuildGreybox"),
		TEXT("Builds the first-area greybox in the current editor level. Re-running replaces what it built before."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Build));
}

#endif // WITH_EDITOR
