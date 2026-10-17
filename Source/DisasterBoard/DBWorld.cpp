#include "DBWorld.h"
#include "DBTypes.h"
#include "DBSfx.h"
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
#include "Components/SceneCaptureComponent2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

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
	BuildFeedbackFx();
	BuildLife();
	BuildWallArt();

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
		Part(B, CubeMesh, FVector(0.f, (SeaY0 + SeaY1) * 0.5f, 12.7f), FVector(Plateau + 0.4f, SeaY1 - SeaY0, 0.8f), RGB(80, 140, 190), FRotator::ZeroRotator, true);
		Part(B, CubeMesh, FVector(0.f, SeaY0 + 0.5f, 13.55f), FVector(Plateau, 1.0f, 0.35f), RGB(240, 246, 250));
	}

	// Su birikintileri (sel): rögar kapağı konunca kaybolur
	for (const FIntPoint& Fc : FDBContent::Get().FloodCells)
	{
		PuddleComps.Add(Part(B, CubeMesh, CellLocal(Fc) + FVector(0, 0, 0.15f), FVector(CellCm - 0.8f, CellCm - 0.8f, 0.3f), RGB(104, 164, 218)));
		PuddleCells.Add(Fc);
		// Bozuk rögar: delik açık, kapak yanına kaymış ve eğik duruyor
		BrokenCovers.Add(Part(B, CylinderMesh, CellLocal(Fc) + FVector(0, 0, 0.32f), FVector(4.2f, 4.2f, 0.06f), RGB(34, 36, 42)));
		BrokenCovers.Add(Part(B, CylinderMesh, CellLocal(Fc) + FVector(2.2f, 1.6f, 0.9f), FVector(5.4f, 5.4f, 0.5f), RGB(78, 80, 88), FRotator(0.f, 0.f, 16.f)));
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

	// Tarama çizgisi (yalnızca taramada görünür): parlak lazer çubuğu + üstünde yükselen ışık perdesi
	ScanBeam = Part(B, CubeMesh, FVector(0, 0, BoardTopZ + 1.f), FVector(100.f, 1.6f, 1.2f), RGB(140, 255, 220), FRotator(0, -45.f, 0));
	ScanBeam->SetVisibility(false);
	{
		const int32 N = 64;
		TArray<uint8> Px;
		Px.SetNumUninitialized(4 * N * 4);
		for (int32 y = 0; y < N; ++y)
		{
			// Alt kenar (V=1) parlak, yukarı doğru söner; ince yatay tarama çizgileri
			const float V = (y + 0.5f) / N;
			const float A = FMath::Pow(V, 2.2f) * (0.75f + 0.25f * ((y / 3) % 2));
			for (int32 x = 0; x < 4; ++x)
			{
				uint8* P = &Px[(y * 4 + x) * 4];
				P[0] = 200; P[1] = 255; P[2] = 120; P[3] = (uint8)FMath::RoundToInt(FMath::Clamp(A, 0.f, 1.f) * 200.f);
			}
		}
		UTexture2D* Tex = UTexture2D::CreateTransient(4, N, PF_B8G8R8A8, NAME_None, TConstArrayView64<uint8>(Px.GetData(), Px.Num()));
		Tex->UpdateResource();
		PuffTextures.Add(Tex);
		ScanCurtain = TexPlane(B, FVector(0, 0, BoardTopZ + 6.f), FRotator(0.f, -45.f, 90.f), FVector2D(100.f, 12.f), Tex, true);
		ScanCurtain->SetVisibility(false);
	}

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
	// Kendi kökü var: tahta dönerken ve hafif esintide sallanır (TickFeedbackFx)
	USceneComponent* T = Node(Parent, Loc);
	Part(T, CylinderMesh, FVector(0, 0, 3.f * Scale), FVector(2.4f, 2.4f, 6.f) * Scale, RGB(140, 100, 66));
	Part(T, SphereMesh, FVector(0, 0, 10.f * Scale), FVector(11.f, 11.f, 10.f) * Scale, RGB(120, 170, 100));
	TreeRoots.Add(T);
}

void ADBWorld::BuildBuilding(int32 Index)
{
	const FDBBuildingDef& D = FDBContent::Get().Buildings[Index];
	// Bina tasarım ölçüsünde (≈20 cm) kurulur, kök BuildingScale ile 2×2 karoya küçülür
	USceneComponent* Root = Node(BoardRoot, LotCenter(D.Lot));
	Root->SetRelativeScale3D(FVector(BuildingScale));
	BuildingRoots.Add(Root);
	PopTime.Add(0.f);
	JiggleTime.Add(-1.f);
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
	// Denizin üstünde duman yok (Andaç: denizdeki kahverengi kalksın); tsunami pusu mavi, o denize taşabilir
	const bool bSeaOk = Tint == TintForHazard(TEXT("Tsunami"));
	const int32 NPuff = VfxLevel >= 2 ? 6 : (VfxLevel == 1 ? 3 : 0);
	for (int32 k = 0; k < NPuff; ++k)
	{
		const float Ang = k * 2.f * PI / 6.f + Building;
		FVector Base = C + FVector(FMath::Cos(Ang) * 10.f, FMath::Sin(Ang) * 10.f, 4.f + k * 2.4f);
		const float Size = 18.f + (k % 3) * 4.f;
		// İlk denemede bulutun kenarı denize taşıyordu (yarıçap + salınım payı); yüksekteki bulut kameradan bakınca
		// daha da arkada (denizin üstünde) görünüyordu (yükseklik payı). Denize bakan taraftakiler karaya döner.
		const float Limit = SeaStartY() - Size * 0.5f - 3.f - (4.f + k * 2.4f) * 0.8f;
		if (!bSeaOk && Base.Y > Limit)
		{
			Base.Y = FMath::Min(Limit, C.Y - (Base.Y - C.Y));
		}
		const int32 Ix = MakePuff(Building, Base, Size, Tint);
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
	static int32 Skip = 0;
	if (VfxLevel == 0 || (VfxLevel == 1 && (++Skip & 1)))
	{
		return;
	}
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
	FVector CamLoc = Focus - Rot.Vector() * ViewDist;
	// Başarı anında kısa sarsıntı (yumuşak, 0,4 sn'de söner)
	ShakeAmp = FMath::FInterpTo(ShakeAmp, 0.f, Dt, 7.f);
	if (ShakeAmp > 0.01f)
	{
		const float T = GetWorld()->GetTimeSeconds();
		CamLoc += FVector(FMath::Sin(T * 61.f), FMath::Cos(T * 53.f), FMath::Sin(T * 47.f) * 0.6f) * ShakeAmp * 2.2f;
	}
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
		if (BrokenCovers.IsValidIndex(Ix * 2 + 1))
		{
			BrokenCovers[Ix * 2]->SetVisibility(false);
			BrokenCovers[Ix * 2 + 1]->SetVisibility(false);
		}
		const FVector W = CellLocal(Cell) + FVector(0, 0, 2.f);
		for (int32 i = 0; i < 6; ++i)
		{
			SpawnGrain(W + FVector(FMath::FRandRange(-3.f, 3.f), FMath::FRandRange(-3.f, 3.f), 0.f), FMath::FRandRange(20.f, 35.f));
		}
	}
}

USceneComponent* ADBWorld::MakeHydrant(int32 Building, FIntPoint Cell, bool bCollide)
{
	// Musluk binaya bağlı (tıklanınca o binanın sorunu), ölçeği geri alınır: çocukları cm ile kurulur
	USceneComponent* Root = BuildingRoots[Building];
	const FVector Local = CellLocal(Cell) + FVector(2.4f, 0.f, 0.f);
	USceneComponent* Hy = Node(Root, (Local - Root->GetRelativeLocation()) / BuildingScale);
	Hy->SetRelativeScale3D(FVector(1.f / BuildingScale));
	const FLinearColor Red = RGB(214, 64, 52);
	Part(Hy, CylinderMesh, FVector(0, 0, 2.2f), FVector(2.4f, 2.4f, 4.4f), Red, FRotator::ZeroRotator, bCollide);
	Part(Hy, SphereMesh, FVector(0, 0, 4.5f), FVector(2.6f, 2.6f, 2.f), RGB(180, 48, 40), FRotator::ZeroRotator, bCollide);
	Part(Hy, CylinderMesh, FVector(0, 0, 2.8f), FVector(1.f, 1.f, 4.f), Red, FRotator(0.f, 0.f, 90.f), bCollide);
	Part(Hy, CylinderMesh, FVector(0, 0, 0.3f), FVector(3.2f, 3.2f, 0.6f), RGB(150, 150, 156), FRotator::ZeroRotator, bCollide);
	return Hy;
}

void ADBWorld::BuildHydrant()
{
	const FDBContent& C = FDBContent::Get();
	if (BuildingRoots.IsValidIndex(C.HydrantBuilding))
	{
		HydrantRoot = MakeHydrant(C.HydrantBuilding, C.HydrantCell, true);
		// Bozuk: eğik duruyor, dibinde su sızıntısı
		HydrantRoot->SetRelativeRotation(FRotator(0.f, 0.f, 24.f));
		HydrantLeak = Part(BoardRoot, CylinderMesh, CellLocal(C.HydrantCell) + FVector(2.4f, 1.6f, 0.12f), FVector(6.f, 7.f, 0.2f), RGB(104, 164, 218));
	}
	// Yeni musluğun yeri: önlem uygulanınca yerden çıkar
	if (BuildingRoots.IsValidIndex(C.NewHydrantBuilding))
	{
		NewHydrantRoot = MakeHydrant(C.NewHydrantBuilding, C.NewHydrantCell, false);
		NewHydrantRoot->SetVisibility(false, true);
	}
}

void ADBWorld::PlaceNewHydrant()
{
	if (!NewHydrantRoot)
	{
		return;
	}
	NewHydrantRoot->SetVisibility(true, true);
	NewHydrantT = 0.f;
	const FVector W = BoardRoot->GetComponentTransform().InverseTransformPosition(NewHydrantRoot->GetComponentLocation());
	for (int32 i = 0; i < 8; ++i)
	{
		SpawnGrain(W + FVector(0, 0, 1.f), FMath::FRandRange(25.f, 45.f));
	}
}

void ADBWorld::TurnBoard(int32 Dir)
{
	if (IsBoardTurning())
	{
		TurnQueued = FMath::Clamp(TurnQueued + Dir, -1, 1);   // dönerken basılan bir sonraki dönüş olur
		return;
	}
	TurnFrom = BoardYaw;
	TurnTo = BoardYaw + 90.f * FMath::Sign(Dir);
	TurnT = 0.f;
	bSplashPlayed = false;
	if (SfxWave)
	{
		SfxWave->Play(EDBSfx::Lift, 0.8f, 0.f, FMath::FRandRange(0.95f, 1.05f));
	}
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
		CarHonkT.Add(-100.f);
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
	if (ScanCurtain) ScanCurtain->SetVisibility(true);
}

void ADBWorld::StopScan()
{
	bScanning = false;
	ScanS = HalfDiag() + 1.f;
	ScanBeam->SetVisibility(false);
	if (ScanCurtain) ScanCurtain->SetVisibility(false);
}

float ADBWorld::SweepCoord(int32 Building) const
{
	if (!BuildingRoots.IsValidIndex(Building))
	{
		return 0.f;
	}
	FVector P = BuildingRoots[Building]->GetRelativeLocation();   // tahta dönebiliyor: tarama tahtanın kendi ekseninde
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

void ADBWorld::JiggleBuilding(int32 Building)
{
	if (JiggleTime.IsValidIndex(Building))
	{
		JiggleTime[Building] = 0.f;
		for (FSway& W : Sways)
		{
			if (W.Root == BuildingRoots[Building])
			{
				W.Omega += 55.f;
			}
		}
	}
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
	if (PageHinge)
	{
		PageAngle = FMath::FInterpConstantTo(PageAngle, PageTarget, Dt, PageFlipDegPerSec);
		// Sayfa hep hafifçe nefes alır; rüzgarda (Gust) alt kenarı duvardan kalkıp çırpınır. Hep duvardan dışarı:
		// içeri dönse sayfa duvara gömülürdü.
		FlutterT += Dt;
		GustAmp = FMath::FInterpTo(GustAmp, 0.f, Dt, 0.9f);
		const float Calm = 0.6f * (0.5f + 0.5f * FMath::Sin(FlutterT * 1.3f));
		const float Wind = GustAmp * 9.f * (0.55f + 0.45f * FMath::Sin(FlutterT * 11.f + FMath::Sin(FlutterT * 3.7f) * 2.f));
		PageHinge->SetRelativeRotation(FRotator(0.f, 0.f, PageSign * (PageAngle + Calm + Wind)));
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
		const float Throb = 1.f + 0.35f * FMath::Sin(ScanT * 40.f);
		ScanBeam->SetRelativeScale3D(FVector(Len / 100.f, 0.03f * Throb, 0.02f * Throb));
		if (ScanCurtain)
		{
			const float Hgt = 10.f + 4.f * FMath::Sin(ScanT * 9.f);
			ScanCurtain->SetRelativeLocation(SweepDir * ScanS + FVector(0, 0, BoardTopZ + 1.f + Hgt * 0.5f));
			ScanCurtain->SetRelativeScale3D(FVector(Len / 100.f, Hgt / 100.f, 1.f));
		}
		if (A >= 1.f)
		{
			ScanBeam->SetVisibility(false);
			if (ScanCurtain) ScanCurtain->SetVisibility(false);
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
			if (P.Base.Y > SeaStartY() - P.Size * 0.5f - 3.f)
			{
				P.Target = 0.f;   // denizin üstünde kum bulutu yok
			}
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
			const FVector At = BeamPos + Across * FMath::FRandRange(-Half, Half) + FVector(0, 0, BoardTopZ + 1.f);
			if (At.Y < SeaStartY() - 2.f)
			{
				SpawnGrain(At, FMath::FRandRange(25.f, 50.f));
			}
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
		// Korna: araba iki kez zıplar
		float Hop = 0.f;
		if (CarHonkT.IsValidIndex(i))
		{
			const float Ht = GetWorld()->GetTimeSeconds() - CarHonkT[i];
			Hop = (Ht >= 0.f && Ht < 0.5f) ? FMath::Abs(FMath::Sin(Ht * 2.f * PI * 2.f)) * 2.2f * (1.f - Ht / 0.5f) : 0.f;
		}
		CarRoots[i]->SetRelativeLocationAndRotation(FMath::Lerp(From, To, Cr.T) + Right * 1.7f + FVector(0, 0, Hop), Dir.Rotation());
	}

	// Musluk tamiri: doğrulur
	if (HydrantFixT >= 0.f && HydrantRoot)
	{
		HydrantFixT += Dt;
		const float F = FMath::Clamp(HydrantFixT / 0.6f, 0.f, 1.f);
		HydrantRoot->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Lerp(24.f, 0.f, F) + FMath::Sin(HydrantFixT * 30.f) * 4.f * (1.f - F)));
	}

	// Yeni yangın musluğu yerden çıkar
	if (NewHydrantT >= 0.f && NewHydrantRoot)
	{
		NewHydrantT += Dt;
		const float F = FMath::Clamp(NewHydrantT / 0.45f, 0.f, 1.f);
		const float Sc = FMath::Lerp(0.2f, 1.f, F) + 0.25f * FMath::Sin(F * PI);
		NewHydrantRoot->SetRelativeScale3D(FVector(1.f, 1.f, Sc) / BuildingScale);
		if (F >= 1.f)
		{
			NewHydrantT = -1.f;
		}
	}

	// Tahta dönüşü: masadan kalkar, havada 90° döner, yerine iner
	if (TurnT >= 0.f && BoardRoot)
	{
		TurnT += Dt;
		const float A = FMath::Clamp(TurnT / FMath::Max(0.2f, TurnSeconds), 0.f, 1.f);
		const float Up = FMath::SmoothStep(0.f, 0.28f, A) * (1.f - FMath::SmoothStep(0.72f, 1.f, A));
		const float Spin = FMath::SmoothStep(0.18f, 0.82f, A);
		BoardYaw = FMath::Lerp(TurnFrom, TurnTo, Spin);
		BoardRoot->SetRelativeLocationAndRotation(FVector(0.f, 0.f, TurnLiftCm * Up), FRotator(0.f, BoardYaw, 0.f));
		// Havadayken denizden su taşar (dönüşün ortası)
		if (A > 0.3f && A < 0.78f)
		{
			DropSpawn += Dt * 70.f;
			while (DropSpawn >= 1.f)
			{
				DropSpawn -= 1.f;
				SpawnDrop();
			}
			if (!bSplashPlayed && SfxWave)
			{
				SfxWave->Play(EDBSfx::Splash, 0.7f, 0.25f);
				bSplashPlayed = true;
			}
		}
		if (A >= 1.f)
		{
			BoardYaw = FRotator::NormalizeAxis(TurnTo);
			BoardRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, BoardYaw, 0.f));
			TurnT = -1.f;
			// İniş: tok ses, tahtadakiler sarsılıp yaylanır
			if (SfxWave)
			{
				SfxWave->Play(EDBSfx::Land, 0.85f);
			}
			for (FSway& W : Sways)
			{
				W.Omega += FMath::FRandRange(-1.f, 1.f) * 70.f * W.Gain;
			}
			if (TurnQueued != 0)
			{
				const int32 Q = TurnQueued;
				TurnQueued = 0;
				TurnBoard(Q);
			}
		}
	}

	{
		float D = BoardYaw - PrevYaw;
		D = FRotator::NormalizeAxis(D);
		PrevYaw = BoardYaw;
		TickFeedbackFx(Dt, Dt > 0.f ? D / Dt : 0.f);
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
		// Karo üstü 13,3–13,6: dalga dibinde bile tahtanın kahverengi düzlüğünün (üstü 13,0) üstünde kalır.
		// Önceden en alçakta 13,0'a iniyordu, iki yüzey çakışıp denizde kahverengi kareler yanıp sönüyordu.
		SeaTiles[i]->SetRelativeLocation(FVector(L.X, L.Y, 13.05f + 0.15f * FMath::Sin(WaveT * 2.2f + L.X * 0.12f + L.Y * 0.05f)));
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

	// Doğru uygulama: kısa zıplama; seçilen bina: jöle gibi esneme (genişler-uzar, sönerek). İkisi birlikte çarpılır.
	for (int32 i = 0; i < PopTime.Num(); ++i)
	{
		const bool bPop = PopTime[i] > 0.f;
		const bool bJig = JiggleTime.IsValidIndex(i) && JiggleTime[i] >= 0.f;
		if (!bPop && !bJig)
		{
			continue;
		}
		float Uni = 1.f;
		if (bPop)
		{
			PopTime[i] += Dt;
			const float T = PopTime[i] / 0.45f;
			Uni = T < 1.f ? 1.f + 0.22f * FMath::Sin(T * PI) : 1.f;
			if (T >= 1.f)
			{
				PopTime[i] = 0.f;
			}
		}
		float Sq = 0.f;
		if (bJig)
		{
			JiggleTime[i] += Dt;
			const float T = JiggleTime[i];
			Sq = 0.16f * FMath::Sin(T * 30.f) * FMath::Exp(-T * 5.5f);
			if (T > 0.8f)
			{
				JiggleTime[i] = -1.f;
				Sq = 0.f;
			}
		}
		BuildingRoots[i]->SetRelativeScale3D(FVector(Uni * (1.f + Sq), Uni * (1.f + Sq), Uni * (1.f - Sq)) * BuildingScale);
	}

	// Deprem
	if (bQuaking && BoardRoot)
	{
		QuakeT += Dt;
		const float A = FMath::Clamp(QuakeT / QuakeDuration, 0.f, 1.f);
		const float Env = FMath::Sin(A * PI);
		const float Amp = 2.5f * Env;
		BoardRoot->SetRelativeLocation(FVector(FMath::Sin(QuakeT * 41.f) * Amp, FMath::Cos(QuakeT * 33.f) * Amp, 0.f));
		BoardRoot->SetRelativeRotation(FRotator(FMath::Sin(QuakeT * 23.f) * 1.2f * Env, BoardYaw, FMath::Cos(QuakeT * 29.f) * 1.2f * Env));

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
			BoardRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(0.f, BoardYaw, 0.f));
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

// ---------------------------------------------------------------- geri bildirim: su sıçraması, sallanma

void ADBWorld::BuildFeedbackFx()
{
	// Damlalar dünyada (tahtaya bağlı değil): tahtadan kopup masaya düşer
	for (int32 i = 0; i < 48; ++i)
	{
		UStaticMeshComponent* C = Part(RootComponent, SphereMesh, FVector::ZeroVector, FVector(1.7f), RGB(110, 170, 222));
		C->SetVisibility(false);
		DropComps.Add(C);
		Drops.AddDefaulted();
	}
	// Masaya düşen damlanın bıraktığı küçük ıslaklık: büyür, sonra kurur
	for (int32 i = 0; i < 14; ++i)
	{
		UStaticMeshComponent* C = Part(RootComponent, CylinderMesh, FVector::ZeroVector, FVector(3.f, 3.f, 0.08f), RGB(122, 156, 186));
		C->SetVisibility(false);
		TablePuddleComps.Add(C);
		TablePuddles.AddDefaulted();
	}
	// Sallananlar: binalar (kasabalılar kökleriyle birlikte) ve ağaçlar
	const FTransform BT = BoardRoot->GetComponentTransform();
	auto Add = [this, &BT](USceneComponent* R, float Gain)
	{
		FSway W;
		W.Root = R;
		const FVector L = BT.InverseTransformPosition(R->GetComponentLocation());
		const FVector2D P(L.X, L.Y);
		W.Radius = P.Size();
		W.Axis = W.Radius > 1.f ? P / W.Radius : FVector2D(1.f, 0.f);
		W.Gain = Gain;
		W.Phase = FMath::FRandRange(0.f, 2.f * PI);
		Sways.Add(W);
	};
	for (USceneComponent* R : BuildingRoots)
	{
		Add(R, 1.f);
	}
	for (USceneComponent* R : TreeRoots)
	{
		Add(R, 1.6f);
	}
}

void ADBWorld::SpawnDrop()
{
	static int32 Skip = 0;
	if (VfxLevel == 0 || (VfxLevel == 1 && (++Skip & 1)))
	{
		return;
	}
	const int32 Ix = Drops.IndexOfByPredicate([](const FDrop& D) { return !D.bOn; });
	if (Ix == INDEX_NONE || !BoardRoot)
	{
		return;
	}
	// Denizin dış kenarından (tahtanın içinde) çıkar; dönüşün savurmasıyla dışarı ve biraz yukarı
	const float Half = GridN * CellCm * 0.5f;
	const FVector Local(FMath::FRandRange(-Half + 4.f, Half - 4.f), FMath::FRandRange(Half - 5.f, Half - 0.5f), BoardTopZ + 0.5f);
	const FTransform T = BoardRoot->GetComponentTransform();
	const FVector P = T.TransformPosition(Local);
	const FVector Out = T.TransformVector(FVector(0.f, 1.f, 0.f)).GetSafeNormal();
	const float W = FMath::DegreesToRadians((TurnTo - TurnFrom) / FMath::Max(0.2f, TurnSeconds)) * 1.6f;
	const FVector Tangent(-P.Y * W, P.X * W, 0.f);
	FDrop& D = Drops[Ix];
	D.bOn = true;
	D.P = P;
	D.V = Out * FMath::FRandRange(30.f, 75.f) + Tangent * FMath::FRandRange(0.2f, 0.45f) + FVector(0.f, 0.f, FMath::FRandRange(15.f, 45.f));
	DropComps[Ix]->SetWorldLocation(P);
	DropComps[Ix]->SetVisibility(true);
}

void ADBWorld::TickFeedbackFx(float Dt, float YawVel)
{
	if (Dt <= 0.f)
	{
		return;
	}
	// Damlalar: yerçekimi; tahtanın üstüne düşen emilir, masaya düşen ıslaklık bırakır
	const float Half = BoardCm * 0.5f;
	for (int32 i = 0; i < Drops.Num(); ++i)
	{
		FDrop& D = Drops[i];
		if (!D.bOn)
		{
			continue;
		}
		D.V.Z -= 520.f * Dt;
		D.P += D.V * Dt;
		const FVector L = BoardRoot->GetComponentTransform().InverseTransformPosition(D.P);
		const bool bOverBoard = FMath::Abs(L.X) < Half && FMath::Abs(L.Y) < Half;
		const float Floor = bOverBoard ? BoardRoot->GetComponentLocation().Z + BoardTopZ - 1.f : 0.05f;
		if (D.P.Z <= Floor)
		{
			D.bOn = false;
			DropComps[i]->SetVisibility(false);
			if (!bOverBoard && (++DropsLanded % 3) == 0)
			{
				const int32 Pi = TablePuddles.IndexOfByPredicate([](const FTablePuddle& Pd) { return Pd.T < 0.f; });
				if (Pi != INDEX_NONE)
				{
					TablePuddles[Pi].T = 0.f;
					TablePuddles[Pi].Size = FMath::FRandRange(2.5f, 5.5f);
					TablePuddleComps[Pi]->SetWorldLocation(FVector(D.P.X, D.P.Y, 0.04f));
					TablePuddleComps[Pi]->SetVisibility(true);
				}
			}
			continue;
		}
		DropComps[i]->SetWorldLocation(D.P);
	}
	// Islaklık: 0,25 sn'de yayılır, 5 sn'de kurur (sıfıra ölçeklenmez, küçülünce gizlenir)
	for (int32 i = 0; i < TablePuddles.Num(); ++i)
	{
		FTablePuddle& Pd = TablePuddles[i];
		if (Pd.T < 0.f)
		{
			continue;
		}
		Pd.T += Dt;
		const float Grow = FMath::Clamp(Pd.T / 0.25f, 0.f, 1.f);
		const float Dry = 1.f - FMath::Clamp((Pd.T - 2.f) / 3.f, 0.f, 1.f);
		const float Sz = Pd.Size * FMath::Max(Grow, 0.15f) * FMath::Max(Dry, 0.05f);
		if (Dry <= 0.05f)
		{
			Pd.T = -1.f;
			TablePuddleComps[i]->SetVisibility(false);
			continue;
		}
		TablePuddleComps[i]->SetRelativeScale3D(FVector(Sz / 100.f, Sz * 0.8f / 100.f, 0.08f / 100.f));
	}
	// Sallanma: yay-sönüm. Dönüşte merkezkaç yönüne yatar, inince yaylanır; durgunken hafif esinti.
	const float Lean = FMath::Clamp(-YawVel * 0.03f, -7.f, 7.f);
	for (FSway& W : Sways)
	{
		if (!W.Root)
		{
			continue;
		}
		const float Target = Lean * W.Gain * FMath::Clamp(W.Radius / 45.f, 0.3f, 1.2f)
			+ 0.45f * W.Gain * FMath::Sin(FlutterT * 1.4f + W.Phase);
		W.Omega += (140.f * (Target - W.Theta) - 6.5f * W.Omega) * Dt;
		W.Theta += W.Omega * Dt;
		W.Theta = FMath::Clamp(W.Theta, -12.f, 12.f);
		// Merkezden dışarı eksen etrafında döner: tepe teğet yönde (dönüş yönünde) yatar
		W.Root->SetRelativeRotation(FQuat(FVector(W.Axis.X, W.Axis.Y, 0.f), FMath::DegreesToRadians(W.Theta)));
	}

	// Böcekler (Andaç: kuşlar yerine): arada sinek / kelebek geçer, uğur böceği yollarda yürür, örümcek ağından inip
	// tahtayı geçer. Bulut gölgeleri tahtada süzülür.
	const bool bLife = VfxLevel > 0 && bLifeVisible;
	TickBugs(Dt, bLife);
	// Yeni konan parçalar (çanta, çapraz) yaylanarak büyür
	for (int32 i = Grows.Num() - 1; i >= 0; --i)
	{
		FGrow& G = Grows[i];
		G.T += Dt;
		const float U = FMath::Clamp(G.T / 0.55f, 0.f, 1.f);
		const float Sc = U >= 1.f ? 1.f : FMath::Max(0.05f, 1.f + FMath::Exp(-6.f * U) * FMath::Sin(U * 12.f - PI * 0.5f));
		if (G.Comp)
		{
			G.Comp->SetRelativeScale3D(FVector(Sc));
		}
		if (U >= 1.f)
		{
			Grows.RemoveAt(i);
		}
	}
	for (int32 i = 0; i < CloudShadows.Num(); ++i)
	{
		if (CloudShadows[i]->IsVisible() != bLife)
		{
			CloudShadows[i]->SetVisibility(bLife);
		}
		const float Span = BoardCm + 80.f;
		const float X = FMath::Fmod(FlutterT * (3.2f + i) + i * Span * 0.55f, Span) - Span * 0.5f;
		CloudShadows[i]->SetRelativeLocation(FVector(X, -30.f + i * 55.f, BoardTopZ + 0.35f));
	}
}

void ADBWorld::BuildLife()
{
	const float Z0 = BoardTopZ;
	// Sinek: koyu gövde, iri kırmızımsı gözler, iki saydamımsı kanat (hızlı çırpar)
	Fly = Node(BoardRoot, FVector(0, 0, 40.f));
	Part(Fly, SphereMesh, FVector::ZeroVector, FVector(2.2f, 1.4f, 1.3f), RGB(46, 48, 56));
	Part(Fly, SphereMesh, FVector(1.1f, 0.45f, 0.25f), FVector(0.8f, 0.8f, 0.8f), RGB(150, 50, 44));
	Part(Fly, SphereMesh, FVector(1.1f, -0.45f, 0.25f), FVector(0.8f, 0.8f, 0.8f), RGB(150, 50, 44));
	for (int32 s = 0; s < 2; ++s)
	{
		USceneComponent* Wg = Node(Fly, FVector(0.f, s ? 0.3f : -0.3f, 0.5f));
		Part(Wg, SphereMesh, FVector(-0.4f, s ? 1.1f : -1.1f, 0.f), FVector(1.6f, 2.2f, 0.12f), RGB(214, 226, 236));
		FlyWings.Add(Wg);
	}
	// Kelebek: ince gövde, iki büyük renkli kanat (yavaş çırpar)
	Butterfly = Node(BoardRoot, FVector(0, 0, 35.f));
	Part(Butterfly, CylinderMesh, FVector::ZeroVector, FVector(0.6f, 0.6f, 3.f), RGB(60, 46, 40), FRotator(90.f, 0.f, 0.f));
	for (int32 s = 0; s < 2; ++s)
	{
		USceneComponent* Wg = Node(Butterfly, FVector(0.f, s ? 0.2f : -0.2f, 0.f));
		Part(Wg, SphereMesh, FVector(0.5f, s ? 1.7f : -1.7f, 0.f), FVector(2.6f, 3.2f, 0.12f), RGB(246, 176, 72));
		Part(Wg, SphereMesh, FVector(-1.1f, s ? 1.3f : -1.3f, 0.f), FVector(1.8f, 2.2f, 0.12f), RGB(232, 128, 104));
		Part(Wg, SphereMesh, FVector(0.6f, s ? 2.1f : -2.1f, 0.05f), FVector(0.8f, 0.8f, 0.1f), RGB(70, 50, 44));
		ButterflyWings.Add(Wg);
	}
	// Uğur böceği: kırmızı kubbe, siyah baş ve orta çizgi, benekler (yollarda yürür)
	Ladybug = Node(BoardRoot, CellLocal(FIntPoint(2, 2)));
	Part(Ladybug, SphereMesh, FVector(0, 0, 0.7f), FVector(3.2f, 2.7f, 1.8f), RGB(214, 52, 44));
	Part(Ladybug, SphereMesh, FVector(1.6f, 0, 0.6f), FVector(1.3f, 1.5f, 1.1f), RGB(30, 30, 34));
	Part(Ladybug, CubeMesh, FVector(-0.1f, 0, 1.55f), FVector(3.f, 0.18f, 0.1f), RGB(30, 30, 34));
	const FVector Dots[] = { FVector(0.5f, 0.75f, 1.4f), FVector(0.5f, -0.75f, 1.4f), FVector(-0.7f, 0.65f, 1.35f), FVector(-0.7f, -0.65f, 1.35f) };
	for (const FVector& D : Dots)
	{
		Part(Ladybug, SphereMesh, D, FVector(0.55f, 0.55f, 0.3f), RGB(30, 30, 34));
	}
	// Örümcek ağı: tahtanın sağ köşesinde (+X, -Y) iki çöp çubuğun arasında, kameraya bakan düzlemde
	{
		const FVector U = FVector(1.f, -1.f, 0.f).GetSafeNormal();
		WebCenter = FVector(54.f, -54.f, Z0 + 6.f);
		WebRoot = Node(BoardRoot, FVector::ZeroVector);
		const FLinearColor Silk = RGB(240, 240, 246);
		auto Thread = [this, Silk](const FVector& A, const FVector& Bp, float Th)
		{
			const FVector D = Bp - A;
			Part(WebRoot, CylinderMesh, (A + Bp) * 0.5f, FVector(Th, Th, D.Size()), Silk, FRotationMatrix::MakeFromZ(D.GetSafeNormal()).Rotator());
		};
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector Base = WebCenter + U * 8.f * s;
			Part(WebRoot, CylinderMesh, FVector(Base.X, Base.Y, 12.f + 8.f), FVector(0.6f, 0.6f, 16.f), RGB(150, 112, 74), FRotator(0.f, 0.f, 4.f * s));
		}
		const int32 Spokes = 9;
		const float Rad = 6.5f;
		TArray<FVector> Prev;
		for (int32 r = 1; r <= 4; ++r)
		{
			TArray<FVector> Ring;
			for (int32 k = 0; k < Spokes; ++k)
			{
				const float A = 2.f * PI * k / Spokes + 0.2f;
				const float Rr = Rad * r / 4.f * (1.f + 0.08f * FMath::Sin(k * 2.1f));
				Ring.Add(WebCenter + U * FMath::Cos(A) * Rr + FVector(0, 0, FMath::Sin(A) * Rr));
			}
			for (int32 k = 0; k < Spokes; ++k)
			{
				Thread(Ring[k], Ring[(k + 1) % Spokes], 0.1f);   // halka
				Thread(r == 1 ? WebCenter : Prev[k], Ring[k], 0.12f);   // ışın
			}
			Prev = Ring;
		}
		// Dış ışınlar çubuklara ve tahtaya bağlanır
		Thread(Prev[0], WebCenter + U * 8.f + FVector(0, 0, 4.f), 0.1f);
		Thread(Prev[Spokes / 2], WebCenter - U * 8.f + FVector(0, 0, 3.f), 0.1f);
		Thread(Prev[3 * Spokes / 4], FVector(WebCenter.X, WebCenter.Y, 12.f), 0.1f);
	}
	// Örümcek: iki parçalı gövde, sekiz bacak (yürürken sırayla kıpırdar)
	Spider = Node(BoardRoot, WebCenter);
	Part(Spider, SphereMesh, FVector(-0.9f, 0, 1.4f), FVector(2.4f, 2.1f, 1.8f), RGB(44, 36, 34));
	Part(Spider, SphereMesh, FVector(0.8f, 0, 1.2f), FVector(1.3f, 1.3f, 1.1f), RGB(60, 48, 44));
	Part(Spider, SphereMesh, FVector(1.35f, 0.3f, 1.45f), FVector(0.35f, 0.35f, 0.35f), RGB(250, 250, 250));
	Part(Spider, SphereMesh, FVector(1.35f, -0.3f, 1.45f), FVector(0.35f, 0.35f, 0.35f), RGB(250, 250, 250));
	for (int32 l = 0; l < 8; ++l)
	{
		const float Side = l < 4 ? 1.f : -1.f;
		const float Fw = 0.9f - 0.6f * (l % 4);
		USceneComponent* Leg = Node(Spider, FVector(0.4f * Fw, 0.4f * Side, 1.3f));
		Leg->SetRelativeRotation(FRotator(0.f, Side * (90.f - 28.f * (l % 4 - 1.5f)), 0.f));
		// Bacak: dışarı ve aşağı eğik ince çubuk (yerel +X dışarı)
		Part(Leg, CylinderMesh, FVector(1.4f, 0.f, -0.5f), FVector(0.22f, 0.22f, 3.2f), RGB(40, 32, 30), FRotator(-70.f, 0.f, 0.f));
		SpiderLegs.Add(Leg);
	}
	// Yol: ağdan tahtaya iner, 2. sıradaki yoldan karşı kenara yürür, geri döner
	{
		const float Row = -GridN * CellCm * 0.5f + 2.5f * CellCm - 2.f;
		SpiderPath = { WebCenter, FVector(WebCenter.X, WebCenter.Y, 12.2f), FVector(50.f, Row, Z0 + 0.1f), FVector(-58.f, Row, Z0 + 0.1f) };
	}
	// Bulut gölgesi: yumuşak, çok saydam koyu leke (tahtanın üstünde süzülür; binalar içinden geçer)
	const int32 N = 64;
	TArray<uint8> Px;
	Px.SetNumUninitialized(N * N * 4);
	for (int32 y = 0; y < N; ++y)
	{
		for (int32 x = 0; x < N; ++x)
		{
			const float Dx = (x + 0.5f) / N * 2.f - 1.f, Dy = (y + 0.5f) / N * 2.f - 1.f;
			const float R = FMath::Sqrt(Dx * Dx * 0.7f + Dy * Dy * 1.3f);
			const float Bump = 0.88f + 0.12f * FMath::Sin(x * 0.45f) * FMath::Cos(y * 0.33f);
			const float A = FMath::Clamp(FMath::Pow(FMath::Max(0.f, 1.f - R), 1.2f) * 0.2f * Bump, 0.f, 1.f);
			uint8* P = &Px[(y * N + x) * 4];
			P[0] = 40; P[1] = 34; P[2] = 30; P[3] = (uint8)FMath::RoundToInt(A * 255.f);
		}
	}
	UTexture2D* Tex = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8, NAME_None, TConstArrayView64<uint8>(Px.GetData(), Px.Num()));
	Tex->UpdateResource();
	PuffTextures.Add(Tex);
	CloudMid = UMaterialInstanceDynamic::Create(PuffMaterial, this);
	CloudMid->SetTextureParameterValue(TEXT("SlateUI"), Tex);
	for (int32 i = 0; i < 2; ++i)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(PlaneMesh);
		C->SetupAttachment(BoardRoot);
		C->SetRelativeScale3D(FVector(0.55f + 0.15f * i, 0.4f, 1.f));
		C->SetMaterial(0, CloudMid);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->RegisterComponent();
		CloudShadows.Add(C);
	}
}

// ---------------------------------------------------------------- böcekler (Andaç, 17 Ekim ~20.00: kuşlar yerine)

void ADBWorld::TickBugs(float Dt, bool bLife)
{
	BugT += Dt;
	const float T = BugT;
	auto Show = [](USceneComponent* C, bool b) { if (C && C->IsVisible() != b) C->SetVisibility(b, true); };

	// Sinek: 30 sn'de bir 13 sn uğrar, kasabanın üstünde zikzak çizer, ara ara havada durur
	const float FlyCycle = FMath::Fmod(T, 30.f);
	const bool bFly = bLife && FlyCycle < 13.f;
	Show(Fly, bFly);
	if (bFly && Fly)
	{
		const float A = FlyCycle;
		const float Hover = 0.5f + 0.5f * FMath::Sin(A * 0.9f);   // yavaşlayıp hızlanır
		const FVector P(FMath::Sin(A * 0.7f) * 42.f + FMath::Sin(A * 3.3f) * 6.f * Hover, FMath::Cos(A * 0.53f) * 36.f + FMath::Sin(A * 2.9f) * 5.f * Hover,
			30.f + 7.f * FMath::Sin(A * 1.7f) + 2.f * FMath::Sin(A * 11.f));
		const FVector V(0.7f * 42.f * FMath::Cos(A * 0.7f), -0.53f * 36.f * FMath::Sin(A * 0.53f), 0.f);
		Fly->SetRelativeLocationAndRotation(P, V.Rotation());
		const float Flap = 50.f * FMath::Sin(T * 95.f);
		FlyWings[0]->SetRelativeRotation(FRotator(0.f, 0.f, Flap));
		FlyWings[1]->SetRelativeRotation(FRotator(0.f, 0.f, -Flap));
	}

	// Kelebek: 40 sn'de bir 15 sn, geniş yavaş kıvrımlar, kanatlar yavaş çırpar
	const float BfCycle = FMath::Fmod(T + 18.f, 40.f);
	const bool bBf = bLife && BfCycle < 15.f;
	Show(Butterfly, bBf);
	if (bBf && Butterfly)
	{
		const float A = BfCycle * 0.35f;
		const FVector P(-70.f + BfCycle / 15.f * 140.f, FMath::Sin(A * 1.3f) * 30.f, 26.f + 6.f * FMath::Sin(A * 2.4f) + 3.f * FMath::Abs(FMath::Sin(T * 5.f)));
		const FVector V(140.f / 15.f, 1.3f * 0.35f * 30.f * FMath::Cos(A * 1.3f), 0.f);
		Butterfly->SetRelativeLocationAndRotation(P, V.Rotation());
		const float Flap = 15.f + 55.f * (0.5f + 0.5f * FMath::Sin(T * 13.f));
		ButterflyWings[0]->SetRelativeRotation(FRotator(0.f, 0.f, Flap));
		ButterflyWings[1]->SetRelativeRotation(FRotator(0.f, 0.f, -Flap));
	}

	// Uğur böceği: yollardan dikdörtgen bir tur (yol kenarında), yavaş; hep tahtada
	Show(Ladybug, bLife);
	if (bLife && Ladybug)
	{
		const float Off = 2.6f;
		const TArray<FVector> Loop = { CellLocal(FIntPoint(2, 2)) + FVector(Off, Off, 0), CellLocal(FIntPoint(8, 2)) + FVector(-Off, Off, 0),
			CellLocal(FIntPoint(8, 8)) + FVector(-Off, -Off, 0), CellLocal(FIntPoint(2, 8)) + FVector(Off, -Off, 0) };
		float Total = 0.f;
		for (int32 i = 0; i < 4; ++i) Total += FVector::Dist(Loop[i], Loop[(i + 1) % 4]);
		float D = FMath::Fmod(T * 3.2f, Total);
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector A = Loop[i], B = Loop[(i + 1) % 4];
			const float L = FVector::Dist(A, B);
			if (D <= L)
			{
				const FVector Dir = (B - A).GetSafeNormal();
				const FVector P = FMath::Lerp(A, B, D / L) + FVector(0, 0, 0.05f + 0.15f * FMath::Abs(FMath::Sin(T * 14.f)));
				Ladybug->SetRelativeLocationAndRotation(P, FRotator(0.f, Dir.Rotation().Yaw + 6.f * FMath::Sin(T * 7.f), 0.f));
				break;
			}
			D -= L;
		}
	}

	// Örümcek: ağında bekler (18 sn), ipiyle tahtaya iner, yoldan karşı kenara yürür, geri gelip ağına tırmanır
	Show(Spider, bLife);
	Show(WebRoot, bLife);
	if (bLife && Spider && SpiderPath.Num() >= 2)
	{
		float Len = 0.f;
		for (int32 i = 0; i + 1 < SpiderPath.Num(); ++i) Len += FVector::Dist(SpiderPath[i], SpiderPath[i + 1]);
		const float Speed = 6.5f;
		const float Wait = 18.f;
		const float Cycle = Wait + 2.f * Len / Speed + 3.f;
		const float C = FMath::Fmod(T + 6.f, Cycle);
		float Dist = 0.f;
		bool bMoving = false;
		if (C < Wait) Dist = 0.f;
		else if (C < Wait + Len / Speed) { Dist = (C - Wait) * Speed; bMoving = true; }
		else if (C < Wait + Len / Speed + 3.f) Dist = Len;   // karşı kenarda durup bakınır
		else { Dist = FMath::Max(0.f, Len - (C - Wait - Len / Speed - 3.f) * Speed); bMoving = true; }
		const bool bBack = C >= Wait + Len / Speed + 3.f;
		FVector P = SpiderPath[0], Dir(1.f, 0.f, 0.f);
		float Rem = Dist;
		for (int32 i = 0; i + 1 < SpiderPath.Num(); ++i)
		{
			const float L = FVector::Dist(SpiderPath[i], SpiderPath[i + 1]);
			if (Rem <= L || i + 2 == SpiderPath.Num())
			{
				P = FMath::Lerp(SpiderPath[i], SpiderPath[i + 1], FMath::Clamp(Rem / FMath::Max(L, 0.01f), 0.f, 1.f));
				Dir = (SpiderPath[i + 1] - SpiderPath[i]).GetSafeNormal2D();
				break;
			}
			Rem -= L;
		}
		if (bBack) Dir = -Dir;
		if (Dir.IsNearlyZero()) Dir = FVector(-1.f, 1.f, 0.f).GetSafeNormal();
		Spider->SetRelativeLocationAndRotation(P, FRotator(0.f, Dir.Rotation().Yaw, 0.f));
		for (int32 l = 0; l < SpiderLegs.Num(); ++l)
		{
			const float Side = l < 4 ? 1.f : -1.f;
			const float Base = Side * (90.f - 28.f * (l % 4 - 1.5f));
			const float Step = bMoving ? 14.f * FMath::Sin(T * 16.f + (l % 2) * PI + (l / 4) * PI) : 3.f * FMath::Sin(T * 2.f + l);
			SpiderLegs[l]->SetRelativeRotation(FRotator(0.f, Base + Step, 0.f));
		}
	}
}

// ---------------------------------------------------------------- fotoğraf albümü, yapılan işin izi, korna

void ADBWorld::TakePhoto(int32 Slot, int32 Building)
{
	if (!BuildingRoots.IsValidIndex(Building) || !BoardCam)
	{
		return;
	}
	if (!PhotoCam)
	{
		PhotoCam = NewObject<USceneCaptureComponent2D>(this);
		PhotoCam->SetupAttachment(RootComponent);
		PhotoCam->bCaptureEveryFrame = false;
		PhotoCam->bCaptureOnMovement = false;
		PhotoCam->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		PhotoCam->FOVAngle = 36.f;
		FPostProcessSettings& PP = PhotoCam->PostProcessSettings;
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMinBrightness = ExposureEV100;
		PP.AutoExposureMaxBrightness = ExposureEV100;
		PP.bOverride_MotionBlurAmount = true;
		PP.MotionBlurAmount = 0.f;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 0.55f;   // eski fotoğraf havası
		PhotoCam->PostProcessBlendWeight = 1.f;
		PhotoCam->RegisterComponent();
	}
	if (Photos.Num() <= Slot)
	{
		Photos.SetNum(Slot + 1);
	}
	if (!Photos[Slot])
	{
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
		RT->RenderTargetFormat = RTF_RGBA8_SRGB;
		RT->ClearColor = FLinearColor(0.9f, 0.85f, 0.75f, 1.f);
		RT->InitAutoFormat(480, 360);
		RT->UpdateResourceImmediate(true);
		Photos[Slot] = RT;
	}
	// Kameranın baktığı yönden, biraz daha alçaktan ve yakından: bina ve önündeki kasabalı kadrajda
	const float Hgt = (BuildingHeights.IsValidIndex(Building) ? BuildingHeights[Building] : 20.f) * BuildingScale;
	const FVector Target = BuildingRoots[Building]->GetComponentLocation() + FVector(0, 0, Hgt * 0.42f);
	const FRotator R(-30.f, BoardCam->GetActorRotation().Yaw, 0.f);
	const float Dist = 46.f + Hgt * 1.7f;
	PhotoCam->SetWorldLocationAndRotation(Target - R.Vector() * Dist, R);
	PhotoCam->TextureTarget = Photos[Slot];
	PhotoCam->CaptureScene();
}

void ADBWorld::PlaceBagProp(int32 Building, const FLinearColor& Color)
{
	if (!BuildingRoots.IsValidIndex(Building))
	{
		return;
	}
	// Kasabalının yanında (tasarım birimi; kasabalı (-15, 7)): sırt çantası, kapak, askı, beyaz şerit
	USceneComponent* Bag = Node(BuildingRoots[Building], FVector(-11.5f, 11.f, 1.f));
	Bag->SetRelativeRotation(FRotator(0.f, -20.f, 0.f));
	const FLinearColor Dark = Color * 0.7f;
	Part(Bag, CubeMesh, FVector(0, 0, 3.4f), FVector(4.6f, 3.2f, 6.8f), Color);
	Part(Bag, CubeMesh, FVector(-0.2f, 0, 6.2f), FVector(5.f, 3.5f, 1.8f), Dark);
	Part(Bag, CubeMesh, FVector(0, -1.75f, 2.6f), FVector(3.f, 0.5f, 2.6f), Dark);
	Part(Bag, CubeMesh, FVector(0, -2.05f, 2.6f), FVector(1.8f, 0.2f, 0.6f), RGB(250, 250, 250));
	Part(Bag, CylinderMesh, FVector(0, 0, 7.6f), FVector(0.6f, 2.4f, 0.6f), Dark, FRotator(0.f, 0.f, 90.f));
	Bag->SetRelativeScale3D(FVector(0.05f));
	Grows.Add({ Bag, 0.f });
}

void ADBWorld::PlaceBraces(int32 Building)
{
	const FDBContent& C = FDBContent::Get();
	if (!BuildingRoots.IsValidIndex(Building) || !C.Buildings.IsValidIndex(Building))
	{
		return;
	}
	// Güçlendirme: kameraya bakan iki yüzde (-X, -Y) çelik çaprazlar + kat kirişleri, köşelerde sarı plakalar
	const FDBBuildingDef& D = C.Buildings[Building];
	const FVector S(FMath::Min(D.Size.X, 20.f), FMath::Min(D.Size.Y, 20.f), D.Size.Z);
	const float Base = 1.f;
	USceneComponent* Br = Node(BuildingRoots[Building], FVector::ZeroVector);
	const FLinearColor Steel = RGB(84, 92, 110);
	const FLinearColor Plate = RGB(246, 196, 64);
	const int32 Bays = FMath::Max(1, FMath::RoundToInt(S.Z / 14.f));
	const float BayH = S.Z / Bays;
	for (int32 Face = 0; Face < 2; ++Face)
	{
		const float W = Face == 0 ? S.Y : S.X;
		const float Ang = FMath::RadiansToDegrees(FMath::Atan2(BayH, W));
		const float L = FMath::Sqrt(W * W + BayH * BayH);
		for (int32 b = 0; b < Bays; ++b)
		{
			const float Zc = Base + BayH * (b + 0.5f);
			for (int32 s = -1; s <= 1; s += 2)
			{
				if (Face == 0)
				{
					Part(Br, CubeMesh, FVector(-S.X * 0.5f - 0.7f, 0.f, Zc), FVector(0.7f, L, 0.9f), Steel, FRotator(0.f, 0.f, s * Ang));
				}
				else
				{
					Part(Br, CubeMesh, FVector(0.f, -S.Y * 0.5f - 0.7f, Zc), FVector(L, 0.7f, 0.9f), Steel, FRotator(s * Ang, 0.f, 0.f));
				}
			}
			const float Zb = Base + BayH * b;
			Part(Br, CubeMesh, Face == 0 ? FVector(-S.X * 0.5f - 0.8f, 0.f, Zb + 0.5f) : FVector(0.f, -S.Y * 0.5f - 0.8f, Zb + 0.5f),
				Face == 0 ? FVector(0.8f, W + 1.f, 1.f) : FVector(W + 1.f, 0.8f, 1.f), Steel);
		}
		Part(Br, CubeMesh, Face == 0 ? FVector(-S.X * 0.5f - 0.8f, 0.f, Base + S.Z - 0.5f) : FVector(0.f, -S.Y * 0.5f - 0.8f, Base + S.Z - 0.5f),
			Face == 0 ? FVector(0.8f, W + 1.f, 1.f) : FVector(W + 1.f, 0.8f, 1.f), Steel);
	}
	for (int32 k = 0; k <= Bays; ++k)
	{
		Part(Br, CubeMesh, FVector(-S.X * 0.5f - 0.9f, -S.Y * 0.5f - 0.9f, Base + BayH * k), FVector(1.8f, 1.8f, 1.8f), Plate);
	}
	Br->SetRelativeScale3D(FVector(0.05f));
	Grows.Add({ Br, 0.f });
}

int32 ADBWorld::CarAtScreen(const APlayerController* PC, const FVector2D& Screen, float RadiusPx) const
{
	int32 Best = -1;
	float BestD = RadiusPx;
	for (int32 i = 0; i < CarRoots.Num(); ++i)
	{
		FVector2D P;
		if (PC && CarRoots[i] && PC->ProjectWorldLocationToScreen(CarRoots[i]->GetComponentLocation() + FVector(0, 0, 1.5f), P))
		{
			const float D = FVector2D::Distance(P, Screen);
			if (D < BestD)
			{
				BestD = D;
				Best = i;
			}
		}
	}
	return Best;
}

void ADBWorld::HonkCar(int32 Car)
{
	if (!CarHonkT.IsValidIndex(Car))
	{
		return;
	}
	CarHonkT[Car] = GetWorld()->GetTimeSeconds();
	if (SfxWave)
	{
		// Arabanın ekrandaki yerine göre sağ / sol
		const float Pan = FMath::Clamp(CarRoots[Car]->GetComponentLocation().Y / 80.f, -0.8f, 0.8f);
		SfxWave->Play(EDBSfx::Horn, 0.9f, Pan, FMath::FRandRange(0.92f, 1.08f));
	}
}

// ---------------------------------------------------------------- yan duvar: ekip logosu + jam afişi

UTexture2D* ADBWorld::LoadWallPng(const FString& Rel)
{
	const FString Path = FPaths::ProjectContentDir() / Rel;
	return FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
}

UStaticMeshComponent* ADBWorld::TexPlane(USceneComponent* Parent, const FVector& Loc, const FRotator& Rot, const FVector2D& SizeCm, UTexture* Tex, bool bTranslucent)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(PlaneMesh);
	C->SetupAttachment(Parent);
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(FVector(SizeCm.X / 100.f, SizeCm.Y / 100.f, 1.f));
	UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(bTranslucent ? PuffMaterial : PageMaterial, this);
	Mid->SetTextureParameterValue(TEXT("SlateUI"), Tex);
	C->SetMaterial(0, Mid);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->RegisterComponent();
	return C;
}

void ADBWorld::BuildWallArt()
{
	// Yan duvar (Y = WallY, odaya -Y bakar). Menü kamerasından takvimin sağında görünür. Düzlemin X'i bakanın sağına
	// (-X), Y'si aşağı, normali odaya. Dosya yoksa çerçeve boş kalmaz: düz renkli pano.
	const FRotator Face = FMatrix(FVector(-1, 0, 0), FVector(0, 0, -1), FVector(0, -1, 0), FVector::ZeroVector).Rotator();
	struct FArt { const TCHAR* File; FVector2D Size; float X, Z; FLinearColor Fallback; };
	const FArt Arts[] = {
		{ TEXT("Arayuz/duvar_afis.png"), FVector2D(44.f, 62.f), 2.f, 82.f, RGB(232, 128, 104) },
		{ TEXT("Arayuz/duvar_logo.png"), FVector2D(34.f, 34.f), -52.f, 96.f, RGB(96, 170, 164) },
	};
	for (const FArt& A : Arts)
	{
		const float Y = WallY - 1.3f;   // çerçevenin (WallY - 0,8) önünde
		// Ahşap çerçeve (arkada) + raptiye
		Part(WallYRoot, CubeMesh, FVector(A.X, WallY - 0.4f, A.Z), FVector(A.Size.X + 3.f, 0.8f, A.Size.Y + 3.f), RGB(150, 104, 66));
		Part(WallYRoot, SphereMesh, FVector(A.X, Y - 0.6f, A.Z + A.Size.Y * 0.5f - 2.f), FVector(1.6f, 1.6f, 1.6f), RGB(214, 76, 66));
		if (UTexture2D* T = LoadWallPng(A.File))
		{
			TexPlane(WallYRoot, FVector(A.X, Y, A.Z), Face, A.Size, T, false);
		}
		else
		{
			Part(WallYRoot, CubeMesh, FVector(A.X, Y + 0.1f, A.Z), FVector(A.Size.X, 0.4f, A.Size.Y), A.Fallback);
		}
	}
}
