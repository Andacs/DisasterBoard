#include "DBWorld.h"
#include "DBTypes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogDBWorld, Log, All);

namespace
{
	using DBColor::RGB;
	const FVector SweepDir = FVector(-1.f, -1.f, 0.f).GetSafeNormal(); // arka köşeden (+X+Y) kameraya doğru
	constexpr float BoardTopZ = 5.f;                                     // karoların üstü
}

ADBWorld::ADBWorld()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeF(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylF(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphF(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeF(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatF(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeF.Object;
	CylinderMesh = CylF.Object;
	SphereMesh = SphF.Object;
	ConeMesh = ConeF.Object;
	ShapeMaterial = MatF.Object;
}

UStaticMeshComponent* ADBWorld::Part(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm,
	const FLinearColor& Color, const FRotator& Rot, bool bCollide)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(Parent);
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(SizeCm / 100.f);
	// Her parçaya ayrı MID: paylaşılan MID'i boyamak ötekini de boyar (Sallantı dersi)
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(ShapeMaterial, this);
	Mid->SetVectorParameterValue(TEXT("Color"), Color);
	C->SetMaterial(0, Mid);
	if (bCollide)
	{
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Block);
	}
	else
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	C->RegisterComponent();
	return C;
}

USceneComponent* ADBWorld::Node(USceneComponent* Parent, const FVector& Loc)
{
	USceneComponent* N = NewObject<USceneComponent>(this);
	N->SetMobility(EComponentMobility::Movable);
	N->SetupAttachment(Parent);
	N->SetRelativeLocation(Loc);
	N->RegisterComponent();
	return N;
}

float ADBWorld::HalfDiag() const
{
	return (GridN * CellCm * 0.5f + BorderCm) * UE_SQRT_2;
}

FVector ADBWorld::CellCenter(FIntPoint Cell) const
{
	const float Half = (GridN - 1) * 0.5f;
	return FVector((Cell.X - Half) * CellCm, (Cell.Y - Half) * CellCm, BoardTopZ);
}

void ADBWorld::Build()
{
	BuildRoom();
	BuildBoard();
	BuildCalendar();
	BuildCameras();

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		It->SetActorRotation(SunRotation);
	}
	UE_LOG(LogDBWorld, Log, TEXT("DB_WORLD bina=%d"), BuildingRoots.Num());
}

void ADBWorld::BuildRoom()
{
	USceneComponent* R = RootComponent;
	const float H = 520.f;
	const float Span = 1600.f;
	Part(R, CubeMesh, FVector(0, 0, FloorZ - 1.f), FVector(Span, Span, 2.f), RGB(170, 132, 98));
	// İki duvar: kameradan bakınca tahtanın arkasında köşe oluşturur
	Part(R, CubeMesh, FVector(WallDistCm + 2.f, 0, FloorZ + H * 0.5f), FVector(4.f, Span, H), RGB(238, 224, 198));
	Part(R, CubeMesh, FVector(0, WallDistCm + 2.f, FloorZ + H * 0.5f), FVector(Span, 4.f, H), RGB(230, 212, 182));
	Part(R, CubeMesh, FVector(WallDistCm - 1.f, 0, FloorZ + 6.f), FVector(4.f, Span, 12.f), RGB(196, 154, 108));
	Part(R, CubeMesh, FVector(0, WallDistCm - 1.f, FloorZ + 6.f), FVector(Span, 4.f, 12.f), RGB(196, 154, 108));
	// Duvarda pencere (dışarısı gök mavisi)
	Part(R, CubeMesh, FVector(WallDistCm - 0.5f, -120.f, 150.f), FVector(2.f, 130.f, 100.f), RGB(150, 112, 78));
	Part(R, CubeMesh, FVector(WallDistCm - 1.2f, -120.f, 150.f), FVector(2.f, 118.f, 88.f), RGB(176, 214, 236));
	Part(R, CubeMesh, FVector(WallDistCm - 1.8f, -120.f, 150.f), FVector(2.f, 4.f, 88.f), RGB(150, 112, 78));
	Part(R, CubeMesh, FVector(WallDistCm - 1.8f, -120.f, 150.f), FVector(2.f, 118.f, 4.f), RGB(150, 112, 78));
}

void ADBWorld::BuildBoard()
{
	BoardRoot = Node(RootComponent, FVector::ZeroVector);
	USceneComponent* B = BoardRoot;

	// Masa (tahta onunla birlikte sallanır)
	const FLinearColor Wood = RGB(158, 110, 72);
	Part(B, CubeMesh, FVector(0, 0, -3.f), FVector(196.f, 196.f, 6.f), Wood);
	const float LegH = -FloorZ - 6.f;
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i & 1) ? 88.f : -88.f;
		const float Y = (i & 2) ? 88.f : -88.f;
		Part(B, CubeMesh, FVector(X, Y, -6.f - LegH * 0.5f), FVector(9.f, 9.f, LegH), Wood);
	}

	// Tahta: karton levha + karolar
	const float Size = GridN * CellCm + 2.f * BorderCm;
	Part(B, CubeMesh, FVector(0, 0, 2.f), FVector(Size, Size, 4.f), RGB(196, 154, 108), FRotator::ZeroRotator, true);
	for (int32 X = 0; X < GridN; ++X)
	{
		for (int32 Y = 0; Y < GridN; ++Y)
		{
			const bool bDark = ((X + Y) & 1) != 0;
			Part(B, CubeMesh, CellCenter(FIntPoint(X, Y)) - FVector(0, 0, 0.5f), FVector(CellCm - 2.f, CellCm - 2.f, 1.f),
				bDark ? RGB(222, 198, 156) : RGB(234, 214, 176), FRotator::ZeroRotator, true);
		}
	}

	const FDBContent& Content = FDBContent::Get();
	for (int32 i = 0; i < Content.Buildings.Num(); ++i)
	{
		BuildBuilding(i);
	}

	// Boş karolara ağaç
	const FIntPoint Trees[] = { FIntPoint(0, 0), FIntPoint(1, 2), FIntPoint(3, 3), FIntPoint(4, 4), FIntPoint(2, 4), FIntPoint(3, 0) };
	for (const FIntPoint& T : Trees)
	{
		BuildTree(B, CellCenter(T), 1.f);
	}

	// Tarama çizgisi (yalnızca taramada görünür)
	ScanBeam = Part(B, CubeMesh, FVector(0, 0, BoardTopZ + 1.f), FVector(100.f, 3.f, 2.f), RGB(110, 235, 140), FRotator(0, -45.f, 0));
	ScanBeam->SetVisibility(false);
}

void ADBWorld::BuildTree(USceneComponent* Parent, const FVector& Loc, float Scale)
{
	Part(Parent, CylinderMesh, Loc + FVector(0, 0, 3.f * Scale), FVector(2.4f, 2.4f, 6.f) * Scale, RGB(140, 100, 66));
	Part(Parent, SphereMesh, Loc + FVector(0, 0, 10.f * Scale), FVector(11.f, 11.f, 10.f) * Scale, RGB(120, 170, 100));
}

void ADBWorld::BuildBuilding(int32 Index)
{
	const FDBBuildingDef& D = FDBContent::Get().Buildings[Index];
	USceneComponent* Root = Node(BoardRoot, CellCenter(D.Cell));
	BuildingRoots.Add(Root);
	PopTime.Add(0.f);
	FallRot.Add(FRotator((Index & 1) ? 26.f : -26.f, 0.f, (Index & 2) ? 20.f : -20.f));

	if (D.bPark)
	{
		Part(Root, CubeMesh, FVector(0, 0, 1.f), FVector(CellCm - 3.f, CellCm - 3.f, 2.f), D.Body, FRotator::ZeroRotator, true);
		BuildTree(Root, FVector(-5.f, -5.f, 2.f), 0.8f);
		BuildTree(Root, FVector(6.f, 4.f, 2.f), 0.65f);
		Part(Root, CubeMesh, FVector(4.f, -6.f, 3.5f), FVector(8.f, 2.5f, 1.2f), RGB(140, 100, 66));
		BuildingHeights.Add(14.f);
		return;
	}

	const FVector S = D.Size;
	const float Base = 1.f;
	Part(Root, CubeMesh, FVector(0, 0, 0.5f), FVector(S.X + 4.f, S.Y + 4.f, 1.f), RGB(244, 232, 208));
	Part(Root, CubeMesh, FVector(0, 0, Base + S.Z * 0.5f), S, D.Body, FRotator::ZeroRotator, true);

	float Top = Base + S.Z;
	if (S.Z > 25.f)
	{
		// Düz çatı + su deposu
		Part(Root, CubeMesh, FVector(0, 0, Top + 1.5f), FVector(S.X + 2.f, S.Y + 2.f, 3.f), D.Roof, FRotator::ZeroRotator, true);
		Part(Root, CylinderMesh, FVector(3.f, 3.f, Top + 6.f), FVector(6.f, 6.f, 6.f), RGB(200, 200, 196));
		Top += 9.f;
	}
	else
	{
		// Beşik çatı: kesiti kare kutu 45° döndürülür, alt yarısı gövdeye gömülür
		const float Side = S.Y * 0.74f;
		Part(Root, CubeMesh, FVector(0, 0, Top), FVector(S.X + 3.f, Side, Side), D.Roof, FRotator(0, 0, 45.f), true);
		Top += Side * UE_SQRT_2 * 0.5f;
	}
	BuildingHeights.Add(Top);

	// Kameraya bakan iki yüz (-X ve -Y): pencereler ve kapı
	const FLinearColor Glass = RGB(72, 92, 124);
	const FLinearColor Door = RGB(120, 84, 56);
	Part(Root, CubeMesh, FVector(0, -S.Y * 0.5f - 0.3f, Base + 3.5f), FVector(4.f, 0.8f, 7.f), Door);
	const int32 Rows = FMath::Max(1, FMath::FloorToInt((S.Z - 10.f) / 9.f));
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		const float Z = (Rows == 1) ? Base + S.Z * 0.66f : Base + 12.f + Row * 9.f;
		for (int32 Col = -1; Col <= 1; Col += 2)
		{
			Part(Root, CubeMesh, FVector(-S.X * 0.5f - 0.3f, Col * S.Y * 0.24f, Z), FVector(0.8f, 4.f, 4.5f), Glass);
			if (Rows > 1 || Col == 1)
			{
				Part(Root, CubeMesh, FVector(Col * S.X * 0.24f, -S.Y * 0.5f - 0.3f, Rows == 1 ? Z : Z), FVector(4.f, 0.8f, 4.5f), Glass);
			}
		}
	}
}

void ADBWorld::BuildCalendar()
{
	const float W = CalendarSize.X;
	const float H = CalendarSize.Y;
	const float Face = WallDistCm;
	const FVector C(CalendarCenter.X, 0.f, CalendarCenter.Z);
	USceneComponent* R = RootComponent;

	const FLinearColor Paper = RGB(248, 240, 224);
	const FLinearColor Band = RGB(232, 128, 104);
	Part(R, CubeMesh, C + FVector(0, Face - 1.f, 0), FVector(W + 10.f, 2.f, H + 12.f), RGB(150, 108, 72));
	// Alttaki sayfa (Ayarlar / Emeği Geçenler buraya yazılır)
	Part(R, CubeMesh, C + FVector(0, Face - 2.5f, 0), FVector(W, 0.6f, H), Paper);
	Part(R, CubeMesh, C + FVector(0, Face - 3.1f, H * 0.5f - H * 0.09f), FVector(W, 0.4f, H * 0.18f), RGB(96, 170, 164));

	// Üstteki sayfa: menteşe üst kenarda, sayfa aşağı sarkar
	PageHinge = Node(R, C + FVector(0, Face - 4.0f, H * 0.5f));
	Part(PageHinge, CubeMesh, FVector(0, 0, -H * 0.5f), FVector(W, 0.6f, H), Paper);
	Part(PageHinge, CubeMesh, FVector(0, -0.5f, -H * 0.09f), FVector(W, 0.4f, H * 0.18f), Band);

	// Spiral halkalar
	const int32 Rings = 12;
	for (int32 i = 0; i < Rings; ++i)
	{
		const float X = -W * 0.5f + 4.f + i * (W - 8.f) / (Rings - 1);
		Part(R, CylinderMesh, C + FVector(X, Face - 4.0f, H * 0.5f + 1.f), FVector(1.6f, 1.6f, 6.f), RGB(90, 90, 96));
	}
	// Çivi
	Part(R, CylinderMesh, C + FVector(0, Face - 2.f, H * 0.5f + 8.f), FVector(1.5f, 1.5f, 4.f), RGB(90, 90, 96), FRotator(0, 0, 90.f));

	// Menteşe yönü: küçük açıda sayfanın alt ucu odaya (-Y) gitmeli, duvara değil
	PageSign = FRotator(0, 0, 30.f).RotateVector(FVector(0, 0, -1.f)).Y < 0.f ? 1.f : -1.f;
}

void ADBWorld::BuildCameras()
{
	auto Setup = [this](ACameraActor* Cam, float Fov)
	{
		UCameraComponent* CC = Cam->GetCameraComponent();
		CC->SetFieldOfView(Fov);
		CC->SetConstraintAspectRatio(false);
		CC->PostProcessSettings.bOverride_MotionBlurAmount = true;
		CC->PostProcessSettings.MotionBlurAmount = 0.f;
		CC->PostProcessBlendWeight = 1.f;
	};

	const FVector CalFront(CalendarCenter.X, WallDistCm - 4.f, CalendarCenter.Z);
	const FVector MenuLoc = CalFront - FVector(0, MenuCamDistance, 0);
	MenuCam = GetWorld()->SpawnActor<ACameraActor>(MenuLoc, (CalFront - MenuLoc).Rotation());
	Setup(MenuCam, MenuCamFov);

	const FRotator BoardRot(BoardCamPitch, BoardCamYaw, 0.f);
	BoardCam = GetWorld()->SpawnActor<ACameraActor>(BoardFocus - BoardRot.Vector() * BoardCamDistance, BoardRot);
	Setup(BoardCam, BoardCamFov);

	ApplyExposure(ExposureEV100);
}

void ADBWorld::ApplyExposure(float EV100)
{
	ExposureEV100 = EV100;
	for (ACameraActor* Cam : { MenuCam.Get(), BoardCam.Get() })
	{
		if (!Cam)
		{
			continue;
		}
		FPostProcessSettings& PP = Cam->GetCameraComponent()->PostProcessSettings;
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMinBrightness = EV100;
		PP.AutoExposureMaxBrightness = EV100;
	}
}

FVector ADBWorld::MarkerAnchor(int32 Building) const
{
	if (!BuildingRoots.IsValidIndex(Building))
	{
		return FVector::ZeroVector;
	}
	return BuildingRoots[Building]->GetComponentTransform().TransformPosition(FVector(0, 0, BuildingHeights[Building] + 12.f));
}

int32 ADBWorld::BuildingFromComponent(const UPrimitiveComponent* Comp) const
{
	if (!Comp)
	{
		return -1;
	}
	for (int32 i = 0; i < BuildingRoots.Num(); ++i)
	{
		if (Comp->IsAttachedTo(BuildingRoots[i]))
		{
			return i;
		}
	}
	return -1;
}

bool ADBWorld::GetPageScreenRect(const APlayerController* PC, FVector2D& OutMin, FVector2D& OutMax) const
{
	if (!PC)
	{
		return false;
	}
	const float W = CalendarSize.X * 0.5f;
	const float H = CalendarSize.Y * 0.5f;
	const FVector C(CalendarCenter.X, WallDistCm - 3.f, CalendarCenter.Z);
	OutMin = FVector2D(1e9f, 1e9f);
	OutMax = FVector2D(-1e9f, -1e9f);
	for (const FVector& Off : { FVector(-W, 0, -H), FVector(W, 0, -H), FVector(-W, 0, H), FVector(W, 0, H) })
	{
		FVector2D S;
		if (!PC->ProjectWorldLocationToScreen(C + Off, S))
		{
			return false;
		}
		OutMin = FVector2D::Min(OutMin, S);
		OutMax = FVector2D::Max(OutMax, S);
	}
	return true;
}

void ADBWorld::FlipPage(bool bUp)
{
	PageTarget = bUp ? 168.f : 0.f;
}

void ADBWorld::StartScan(float Seconds)
{
	bScanning = true;
	ScanT = 0.f;
	ScanDuration = FMath::Max(0.1f, Seconds);
	ScanS = -HalfDiag();
	ScanBeam->SetVisibility(true);
}

void ADBWorld::StopScan()
{
	bScanning = false;
	ScanS = HalfDiag() + 1.f;
	ScanBeam->SetVisibility(false);
}

float ADBWorld::SweepCoord(int32 Building) const
{
	if (!BuildingRoots.IsValidIndex(Building))
	{
		return 0.f;
	}
	FVector P = BuildingRoots[Building]->GetComponentLocation();
	P.Z = 0.f;
	return FVector::DotProduct(P, SweepDir);
}

void ADBWorld::StartQuake(const TArray<bool>& SafeBuildings, float Seconds)
{
	QuakeSafe = SafeBuildings;
	QuakeSafe.SetNum(BuildingRoots.Num());
	bQuaking = true;
	QuakeT = 0.f;
	QuakeDuration = FMath::Max(0.5f, Seconds);
}

void ADBWorld::PopBuilding(int32 Building)
{
	if (PopTime.IsValidIndex(Building))
	{
		PopTime[Building] = KINDA_SMALL_NUMBER;
	}
}

void ADBWorld::Tick(float Dt)
{
	Super::Tick(Dt);

	// Takvim sayfası
	if (PageHinge && !FMath::IsNearlyEqual(PageAngle, PageTarget, 0.01f))
	{
		PageAngle = FMath::FInterpConstantTo(PageAngle, PageTarget, Dt, PageFlipDegPerSec);
		PageHinge->SetRelativeRotation(FRotator(0.f, 0.f, PageSign * PageAngle));
	}

	// Tarama
	if (bScanning)
	{
		ScanT += Dt;
		const float R = HalfDiag();
		const float A = FMath::Clamp(ScanT / ScanDuration, 0.f, 1.f);
		ScanS = FMath::Lerp(-R, R, A);
		const float Len = FMath::Max(4.f, 2.f * (R - FMath::Abs(ScanS)));
		ScanBeam->SetRelativeLocation(SweepDir * ScanS + FVector(0, 0, BoardTopZ + 1.f));
		ScanBeam->SetRelativeScale3D(FVector(Len / 100.f, 0.03f, 0.02f));
		if (A >= 1.f)
		{
			ScanBeam->SetVisibility(false);
		}
	}

	// Doğru uygulama: kısa zıplama
	for (int32 i = 0; i < PopTime.Num(); ++i)
	{
		if (PopTime[i] <= 0.f)
		{
			continue;
		}
		PopTime[i] += Dt;
		const float T = PopTime[i] / 0.45f;
		const float S = T < 1.f ? 1.f + 0.22f * FMath::Sin(T * PI) : 1.f;
		BuildingRoots[i]->SetRelativeScale3D(FVector(S));
		if (T >= 1.f)
		{
			PopTime[i] = 0.f;
		}
	}

	// Deprem
	if (bQuaking && BoardRoot)
	{
		QuakeT += Dt;
		const float A = FMath::Clamp(QuakeT / QuakeDuration, 0.f, 1.f);
		const float Env = FMath::Sin(A * PI);
		const float Amp = 4.f * Env;
		BoardRoot->SetRelativeLocation(FVector(FMath::Sin(QuakeT * 41.f) * Amp, FMath::Cos(QuakeT * 33.f) * Amp, 0.f));
		BoardRoot->SetRelativeRotation(FRotator(FMath::Sin(QuakeT * 23.f) * 1.2f * Env, 0.f, FMath::Cos(QuakeT * 29.f) * 1.2f * Env));

		int32 Unsafe = 0;
		for (int32 i = 0; i < BuildingRoots.Num(); ++i)
		{
			USceneComponent* B = BuildingRoots[i];
			if (QuakeSafe[i])
			{
				B->SetRelativeRotation(FRotator(FMath::Sin(QuakeT * 30.f + i) * 3.f * Env, 0.f, FMath::Cos(QuakeT * 27.f + i) * 3.f * Env));
				continue;
			}
			const float Start = 0.9f + 0.18f * Unsafe++;
			const float F = FMath::SmoothStep(0.f, 1.f, FMath::Clamp((QuakeT - Start) / 0.8f, 0.f, 1.f));
			const float Shake = (1.f - F) * Env;
			B->SetRelativeRotation(FRotator(FallRot[i].Pitch * F + FMath::Sin(QuakeT * 34.f + i) * 4.f * Shake, 0.f,
				FallRot[i].Roll * F + FMath::Cos(QuakeT * 31.f + i) * 4.f * Shake));
			B->SetRelativeScale3D(FVector(1.f, 1.f, FMath::Lerp(1.f, 0.42f, F)));
		}

		if (A >= 1.f)
		{
			bQuaking = false;
			BoardRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			for (int32 i = 0; i < BuildingRoots.Num(); ++i)
			{
				if (QuakeSafe[i])
				{
					BuildingRoots[i]->SetRelativeRotation(FRotator::ZeroRotator);
				}
			}
		}
	}
}
