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

// Odanın tamamı koddan: zemin, iki duvar, masa, tahta, binalar, duvardaki takvim, iki kamera.
// Seviye dosyasında yalnızca ışık var; modelciden mesh gelince ilgili parça burada değiştirilir.
UCLASS()
class DISASTERBOARD_API ADBWorld : public AActor
{
	GENERATED_BODY()

public:
	ADBWorld();
	virtual void Tick(float DeltaSeconds) override;

	void Build();

	// --- Ayarlar ---
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") int32 GridN = 5;
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float CellCm = 26.f;
	UPROPERTY(EditAnywhere, Category = "DB|Tahta") float BorderCm = 10.f;
	UPROPERTY(EditAnywhere, Category = "DB|Oda") float WallDistCm = 330.f;
	UPROPERTY(EditAnywhere, Category = "DB|Oda") float FloorZ = -80.f;
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") FVector CalendarCenter = FVector(40.f, 328.f, 135.f);
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") FVector2D CalendarSize = FVector2D(74.f, 92.f);
	UPROPERTY(EditAnywhere, Category = "DB|Takvim") float PageFlipDegPerSec = 300.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float MenuCamDistance = 250.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float MenuCamFov = 50.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") FVector BoardFocus = FVector(-34.f, -34.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamDistance = 450.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamPitch = -50.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamYaw = 45.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float BoardCamFov = 50.f;
	UPROPERTY(EditAnywhere, Category = "DB|Kamera") float ExposureEV100 = 1.9f;
	UPROPERTY(EditAnywhere, Category = "DB|Işık") FRotator SunRotation = FRotator(-52.f, 32.f, 0.f);

	UPROPERTY() TObjectPtr<ACameraActor> MenuCam;
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
	void ApplyExposure(float EV100);

private:
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> ConeMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;

	UPROPERTY() TObjectPtr<USceneComponent> BoardRoot;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> BuildingRoots;
	UPROPERTY() TObjectPtr<USceneComponent> PageHinge;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ScanBeam;

	TArray<float> BuildingHeights;
	TArray<float> PopTime;
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
	FVector CellCenter(FIntPoint Cell) const;

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
