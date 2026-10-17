#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DBWorld.generated.h"

class ACameraActor;
class APlayerController;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UTextureRenderTarget2D;
class UTexture2D;
class UMaterialInstanceDynamic;
class UDBSfxWave;

// Odanın tamamı koddan: zemin, iki duvar, kaide, tahta, binalar, duvardaki takvim, üç kamera.
// Seviye dosyasında yalnızca ışık var; modelciden mesh gelince ilgili parça burada değiştirilir.
UCLASS()
class DISASTERBOARD_API ADBWorld : public AActor
{
	GENERATED_BODY()

public:
	ADBWorld();
	virtual void Tick(float DeltaSeconds) override;

	void Build();

	// --- Ayarlar (ölçüler Art/Sahne/AnaMenu_OyunAlani.blend'den; Blender x → −X, y → Y, kaide üstü Z=0) ---
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") int32 GridN = 13;            // blend: 13×13 oluk
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float CellCm = 8.f;          // oluk aralığı
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float TileCm = 6.6f;         // karonun üst yüzü
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float BoardCm = 128.f;       // dış ölçü
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float BoardTopZ = 13.8f;     // karoların üstü
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float BuildingScale = 0.608f; // 24'lük bina plakası = 2×2 karo (14,6 cm)
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float WalkerSpeed = 26.f;     // tahliyede sakinler (cm/sn)
	UPROPERTY(EditAnywhere, Category = "DB|Oda") float WallX = 100.7f;          // takvimin duvarı
	UPROPERTY(EditAnywhere, Category = "DB|Oda") float WallY = 100.7f;          // yan duvar
	UPROPERTY(EditAnywhere, Category = "DB|Oda") float FloorZ = -160.7f;
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") FVector2D CalendarYZ = FVector2D(-2.4f, 88.9f);   // duvarda merkez
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") FVector2D CalendarSize = FVector2D(88.5f, 64.3f); // yatay
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") float PageFlipDegPerSec = 300.f;
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") float PageGain = 1.1f;       // ışıksız sayfa dokusu sahneyle aynı parlaklıkta görünsün
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") bool bFlipPageU = false;     // doku aynalı görünürse
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") bool bFlipPageV = false;

	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> TopRT;    // menü (üst sayfa)
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> UnderRT;  // Ayarlar / Emeği Geçenler (alt sayfa)
	// Bulanıklığın asıl sebebi bitmap fonttu (Runtime fontla düzeldi). 2048 + mip denendi: mipler canvas çiziminden
	// sonra güncellenmediği için sayfa boş göründü. Yakın planda sayfa ekranda ~1100 px: 1024 yetiyor.
	FVector2D PageTextureSize() const { return FVector2D(1024.f, 744.f); }
	// Ekrandaki nokta sayfanın neresine denk geliyor (0..1); sayfa düğmelerine tıklamak için
	bool PagePointFromScreen(const APlayerController* PC, const FVector2D& Screen, bool bTop, FVector2D& OutUV) const;
	bool PageUVToScreen(const APlayerController* PC, bool bTop, const FVector2D& UV, FVector2D& OutScreen) const;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") FVector MenuCamLoc = FVector(-245.f, -130.f, 115.f);  // Blender görüntüsü: köşeye çapraz, solda takvim, sağda yan duvar
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") FVector MenuCamLookAt = FVector(60.f, 30.f, 60.f);
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float CalendarCamDistance = 200.f;                 // Ayarlar / Emeği Geçenler
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float MenuCamFov = 55.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") FVector BoardFocus = FVector(-8.f, -8.f, 10.f);
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamDistance = 330.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamPitch = -50.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamYaw = 45.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamFov = 50.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float MinZoomDistance = 190.f;  // tekerlekle yakınlaşma sınırı
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float MaxZoomDistance = 520.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float ExposureEV100 = 1.9f;
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float TurnSeconds = 1.3f;   // oklarla döndürme: kalk, 90° dön, in
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float TurnLiftCm = 28.f;
	UPROPERTY(EditAnywhere, Category = "DB|Işık") FRotator SunRotation = FRotator(-52.f, 32.f, 0.f);

	UPROPERTY() TObjectPtr<ACameraActor> MenuCam;
	UPROPERTY() TObjectPtr<ACameraActor> CalendarCam;
	UPROPERTY() TObjectPtr<ACameraActor> BoardCam;

	// --- Sorgular ---
	int32 NumBuildings() const { return BuildingRoots.Num(); }
	FVector MarkerAnchor(int32 Building) const;
	int32 BuildingFromComponent(const UPrimitiveComponent* Comp) const;
	// Takvim sayfasının ekrandaki dikdörtgeni (menü yazıları buraya çizilir)
	bool GetPageScreenRect(const APlayerController* PC, FVector2D& OutMin, FVector2D& OutMax) const;

	// --- Takvim ---
	void FlipPage(bool bUp);
	bool IsPageFlipping() const { return !FMath::IsNearlyEqual(PageAngle, PageTarget, 0.5f); }
	bool IsPageUp() const { return PageTarget > 1.f; }

	// --- Tarama ---
	void StartScan(float Seconds);
	void StopScan();
	float ScanPos() const { return ScanS; }            // -R..R, arkadan öne
	float SweepCoord(int32 Building) const;           // binanın tarama eksenindeki yeri

	// --- Deprem / geri bildirim ---
	void StartQuake(const TArray<bool>& SafeBuildings, float Seconds);
	void PopBuilding(int32 Building);
	void JiggleBuilding(int32 Building);   // seçilince esneyip sallanır (jöle)
	void StartEvacuation(int32 FromBuilding, int32 ToBuilding);   // tsunami: sakinler sokaklardan sığınağa yürür
	// Atmosfer: riskli binanın çevresinde afetine göre renkli sis; çözülünce dağılır, kum savrulur
	void ShowHaze(int32 Building, const FString& Hazard);

	// Izgara: yol karoları, rögar kapakları, yangın musluğu
	static bool IsStreetIndex(int32 I) { return I == 2 || I == 5 || I == 8; }
	bool IsRoad(FIntPoint Cell) const { return Cell.Y >= 0 && Cell.Y < GridN - 2 && Cell.X >= 0 && Cell.X < GridN && (IsStreetIndex(Cell.X) || IsStreetIndex(Cell.Y)); }
	bool CellFromWorld(const FVector& WorldLoc, FIntPoint& OutCell) const;
	FVector CellWorld(FIntPoint Cell) const;        // karonun üst yüzünün ortası (dünya)
	void PlaceManhole(FIntPoint Cell);
	bool HasManhole(FIntPoint Cell) const { return Manholes.Contains(Cell); }
	void RepairHydrant();
	void PlaceNewHydrant();                         // musluğu olmayan sokağa yenisi (yerden çıkar)
	FVector ResidentWorld(int32 Building) const;    // kasabalının başının üstü (konuşma balonu buraya bağlanır)
	void ClearHaze(int32 Building);
	void ApplyExposure(float EV100);

	// Oyuncu tahtayı döndürür: alttaki oklar / Q-E → tahta masadan kalkar, 90° döner, yerine iner (kamera sabit)
	void TurnBoard(int32 Dir);
	UPROPERTY() TObjectPtr<UDBSfxWave> SfxWave;      // GameMode verir: tahta sesleri
	void Gust(float Strength) { GustAmp = FMath::Max(GustAmp, Strength); }   // takvim sayfasına rüzgar
	bool IsBoardTurning() const { return TurnT >= 0.f; }
	// Yakınlaştırma (tekerlek); yumuşak geçiş
	void RotateBoardView(float Deg) { ViewYawTarget += Deg; }
	void ZoomBoardView(float DeltaCm) { ViewDistTarget = FMath::Clamp(ViewDistTarget + DeltaCm, MinZoomDistance, MaxZoomDistance); }
	float ViewYaw = 0.f;
	float ViewYawTarget = 0.f;
	float ViewDist = 330.f;
	float ViewDistTarget = 330.f;
	void UpdateBoardCam(float Dt);

private:
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY() TObjectPtr<USceneComponent> BoardRoot;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> BuildingRoots;
	UPROPERTY() TObjectPtr<USceneComponent> CalendarRoot;
	UPROPERTY() TObjectPtr<USceneComponent> WallXRoot;
	UPROPERTY() TObjectPtr<USceneComponent> WallYRoot;
	UPROPERTY() TObjectPtr<USceneComponent> PageHinge;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ScanBeam;
	UPROPERTY() TObjectPtr<UStaticMesh> PlaneMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> PageMaterial;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> TopPagePlane;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> UnderPagePlane;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> Walkers;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SeaTiles;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PuddleComps;
	TArray<FIntPoint> PuddleCells;
	TSet<FIntPoint> Manholes;
	UPROPERTY() TObjectPtr<USceneComponent> HydrantRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HydrantLeak;
	float HydrantFixT = -1.f;
	UPROPERTY() TObjectPtr<USceneComponent> NewHydrantRoot;
	float NewHydrantT = -1.f;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BrokenCovers;   // su birikintisindeki delik ve yerinden oynamış kapak
	USceneComponent* MakeHydrant(int32 Building, FIntPoint Cell, bool bCollide);
	float BoardYaw = 0.f;
	float TurnFrom = 0.f;
	float TurnTo = 0.f;
	float TurnT = -1.f;
	int32 TurnQueued = 0;
	// Geri bildirim (17 Ekim): takvim sayfası dalgalanır; tahta dönerken denizden masaya su sıçrar, üstündekiler sallanır
	float GustAmp = 0.f;
	float FlutterT = 0.f;
	struct FDrop
	{
		FVector P;
		FVector V;
		bool bOn = false;
	};
	TArray<FDrop> Drops;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DropComps;
	struct FTablePuddle
	{
		float T = -1.f;
		float Size = 3.f;
	};
	TArray<FTablePuddle> TablePuddles;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TablePuddleComps;
	float DropSpawn = 0.f;
	int32 DropsLanded = 0;
	bool bSplashPlayed = false;
	struct FSway
	{
		TObjectPtr<USceneComponent> Root;
		FVector2D Axis = FVector2D(1.f, 0.f);   // tahtada merkezden dışarı: üst kısım bunun dik yönünde sallanır
		float Radius = 0.f;
		float Theta = 0.f;                       // derece
		float Omega = 0.f;
		float Gain = 1.f;
		float Phase = 0.f;
	};
	TArray<FSway> Sways;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> TreeRoots;
	float PrevYaw = 0.f;
	void BuildFeedbackFx();
	void TickFeedbackFx(float Dt, float YawVel);
	void SpawnDrop();
	float SeaStartY() const { return -GridN * CellCm * 0.5f + (GridN - 2) * CellCm; }   // yerel Y: deniz buradan başlar
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> CarRoots;
	struct FCar
	{
		FVector A, B;
		float T = 0.f;
		float Speed = 12.f;
		bool bFwd = true;
	};
	TArray<FCar> Cars;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> ResidentRoots;
	FVector CellLocal(FIntPoint Cell) const;
	void BuildCars();
	void BuildHydrant();
	UPROPERTY() TObjectPtr<UMaterialInterface> PuffMaterial;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> PuffTextures;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PuffMids;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PuffComps;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GrainComps;
	struct FPuff
	{
		int32 Building = -1;    // -1: tarama tozu
		FVector Base;
		float Size = 10.f;
		float Phase = 0.f;
		float Scale = 0.f;
		float Target = 0.f;
	};
	TArray<FPuff> Puffs;
	struct FGrain
	{
		FVector P;
		FVector V;
		bool bOn = false;
	};
	TArray<FGrain> Grains;
	float GrainSpawn = 0.f;
	int32 MakePuff(int32 Building, const FVector& Base, float Size, int32 Tint);
	void SpawnGrain(const FVector& At, float Up);
	void BuildFx();
	TArray<FVector> EvacPath;
	float EvacT = -1.f;
	float WaveT = 0.f;
	UStaticMeshComponent* PagePlane(USceneComponent* Parent, const FVector& Loc, UTextureRenderTarget2D* RT);
	FVector PointOnPath(float Dist) const;
	float PathLength() const;

	TArray<float> BuildingHeights;
	TArray<float> PopTime;
	TArray<float> JiggleTime;
	TArray<bool> QuakeSafe;
	TArray<FRotator> FallRot;

	float PageAngle = 0.f;
	float PageTarget = 0.f;
	float PageSign = 1.f;

	bool bScanning = false;
	float ScanT = 0.f;
	float ScanDuration = 2.f;
	float ScanS = -1000.f;

	bool bQuaking = false;
	float QuakeT = 0.f;
	float QuakeDuration = 4.f;

	float HalfDiag() const;
	FVector LotCenter(FIntPoint Lot) const;   // DBData'daki 5×5 arsa → 13×13 tahtada 2×2 karo

	UStaticMeshComponent* Part(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FVector& SizeCm,
		const FLinearColor& Color, const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = false);
	USceneComponent* Node(USceneComponent* Parent, const FVector& Loc);
	void BuildRoom();
	void BuildBoard();
	void BuildBuilding(int32 Index);
	void BuildCalendar();
	void BuildCameras();
	void BuildTree(USceneComponent* Parent, const FVector& Loc, float Scale);
};
