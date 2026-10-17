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
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogDBWorld, Log, All);

namespace
{
	using DBColor::RGB;
	const FVector SweepDir = FVector(-1.f, -1.f, 0.f).GetSafeNormal(); // arka köşeden (+X+Y) kameraya doğru
	// DBData'daki 5×5 arsa sırası → 13 karoluk tahtada 2×2 bloğun ilk karosu (aralarda bir karo sokak)
	const int32 LotOrigin[5] = { 0, 3, 6, 9, 11 };
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneF(TEXT("/Engine/BasicShapes/Plane.Plane"));
	// UWidgetComponent'in kullandığı ışıksız, iki yüzlü doku malzemesi; parametre "SlateUI"
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PageMatF(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PuffMatF(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	PuffMaterial = PuffMatF.Object;
	PlaneMesh = PlaneF.Object;
	PageMaterial = PageMatF.Object;
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
	return (GridN * CellCm * 0.5f + 2.f) * UE_SQRT_2;
}

FVector ADBWorld::LotCenter(FIntPoint Lot) const
{
	const float Min = -GridN * CellCm * 0.5f;
	const int32 GX = LotOrigin[FMath::Clamp(Lot.X, 0, 4)];
	const int32 GY = LotOrigin[FMath::Clamp(Lot.Y, 0, 4)];
	return FVector(Min + (GX + 1) * CellCm, Min + (GY + 1) * CellCm, BoardTopZ);
}

void ADBWorld::Build()
{
	BuildRoom();
	BuildBoard();
	BuildCalendar();
	BuildCameras();
	BuildFx();
	BuildCars();
	BuildHydrant();

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		It->SetActorRotation(SunRotation);
	}
	UE_LOG(LogDBWorld, Log, TEXT("DB_WORLD bina=%d"), BuildingRoots.Num());
}

void ADBWorld::BuildRoom()
{
	USceneComponent* R = RootComponent;
	const float H = 560.f;
	const float Span = 1400.f;
	Part(R, CubeMesh, FVector(0, 0, FloorZ - 1.f), FVector(Span, Span, 2.f), RGB(170, 132, 98));
	// İki duvar köşe oluşturur; kaide bu köşeye dayalı (blend: Cube.001). Kamera duvarın arkasına dönünce duvar gizlenir.
	WallXRoot = Node(R, FVector::ZeroVector);
	WallYRoot = Node(R, FVector::ZeroVector);
	Part(WallXRoot, CubeMesh, FVector(WallX + 2.f, 0, FloorZ + H * 0.5f), FVector(4.f, Span, H), RGB(238, 224, 198));
	Part(WallYRoot, CubeMesh, FVector(0, WallY + 2.f, FloorZ + H * 0.5f), FVector(Span, 4.f, H), RGB(228, 210, 180));
	Part(WallXRoot, CubeMesh, FVector(WallX - 1.f, 0, FloorZ + 6.f), FVector(4.f, Span, 12.f), RGB(196, 154, 108));
	Part(WallYRoot, CubeMesh, FVector(0, WallY - 1.f, FloorZ + 6.f), FVector(Span, 4.f, 12.f), RGB(196, 154, 108));
	R = WallXRoot;
	// Takvimin solunda pencere
	const float WinY = -190.f;
	const float WinZ = 110.f;
	Part(R, CubeMesh, FVector(WallX - 0.5f, WinY, WinZ), FVector(2.f, 120.f, 96.f), RGB(150, 112, 78));
	Part(R, CubeMesh, FVector(WallX - 1.2f, WinY, WinZ), FVector(2.f, 108.f, 84.f), RGB(176, 214, 236));
	Part(R, CubeMesh, FVector(WallX - 1.8f, WinY, WinZ), FVector(2.f, 4.f, 84.f), RGB(150, 112, 78));
	Part(R, CubeMesh, FVector(WallX - 1.8f, WinY, WinZ), FVector(2.f, 108.f, 4.f), RGB(150, 112, 78));
}

void ADBWorld::BuildBoard()
{
	BoardRoot = Node(RootComponent, FVector::ZeroVector);
	USceneComponent* B = BoardRoot;

	// Masa bütün zemini kaplar (üstü Z=0): tahtanın çevresi her açıdan masa yüzeyi, arayüz bunun üstüne
	const float Far = 1300.f;
	Part(RootComponent, CubeMesh, FVector((WallX - Far) * 0.5f, (WallY - Far) * 0.5f, FloorZ * 0.5f),
		FVector(WallX + Far, WallY + Far, -FloorZ), RGB(196, 150, 104));

	// Tahta (blend: Cube.002): taban, pahlı kenar, oluklu düzlük
	const float Plateau = GridN * CellCm;
	Part(B, CubeMesh, FVector(0, 0, 3.6f), FVector(BoardCm, BoardCm, 10.2f), RGB(176, 132, 90), FRotator::ZeroRotator, true);
	Part(B, CubeMesh, FVector(0, 0, 10.35f), FVector(BoardCm - 10.f, BoardCm - 10.f, 3.3f), RGB(196, 154, 108), FRotator::ZeroRotator, true);
	Part(B, CubeMesh, FVector(0, 0, 12.5f), FVector(Plateau + 2.f, Plateau + 2.f, 1.f), RGB(150, 112, 78), FRotator::ZeroRotator, true);
	const float TileH = BoardTopZ - 13.f;
	for (int32 X = 0; X < GridN; ++X)
	{
		for (int32 Y = 0; Y < GridN; ++Y)
		{
			const bool bDark = ((X + Y) & 1) != 0;
			const FVector C(-Plateau * 0.5f + (X + 0.5f) * CellCm, -Plateau * 0.5f + (Y + 0.5f) * CellCm, 13.f + TileH * 0.5f);
			if (Y >= GridN - 2)
			{
				// Sahil: son iki sıra deniz, karanın biraz altında (haritaya gömülü), dalgalanır
				UStaticMeshComponent* Sea = Part(B, CubeMesh, C - FVector(0, 0, 0.6f), FVector(CellCm + 0.2f, CellCm + 0.2f, TileH),
					bDark ? RGB(96, 160, 204) : RGB(110, 174, 214), FRotator::ZeroRotator, true);
				SeaTiles.Add(Sea);
				continue;
			}
			if (IsRoad(FIntPoint(X, Y)))
			{
				// Yol: arsaların arasındaki karolar asfalt, düz yolda kesik orta çizgi
				Part(B, CubeMesh, C, FVector(CellCm, CellCm, TileH), RGB(98, 100, 110), FRotator::ZeroRotator, true);
				const bool bV = IsStreetIndex(X) && !IsStreetIndex(Y);
				const bool bH = IsStreetIndex(Y) && !IsStreetIndex(X);
				if (bV || bH)
				{
					Part(B, CubeMesh, C + FVector(0, 0, TileH * 0.5f + 0.05f), bV ? FVector(0.5f, 3.2f, 0.1f) : FVector(3.2f, 0.5f, 0.1f), RGB(236, 230, 210));
				}
				continue;
			}
			Part(B, CubeMesh, C, FVector(TileCm, TileCm, TileH), bDark ? RGB(226, 204, 164) : RGB(236, 218, 182), FRotator::ZeroRotator, true);
		}
	}

	// Deniz tabanı (karanın altında, tahtanın içinde) ve kıyıda köpük çizgisi: deniz haritaya gömülü
	{
		const float SeaY0 = -Plateau * 0.5f + (GridN - 2) * CellCm;
		const float SeaY1 = Plateau * 0.5f;
		Part(B, CubeMesh, FVector(0.f, (SeaY0 + SeaY1) * 0.5f, 12.6f), FVector(Plateau + 0.4f, SeaY1 - SeaY0, 0.8f), RGB(80, 140, 190), FRotator::ZeroRotator, true);
		Part(B, CubeMesh, FVector(0.f, SeaY0 + 0.5f, 13.55f), FVector(Plateau, 1.0f, 0.35f), RGB(240, 246, 250));
	}

	// Su birikintileri (sel): rögar kapağı konunca kaybolur
	for (const FIntPoint& Fc : FDBContent::Get().FloodCells)
	{
		PuddleComps.Add(Part(B, CubeMesh, CellLocal(Fc) + FVector(0, 0, 0.15f), FVector(CellCm - 0.8f, CellCm - 0.8f, 0.3f), RGB(104, 164, 218)));
		PuddleCells.Add(Fc);
	}

	const FDBContent& Content = FDBContent::Get();
	for (int32 i = 0; i < Content.Buildings.Num(); ++i)
	{
		BuildBuilding(i);
	}

	// Boş arsalara ağaç
	const FIntPoint Trees[] = { FIntPoint(1, 0), FIntPoint(2, 1), FIntPoint(4, 1), FIntPoint(1, 2), FIntPoint(2, 3), FIntPoint(4, 3) };
	for (const FIntPoint& T : Trees)
	{
		BuildTree(B, LotCenter(T), BuildingScale);
	}

	// Tarama çizgisi (yalnızca taramada görünür)
	ScanBeam = Part(B, CubeMesh, FVector(0, 0, BoardTopZ + 1.f), FVector(100.f, 1.6f, 1.2f), RGB(222, 196, 140), FRotator(0, -45.f, 0));
	ScanBeam->SetVisibility(false);

	// Tahliye edilen sakinler (tsunami)
	const FLinearColor Shirts[] = { RGB(232, 128, 104), RGB(96, 170, 164), RGB(236, 188, 86), RGB(172, 152, 204) };
	for (int32 k = 0; k < 4; ++k)
	{
		USceneComponent* Wk = Node(B, FVector(0, 0, BoardTopZ));
		Part(Wk, CylinderMesh, FVector(0, 0, 2.2f), FVector(2.4f, 2.4f, 4.4f), Shirts[k]);
		Part(Wk, SphereMesh, FVector(0, 0, 5.6f), FVector(2.6f, 2.6f, 2.6f), RGB(240, 200, 160));
		Wk->SetVisibility(false, true);
		Walkers.Add(Wk);
	}
}

void ADBWorld::BuildTree(USceneComponent* Parent, const FVector& Loc, float Scale)
{
	Part(Parent, CylinderMesh, Loc + FVector(0, 0, 3.f * Scale), FVector(2.4f, 2.4f, 6.f) * Scale, RGB(140, 100, 66));
	Part(Parent, SphereMesh, Loc + FVector(0, 0, 10.f * Scale), FVector(11.f, 11.f, 10.f) * Scale, RGB(120, 170, 100));
}

void ADBWorld::BuildBuilding(int32 Index)
{
	const FDBBuildingDef& D = FDBContent::Get().Buildings[Index];
	// Bina tasarım ölçüsünde (≈20 cm) kurulur, kök BuildingScale ile 2×2 karoya küçülür
	USceneComponent* Root = Node(BoardRoot, LotCenter(D.Lot));
	Root->SetRelativeScale3D(FVector(BuildingScale));
	BuildingRoots.Add(Root);
	PopTime.Add(0.f);
	FallRot.Add(FRotator((Index & 1) ? 26.f : -26.f, 0.f, (Index & 2) ? 20.f : -20.f));

	// Kasabalı: plakanın ön köşesinde, yol kenarında (tasarım birimi)
	{
		const FLinearColor Shirts[] = { RGB(232, 128, 104), RGB(96, 170, 164), RGB(236, 188, 86), RGB(172, 152, 204), RGB(120, 180, 110), RGB(132, 176, 212) };
		USceneComponent* Fig = Node(Root, FVector(-15.f, 7.f, 0.f));
		Part(Fig, CylinderMesh, FVector(0, 0, 4.5f), FVector(5.f, 5.f, 9.f), Shirts[Index % 6], FRotator::ZeroRotator, true);
		Part(Fig, SphereMesh, FVector(0, 0, 11.f), FVector(5.4f, 5.4f, 5.4f), RGB(240, 200, 160), FRotator::ZeroRotator, true);
		Part(Fig, SphereMesh, FVector(0.6f, 0, 12.2f), FVector(5.6f, 5.6f, 3.6f), (Index & 1) ? RGB(70, 50, 36) : RGB(200, 200, 205), FRotator::ZeroRotator, true);
		ResidentRoots.Add(Fig);
	}

	if (D.bPark)
	{
		Part(Root, CubeMesh, FVector(0, 0, 1.f), FVector(24.f, 24.f, 2.f), D.Body, FRotator::ZeroRotator, true);
		BuildTree(Root, FVector(-5.f, -5.f, 2.f), 0.8f);
		BuildTree(Root, FVector(6.f, 4.f, 2.f), 0.65f);
		Part(Root, CubeMesh, FVector(4.f, -6.f, 3.5f), FVector(8.f, 2.5f, 1.2f), RGB(140, 100, 66));
		BuildingHeights.Add(14.f);
		return;
	}

	const FVector S(FMath::Min(D.Size.X, 20.f), FMath::Min(D.Size.Y, 20.f), D.Size.Z);
	const float Base = 1.f;
	// Plaka 24×24 tasarım birimi = BuildingScale ile tam 2×2 karo
	Part(Root, CubeMesh, FVector(0, 0, 0.5f), FVector(24.f, 24.f, 1.f), RGB(244, 232, 208));
	if (D.bShelter)
	{
		// Sığınak: yarı gömülü kubbe, kapı, yeşil bayrak
		Part(Root, SphereMesh, FVector(0, 0, Base), FVector(S.X, S.Y, S.Z * 2.f), D.Body, FRotator::ZeroRotator, true);
		Part(Root, CubeMesh, FVector(0, -S.Y * 0.42f, Base + 3.f), FVector(5.f, 3.f, 6.f), RGB(70, 90, 70));
		Part(Root, CylinderMesh, FVector(7.f, 7.f, Base + 9.f), FVector(0.8f, 0.8f, 18.f), RGB(90, 90, 96));
		Part(Root, CubeMesh, FVector(7.f, 4.f, Base + 15.f), FVector(0.5f, 6.f, 4.f), RGB(90, 170, 90));
		BuildingHeights.Add(Base + 18.f);
		return;
	}
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
				Part(Root, CubeMesh, FVector(Col * S.X * 0.24f, -S.Y * 0.5f - 0.3f, Z), FVector(4.f, 0.8f, 4.5f), Glass);
			}
		}
	}
}

void ADBWorld::BuildCalendar()
{
	// Takvim X duvarında, odaya (-X) bakar. Yerel eksenler: X = genişlik, -Y = duvardan odaya, Z = yukarı.
	const float W = CalendarSize.X;
	const float H = CalendarSize.Y;
	CalendarRoot = Node(RootComponent, FVector(WallX, CalendarYZ.X, CalendarYZ.Y));
	CalendarRoot->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	USceneComponent* R = CalendarRoot;

	const FLinearColor Paper = RGB(248, 240, 224);
	Part(R, CubeMesh, FVector(0, -1.f, 0), FVector(W + 8.f, 2.f, H + 10.f), RGB(150, 108, 72));
	// Alttaki sayfa (Ayarlar / Emeği Geçenler buraya yazılır)
	Part(R, CubeMesh, FVector(0, -2.5f, 0), FVector(W, 0.6f, H), Paper);
	const FVector2D TexSize = PageTextureSize();
	TopRT = UKismetRenderingLibrary::CreateRenderTarget2D(this, int32(TexSize.X), int32(TexSize.Y), RTF_RGBA16f, FLinearColor(FColor(248, 240, 224)) * PageGain);
	UnderRT = UKismetRenderingLibrary::CreateRenderTarget2D(this, int32(TexSize.X), int32(TexSize.Y), RTF_RGBA16f, FLinearColor(FColor(248, 240, 224)) * PageGain);
	UnderPagePlane = PagePlane(R, FVector(0, -2.85f, 0), UnderRT);

	// Üstteki sayfa: menteşe üst kenarda, sayfa aşağı sarkar
	PageHinge = Node(R, FVector(0, -4.f, H * 0.5f));
	Part(PageHinge, CubeMesh, FVector(0, 0, -H * 0.5f), FVector(W, 0.6f, H), Paper);
	TopPagePlane = PagePlane(PageHinge, FVector(0, -0.35f, -H * 0.5f), TopRT);

	// Spiral halkalar
	const int32 Rings = 22;
	for (int32 i = 0; i < Rings; ++i)
	{
		const float X = -W * 0.5f + 3.f + i * (W - 6.f) / (Rings - 1);
		Part(R, CylinderMesh, FVector(X, -4.f, H * 0.5f + 1.f), FVector(1.4f, 1.4f, 5.f), RGB(90, 90, 96));
	}
	Part(R, CylinderMesh, FVector(0, -2.f, H * 0.5f + 7.f), FVector(1.5f, 1.5f, 4.f), RGB(90, 90, 96), FRotator(0, 0, 90.f));

	// Menteşe yönü: küçük açıda sayfanın alt ucu odaya (yerel -Y) gitmeli, duvara değil
	PageSign = FRotator(0, 0, 30.f).RotateVector(FVector(0, 0, -1.f)).Y < 0.f ? 1.f : -1.f;
}

void ADBWorld::BuildCameras()
{
	auto Make = [this](const FVector& Loc, const FRotator& Rot, float Fov)
	{
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(Loc, Rot);
		UCameraComponent* CC = Cam->GetCameraComponent();
		CC->SetFieldOfView(Fov);
		CC->SetConstraintAspectRatio(false);
		CC->PostProcessSettings.bOverride_MotionBlurAmount = true;
		CC->PostProcessSettings.MotionBlurAmount = 0.f;
		CC->PostProcessBlendWeight = 1.f;
		return Cam;
	};

	// Menü: yatay bakış, üstte takvim, altta tahta
	MenuCam = Make(MenuCamLoc, (MenuCamLookAt - MenuCamLoc).Rotation(), MenuCamFov);

	// Takvim: tam karşıdan, yakın (Ayarlar / Emeği Geçenler)
	const FVector CalFront(WallX - 4.f, CalendarYZ.X, CalendarYZ.Y);
	CalendarCam = Make(CalFront - FVector(CalendarCamDistance, 0, 0), FRotator::ZeroRotator, MenuCamFov);

	// Oyun: tahtaya köşeden, eşkenar dörtgen görünüş
	const FRotator BoardRot(BoardCamPitch, BoardCamYaw, 0.f);
	BoardCam = Make(BoardFocus - BoardRot.Vector() * BoardCamDistance, BoardRot, BoardCamFov);
	ViewDist = ViewDistTarget = BoardCamDistance;

	ApplyExposure(ExposureEV100);
}

void ADBWorld::ApplyExposure(float EV100)
{
	ExposureEV100 = EV100;
	for (ACameraActor* Cam : { MenuCam.Get(), CalendarCam.Get(), BoardCam.Get() })
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
	return BuildingRoots[Building]->GetComponentTransform().TransformPosition(FVector(0, 0, BuildingHeights[Building] + 16.f));
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

UStaticMeshComponent* ADBWorld::PagePlane(USceneComponent* Parent, const FVector& Loc, UTextureRenderTarget2D* RT)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(PlaneMesh);
	C->SetupAttachment(Parent);
	C->SetRelativeLocation(Loc);
	// Düzlemin X'i → takvimde -X (bakanın sağı), Y'si → aşağı, normali → odaya (-Y)
	C->SetRelativeRotation(FMatrix(FVector(-1, 0, 0), FVector(0, 0, -1), FVector(0, -1, 0), FVector::ZeroVector).Rotator());
	C->SetRelativeScale3D(FVector(CalendarSize.X / 100.f * (bFlipPageU ? -1.f : 1.f), CalendarSize.Y / 100.f * (bFlipPageV ? -1.f : 1.f), 1.f));
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(PageMaterial, this);
	Mid->SetTextureParameterValue(TEXT("SlateUI"), RT);
	C->SetMaterial(0, Mid);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->RegisterComponent();
	return C;
}

bool ADBWorld::PagePointFromScreen(const APlayerController* PC, const FVector2D& Screen, bool bTop, FVector2D& OutUV) const
{
	const UStaticMeshComponent* Pl = bTop ? TopPagePlane.Get() : UnderPagePlane.Get();
	FVector O, D;
	if (!PC || !Pl || !PC->DeprojectScreenPositionToWorld(Screen.X, Screen.Y, O, D))
	{
		return false;
	}
	const FTransform T = Pl->GetComponentTransform();
	const FVector LO = T.InverseTransformPosition(O);
	const FVector LD = T.InverseTransformVector(D);
	if (FMath::IsNearlyZero(LD.Z))
	{
		return false;
	}
	const float Tt = -LO.Z / LD.Z;
	if (Tt < 0.f)
	{
		return false;
	}
	const FVector P = LO + LD * Tt;
	OutUV = FVector2D((P.X + 50.f) / 100.f, (P.Y + 50.f) / 100.f);
	return OutUV.X >= 0.f && OutUV.X <= 1.f && OutUV.Y >= 0.f && OutUV.Y <= 1.f;
}

bool ADBWorld::PageUVToScreen(const APlayerController* PC, bool bTop, const FVector2D& UV, FVector2D& OutScreen) const
{
	const UStaticMeshComponent* Pl = bTop ? TopPagePlane.Get() : UnderPagePlane.Get();
	if (!PC || !Pl)
	{
		return false;
	}
	const FVector Wp = Pl->GetComponentTransform().TransformPosition(FVector(UV.X * 100.f - 50.f, UV.Y * 100.f - 50.f, 0.f));
	return PC->ProjectWorldLocationToScreen(Wp, OutScreen);
}

// ---------------------------------------------------------------- atmosfer: sis ve kum

namespace
{
	// Afet → sis rengi (dizin): 0 toz (deprem), 1 sis (sel), 2 duman (yangın), 3 deniz pusu (tsunami), 4 ılık (ilk yardım), 5 kum (tarama)
	int32 TintForHazard(const FString& H)
	{
		if (H == TEXT("Deprem")) return 0;
		if (H == TEXT("Sel")) return 1;
		if (H == TEXT("Yangın")) return 2;
		if (H == TEXT("Tsunami")) return 3;
		return 4;
	}
}

void ADBWorld::BuildFx()
{
	// Yumuşak bulut dokusu CPU'da üretilir (asset yok): ortası opak, kenarı sönen, hafif pürüzlü
	// İlk denemede zemine çok yakın renk ve bina diplerinde kaldı (fark görüntüsünde zor seçildi): koyu, belirgin tonlar
	const FColor Tints[] = { FColor(150, 128, 100), FColor(104, 146, 200), FColor(86, 80, 80), FColor(110, 176, 210), FColor(222, 140, 130), FColor(214, 186, 128) };
	const int32 N = 64;
	for (const FColor& T : Tints)
	{
		TArray<uint8> Px;
		Px.SetNumUninitialized(N * N * 4);
		for (int32 y = 0; y < N; ++y)
		{
			for (int32 x = 0; x < N; ++x)
			{
				const float Dx = (x + 0.5f) / N * 2.f - 1.f;
				const float Dy = (y + 0.5f) / N * 2.f - 1.f;
				const float R = FMath::Sqrt(Dx * Dx + Dy * Dy);
				const float Noise = 0.85f + 0.15f * FMath::Sin(x * 0.7f + y * 0.37f) * FMath::Cos(y * 0.53f - x * 0.21f);
				const float A = FMath::Clamp(FMath::Pow(FMath::Max(0.f, 1.f - R), 1.4f) * 0.9f * Noise, 0.f, 1.f);
				uint8* P = &Px[(y * N + x) * 4];
				P[0] = T.B; P[1] = T.G; P[2] = T.R; P[3] = (uint8)FMath::RoundToInt(A * 255.f);
			}
		}
		UTexture2D* Tex = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8, NAME_None, TConstArrayView64<uint8>(Px.GetData(), Px.Num()));
		Tex->UpdateResource();
		PuffTextures.Add(Tex);
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(PuffMaterial, this);
		Mid->SetTextureParameterValue(TEXT("SlateUI"), Tex);
		PuffMids.Add(Mid);
	}

	// Tarama tozu: çizgiyle birlikte ilerleyen 6 bulut
	for (int32 i = 0; i < 6; ++i)
	{
		MakePuff(-1, FVector::ZeroVector, 16.f, 5);
	}
	// Kum taneleri
	for (int32 i = 0; i < 60; ++i)
	{
		const uint8 Shade = uint8(180 + (i * 37) % 50);
		UStaticMeshComponent* G = Part(BoardRoot, CubeMesh, FVector::ZeroVector, FVector(0.9f), RGB(uint8(Shade + 20), Shade, uint8(Shade - 50)));
		G->SetVisibility(false);
		GrainComps.Add(G);
		Grains.AddDefaulted();
	}
}

int32 ADBWorld::MakePuff(int32 Building, const FVector& Base, float Size, int32 Tint)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(PlaneMesh);
	C->SetupAttachment(BoardRoot);
	C->SetRelativeLocation(Base);
	C->SetRelativeScale3D(FVector(0.01f));
	C->SetMaterial(0, PuffMids[FMath::Clamp(Tint, 0, PuffMids.Num() - 1)]);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetVisibility(false);
	C->RegisterComponent();
	PuffComps.Add(C);
	FPuff P;
	P.Building = Building;
	P.Base = Base;
	P.Size = Size;
	P.Phase = PuffComps.Num() * 1.7f;
	return Puffs.Add(P);
}

void ADBWorld::ShowHaze(int32 Building, const FString& Hazard)
{
	if (!BuildingRoots.IsValidIndex(Building))
	{
		return;
	}
	const FVector C = BuildingRoots[Building]->GetRelativeLocation();
	const int32 Tint = TintForHazard(Hazard);
	for (int32 k = 0; k < 6; ++k)
	{
		const float Ang = k * 2.f * PI / 6.f + Building;
		const FVector Base = C + FVector(FMath::Cos(Ang) * 10.f, FMath::Sin(Ang) * 10.f, 4.f + k * 2.4f);
		const int32 Ix = MakePuff(Building, Base, 18.f + (k % 3) * 4.f, Tint);
		Puffs[Ix].Target = 1.f;
		PuffComps[Ix]->SetVisibility(true);
	}
}

void ADBWorld::ClearHaze(int32 Building)
{
	for (FPuff& P : Puffs)
	{
		if (P.Building == Building)
		{
			P.Target = 0.f;
		}
	}
	if (BuildingRoots.IsValidIndex(Building))
	{
		const FVector Top = BuildingRoots[Building]->GetRelativeLocation() + FVector(0, 0, 6.f);
		for (int32 i = 0; i < 14; ++i)
		{
			SpawnGrain(Top + FVector(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), 0.f), FMath::FRandRange(30.f, 55.f));
		}
	}
}

void ADBWorld::SpawnGrain(const FVector& At, float Up)
{
	for (int32 i = 0; i < Grains.Num(); ++i)
	{
		if (!Grains[i].bOn)
		{
			Grains[i].bOn = true;
			Grains[i].P = At;
			Grains[i].V = FVector(FMath::FRandRange(-18.f, 18.f), FMath::FRandRange(-18.f, 18.f), Up);
			GrainComps[i]->SetVisibility(true);
			GrainComps[i]->SetRelativeLocation(At);
			return;
		}
	}
}

void ADBWorld::UpdateBoardCam(float Dt)
{
	if (!BoardCam)
	{
		return;
	}
	ViewYaw = FMath::FInterpTo(ViewYaw, ViewYawTarget, Dt, 7.f);
	ViewDist = FMath::FInterpTo(ViewDist, ViewDistTarget, Dt, 8.f);
	// Bakış noktası tahtanın ortasına göre döner: hangi açıdan bakılırsa bakılsın tahta ortada kalır
	const FVector Focus = FVector(BoardFocus.X, BoardFocus.Y, 0.f).RotateAngleAxis(ViewYaw, FVector::UpVector) + FVector(0.f, 0.f, BoardFocus.Z);
	const FRotator Rot(BoardCamPitch, BoardCamYaw + ViewYaw, 0.f);
	const FVector CamLoc = Focus - Rot.Vector() * ViewDist;
	BoardCam->SetActorLocationAndRotation(CamLoc, Rot);
	// Kamera bir duvarın arkasına geçtiyse o duvarı (ve üstündekileri) gizle; yoksa yalnızca duvarın arkası görünüyordu
	const bool bShowX = CamLoc.X < WallX - 2.f || FMath::IsNearlyZero(ViewYawTarget);
	const bool bShowY = CamLoc.Y < WallY - 2.f || FMath::IsNearlyZero(ViewYawTarget);
	if (WallXRoot && WallXRoot->IsVisible() != bShowX)
	{
		WallXRoot->SetVisibility(bShowX, true);
		if (CalendarRoot) CalendarRoot->SetVisibility(bShowX, true);
	}
	if (WallYRoot && WallYRoot->IsVisible() != bShowY)
	{
		WallYRoot->SetVisibility(bShowY, true);
	}
}

// ---------------------------------------------------------------- ızgara: yol, rögar, musluk, araçlar

FVector ADBWorld::CellLocal(FIntPoint Cell) const
{
	const float Min = -GridN * CellCm * 0.5f;
	return FVector(Min + (Cell.X + 0.5f) * CellCm, Min + (Cell.Y + 0.5f) * CellCm, BoardTopZ);
}

FVector ADBWorld::CellWorld(FIntPoint Cell) const
{
	return BoardRoot ? BoardRoot->GetComponentTransform().TransformPosition(CellLocal(Cell)) : CellLocal(Cell);
}

bool ADBWorld::CellFromWorld(const FVector& WorldLoc, FIntPoint& OutCell) const
{
	if (!BoardRoot)
	{
		return false;
	}
	const FVector L = BoardRoot->GetComponentTransform().InverseTransformPosition(WorldLoc);
	const float Min = -GridN * CellCm * 0.5f;
	OutCell = FIntPoint(FMath::FloorToInt((L.X - Min) / CellCm), FMath::FloorToInt((L.Y - Min) / CellCm));
	return OutCell.X >= 0 && OutCell.Y >= 0 && OutCell.X < GridN && OutCell.Y < GridN && L.Z > BoardTopZ - 3.f;
}

FVector ADBWorld::ResidentWorld(int32 Building) const
{
	return ResidentRoots.IsValidIndex(Building) ? ResidentRoots[Building]->GetComponentTransform().TransformPosition(FVector(0, 0, 16.f)) : MarkerAnchor(Building);
}

void ADBWorld::PlaceManhole(FIntPoint Cell)
{
	if (Manholes.Contains(Cell))
	{
		return;
	}
	Manholes.Add(Cell);
	const FVector C = CellLocal(Cell);
	Part(BoardRoot, CylinderMesh, C + FVector(0, 0, 0.25f), FVector(5.4f, 5.4f, 0.5f), RGB(64, 66, 72));
	Part(BoardRoot, CylinderMesh, C + FVector(0, 0, 0.35f), FVector(4.2f, 4.2f, 0.5f), RGB(96, 98, 106));
	for (int32 k = -1; k <= 1; ++k)
	{
		Part(BoardRoot, CubeMesh, C + FVector(k * 1.1f, 0, 0.62f), FVector(0.4f, 3.2f, 0.1f), RGB(56, 58, 64));
	}
	const int32 Ix = PuddleCells.IndexOfByKey(Cell);
	if (Ix != INDEX_NONE)
	{
		PuddleComps[Ix]->SetVisibility(false);
		const FVector W = CellLocal(Cell) + FVector(0, 0, 2.f);
		for (int32 i = 0; i < 6; ++i)
		{
			SpawnGrain(W + FVector(FMath::FRandRange(-3.f, 3.f), FMath::FRandRange(-3.f, 3.f), 0.f), FMath::FRandRange(20.f, 35.f));
		}
	}
}

void ADBWorld::BuildHydrant()
{
	const FDBContent& C = FDBContent::Get();
	if (!BuildingRoots.IsValidIndex(C.HydrantBuilding))
	{
		return;
	}
	// Musluk binaya bağlı (tıklanınca o binanın sorunu), ölçeği geri alınır: çocukları cm ile kurulur
	USceneComponent* Root = BuildingRoots[C.HydrantBuilding];
	const FVector Local = CellLocal(C.HydrantCell) + FVector(2.4f, 0.f, 0.f);
	HydrantRoot = Node(Root, (Local - Root->GetRelativeLocation()) / BuildingScale);
	HydrantRoot->SetRelativeScale3D(FVector(1.f / BuildingScale));
	const FLinearColor Red = RGB(214, 64, 52);
	Part(HydrantRoot, CylinderMesh, FVector(0, 0, 2.2f), FVector(2.4f, 2.4f, 4.4f), Red, FRotator::ZeroRotator, true);
	Part(HydrantRoot, SphereMesh, FVector(0, 0, 4.5f), FVector(2.6f, 2.6f, 2.f), RGB(180, 48, 40), FRotator::ZeroRotator, true);
	Part(HydrantRoot, CylinderMesh, FVector(0, 0, 2.8f), FVector(1.f, 1.f, 4.f), Red, FRotator(0.f, 0.f, 90.f), true);
	Part(HydrantRoot, CylinderMesh, FVector(0, 0, 0.3f), FVector(3.2f, 3.2f, 0.6f), RGB(150, 150, 156), FRotator::ZeroRotator, true);
	// Bozuk: eğik duruyor, dibinde su sızıntısı
	HydrantRoot->SetRelativeRotation(FRotator(0.f, 0.f, 24.f));
	HydrantLeak = Part(BoardRoot, CylinderMesh, Local + FVector(0, 1.6f, 0.12f), FVector(6.f, 7.f, 0.2f), RGB(104, 164, 218));
}

void ADBWorld::RepairHydrant()
{
	HydrantFixT = 0.f;
	if (HydrantLeak)
	{
		HydrantLeak->SetVisibility(false);
	}
	if (HydrantRoot)
	{
		const FVector W = BoardRoot->GetComponentTransform().InverseTransformPosition(HydrantRoot->GetComponentLocation());
		for (int32 i = 0; i < 10; ++i)
		{
			SpawnGrain(W + FVector(0, 0, 5.f), FMath::FRandRange(35.f, 60.f));
		}
	}
}

void ADBWorld::BuildCars()
{
	// Üç araba yollarda gidip gelir (sağ şeritten)
	struct FRoute { FIntPoint From, To; FLinearColor Color; float Speed; };
	const FRoute Routes[] = {
		{ FIntPoint(0, 5), FIntPoint(12, 5), RGB(232, 128, 104), 13.f },
		{ FIntPoint(5, 0), FIntPoint(5, 10), RGB(96, 170, 164), 10.f },
		{ FIntPoint(12, 2), FIntPoint(0, 2), RGB(236, 188, 86), 11.f },
	};
	for (const FRoute& Rt : Routes)
	{
		USceneComponent* Car = Node(BoardRoot, CellLocal(Rt.From));
		Part(Car, CubeMesh, FVector(0, 0, 1.4f), FVector(6.f, 3.f, 1.6f), Rt.Color);
		Part(Car, CubeMesh, FVector(-0.4f, 0, 2.7f), FVector(3.4f, 2.6f, 1.3f), Rt.Color);
		Part(Car, CubeMesh, FVector(-0.4f, 0, 2.75f), FVector(3.5f, 2.2f, 0.9f), RGB(176, 214, 236));
		for (int32 w = 0; w < 4; ++w)
		{
			Part(Car, CylinderMesh, FVector((w & 1) ? 1.9f : -1.9f, (w & 2) ? 1.5f : -1.5f, 0.7f), FVector(1.4f, 1.4f, 0.6f), RGB(40, 40, 44), FRotator(0.f, 0.f, 90.f));
		}
		CarRoots.Add(Car);
		FCar Cr;
		Cr.A = CellLocal(Rt.From);
		Cr.B = CellLocal(Rt.To);
		Cr.Speed = Rt.Speed;
		Cr.T = CarRoots.Num() * 0.27f;
		Cars.Add(Cr);
	}
}

void ADBWorld::StartEvacuation(int32 FromBuilding, int32 ToBuilding)
{
	const FDBContent& C = FDBContent::Get();
	if (!C.Buildings.IsValidIndex(FromBuilding) || !C.Buildings.IsValidIndex(ToBuilding))
	{
		return;
	}
	// Sokaklar arsaların arasındaki tek karoluk boşluklar: evin önündeki sokak → sığınağın yanındaki sokak → sığınak
	const FIntPoint From = C.Buildings[FromBuilding].Lot;
	const FIntPoint To = C.Buildings[ToBuilding].Lot;
	const float Min = -GridN * CellCm * 0.5f;
	auto Street = [&](int32 Cell) { return Min + (Cell + 0.5f) * CellCm; };
	const FVector A = LotCenter(From);
	const FVector Bp = LotCenter(To);
	const float RowFrom = Street(LotOrigin[From.Y] - 1);
	const float ColTo = Street(LotOrigin[To.X] - 1);
	const float RowTo = Street(LotOrigin[To.Y] + 2);
	EvacPath = { A, FVector(A.X, RowFrom, BoardTopZ), FVector(ColTo, RowFrom, BoardTopZ), FVector(ColTo, RowTo, BoardTopZ),
		FVector(Bp.X, RowTo, BoardTopZ), Bp };
	EvacT = 0.f;
}

float ADBWorld::PathLength() const
{
	float L = 0.f;
	for (int32 i = 1; i < EvacPath.Num(); ++i)
	{
		L += FVector::Dist(EvacPath[i - 1], EvacPath[i]);
	}
	return L;
}

FVector ADBWorld::PointOnPath(float Dist) const
{
	for (int32 i = 1; i < EvacPath.Num(); ++i)
	{
		const float Seg = FVector::Dist(EvacPath[i - 1], EvacPath[i]);
		if (Dist <= Seg && Seg > 0.f)
		{
			return FMath::Lerp(EvacPath[i - 1], EvacPath[i], Dist / Seg);
		}
		Dist -= Seg;
	}
	return EvacPath.Num() ? EvacPath.Last() : FVector::ZeroVector;
}

bool ADBWorld::GetPageScreenRect(const APlayerController* PC, FVector2D& OutMin, FVector2D& OutMax) const
{
	if (!PC || !CalendarRoot)
	{
		return false;
	}
	const float W = CalendarSize.X * 0.5f;
	const float H = CalendarSize.Y * 0.5f;
	const FTransform T = CalendarRoot->GetComponentTransform();
	OutMin = FVector2D(1e9f, 1e9f);
	OutMax = FVector2D(-1e9f, -1e9f);
	for (const FVector& Off : { FVector(-W, -3.f, -H), FVector(W, -3.f, -H), FVector(-W, -3.f, H), FVector(W, -3.f, H) })
	{
		FVector2D S;
		if (!PC->ProjectWorldLocationToScreen(T.TransformPosition(Off), S))
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

	UpdateBoardCam(Dt);

	// Sis bulutları: yavaş döner, salınır, nefes alır; hedef 0 olunca küçülüp kaybolur
	const FVector BeamPos = SweepDir * ScanS;
	for (int32 i = 0; i < Puffs.Num(); ++i)
	{
		FPuff& P = Puffs[i];
		UStaticMeshComponent* C = PuffComps[i];
		if (P.Building < 0)
		{
			// Tarama tozu çizgiyle ilerler
			P.Target = bScanning ? 1.f : 0.f;
			const FVector Across = FVector(1.f, -1.f, 0.f).GetSafeNormal();
			const float Half = FMath::Max(4.f, HalfDiag() - FMath::Abs(ScanS));
			P.Base = BeamPos + Across * ((i % 6) / 5.f * 2.f - 1.f) * Half * 0.9f + FVector(0, 0, BoardTopZ + 3.f + (i % 2) * 2.f);
		}
		P.Scale = FMath::FInterpTo(P.Scale, P.Target, Dt, P.Target > 0.f ? 2.f : 1.6f);
		if (P.Target <= 0.f && P.Scale < 0.04f)
		{
			if (C->IsVisible())
			{
				C->SetVisibility(false);
			}
			continue;
		}
		if (!C->IsVisible())
		{
			C->SetVisibility(true);
		}
		const float T = WaveT + P.Phase;
		const FVector Drift(FMath::Sin(T * 0.6f) * 2.2f, FMath::Cos(T * 0.47f) * 2.2f, FMath::Sin(T * 0.8f) * 0.8f);
		C->SetRelativeLocation(P.Base + Drift);
		C->SetRelativeRotation(FRotator(0.f, T * 14.f, 0.f));
		const float S = FMath::Max(0.02f, P.Size / 100.f * P.Scale * (1.f + 0.08f * FMath::Sin(T * 1.3f)));
		C->SetRelativeScale3D(FVector(S, S, 1.f));
	}

	// Kum: taramada çizgi boyunca savrulur, yere düşünce kaybolur
	if (bScanning)
	{
		GrainSpawn += Dt * 70.f;
		const FVector Across = FVector(1.f, -1.f, 0.f).GetSafeNormal();
		const float Half = FMath::Max(4.f, HalfDiag() - FMath::Abs(ScanS));
		while (GrainSpawn >= 1.f)
		{
			GrainSpawn -= 1.f;
			SpawnGrain(BeamPos + Across * FMath::FRandRange(-Half, Half) + FVector(0, 0, BoardTopZ + 1.f), FMath::FRandRange(25.f, 50.f));
		}
	}
	for (int32 i = 0; i < Grains.Num(); ++i)
	{
		FGrain& G = Grains[i];
		if (!G.bOn)
		{
			continue;
		}
		G.V.Z -= 220.f * Dt;
		G.P += G.V * Dt;
		if (G.P.Z < BoardTopZ)
		{
			G.bOn = false;
			GrainComps[i]->SetVisibility(false);
			continue;
		}
		GrainComps[i]->SetRelativeLocation(G.P);
	}

	// Arabalar: sona varınca dönüp geri gelir
	for (int32 i = 0; i < Cars.Num(); ++i)
	{
		FCar& Cr = Cars[i];
		const float Len = FMath::Max(1.f, FVector::Dist(Cr.A, Cr.B));
		Cr.T += Cr.Speed * Dt / Len;
		if (Cr.T >= 1.f)
		{
			Cr.T = 0.f;
			Cr.bFwd = !Cr.bFwd;
		}
		const FVector From = Cr.bFwd ? Cr.A : Cr.B;
		const FVector To = Cr.bFwd ? Cr.B : Cr.A;
		const FVector Dir = (To - From).GetSafeNormal();
		const FVector Right(-Dir.Y, Dir.X, 0.f);
		CarRoots[i]->SetRelativeLocationAndRotation(FMath::Lerp(From, To, Cr.T) + Right * 1.7f, Dir.Rotation());
	}

	// Musluk tamiri: doğrulur
	if (HydrantFixT >= 0.f && HydrantRoot)
	{
		HydrantFixT += Dt;
		const float F = FMath::Clamp(HydrantFixT / 0.6f, 0.f, 1.f);
		HydrantRoot->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Lerp(24.f, 0.f, F) + FMath::Sin(HydrantFixT * 30.f) * 4.f * (1.f - F)));
	}

	// Kasabalılar hafifçe sallanır
	for (int32 i = 0; i < ResidentRoots.Num(); ++i)
	{
		const FVector L = ResidentRoots[i]->GetRelativeLocation();
		ResidentRoots[i]->SetRelativeLocation(FVector(L.X, L.Y, FMath::Abs(FMath::Sin(WaveT * 2.5f + i)) * 0.8f));
	}

	// Deniz dalgalanır
	WaveT += Dt;
	for (int32 i = 0; i < SeaTiles.Num(); ++i)
	{
		const FVector L = SeaTiles[i]->GetRelativeLocation();
		SeaTiles[i]->SetRelativeLocation(FVector(L.X, L.Y, 12.8f + 0.2f * FMath::Sin(WaveT * 2.2f + L.X * 0.12f + L.Y * 0.05f)));
	}

	// Tahliye: sakinler sırayla yola çıkar, sığınağın önünde toplanır
	if (EvacT >= 0.f && EvacPath.Num() > 1)
	{
		EvacT += Dt;
		const float Len = PathLength();
		for (int32 k = 0; k < Walkers.Num(); ++k)
		{
			const float D = (EvacT - k * 0.5f) * WalkerSpeed;
			if (D < 0.f)
			{
				continue;
			}
			Walkers[k]->SetVisibility(true, true);
			const bool bArrived = D >= Len - 6.f;
			FVector P = PointOnPath(FMath::Min(D, Len - 6.f));
			P += bArrived ? FVector((k % 2) * 3.5f - 1.75f, (k / 2) * 3.5f - 5.f, 0.f) : FVector(0, 0, FMath::Abs(FMath::Sin(EvacT * 12.f + k)) * 1.2f);
			Walkers[k]->SetRelativeLocation(P);
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
		BuildingRoots[i]->SetRelativeScale3D(FVector(S * BuildingScale));
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
		const float Amp = 2.5f * Env;
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
			B->SetRelativeScale3D(FVector(1.f, 1.f, FMath::Lerp(1.f, 0.42f, F)) * BuildingScale);
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
