#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DBHUD.generated.h"

class ADBGameMode;
class UFont;
class UTextureRenderTarget2D;
class UTexture2D;
struct FDBLine;

// Bütün arayüz Canvas ile çiziliyor (asset yok). Ölçüler 1080p'ye göre yazılır, S ile ölçeklenir.
// Takvim sayfaları (menü, Ayarlar, Emeği Geçenler) aynı yardımcılarla sayfanın dokusuna çizilir; böylece yazılar
// takvimle birlikte eğik görünür. Tıklanabilir alanlar her karede Hits'e (sayfalarda sayfa pikseliyle) yazılır.
UCLASS()
class DISASTERBOARD_API ADBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	bool HandleClick(const FVector2D& P);
	bool IsOverButton(const FVector2D& P) const;
	bool IsOverUI(const FVector2D& P) const;

	// Test: takvim sayfasındaki bir düğmenin ekrandaki yeri (sayfa dokusu ↔ fare eşlemesini doğrulamak için)
	bool PageButtonScreenPos(FName Id, bool bTop, FVector2D& OutScreen) const;
	FVector2D DebugDot = FVector2D(-1.f, -1.f);

private:
	struct FHit
	{
		FBox2D R;
		FName Id;
		int32 Index;
	};
	TArray<FHit> Hits;
	TArray<FHit> TopHits;    // menü sayfası (sayfa pikseli)
	TArray<FHit> UnderHits;  // Ayarlar / Emeği Geçenler sayfası
	TArray<FBox2D> Blockers;
	bool bPageHover = false;

	ADBGameMode* GM = nullptr;
	float S = 1.f;
	float W = 1920.f;
	float H = 1080.f;
	FVector2D Mouse = FVector2D::ZeroVector;
	UPROPERTY() TObjectPtr<UFont> Font;
	float FontBaseH = 1.f;
	float PanelSlide = 0.f;   // hazırlık masası çekmecesi: 0 kapalı (sağ kenarda kulp), 1 açık
	float ColorGain = 1.f;

	// Rehber karakterin gösterdiği yerler (her karede çizilirken güncellenir)
	FBox2D TgScanner = FBox2D(ForceInit), TgTasks = FBox2D(ForceInit), TgTabs = FBox2D(ForceInit), TgItems = FBox2D(ForceInit),
		TgPack = FBox2D(ForceInit), TgClose = FBox2D(ForceInit), TgTools = FBox2D(ForceInit), TgMoney = FBox2D(ForceInit),
		TgBackpack = FBox2D(ForceInit), TgDrawer = FBox2D(ForceInit),
		TgOrder = FBox2D(ForceInit), TgKitStation = FBox2D(ForceInit), TgStations = FBox2D(ForceInit), TgDeliver = FBox2D(ForceInit);
	void DrawBagIcon(const FVector2D& C, float Sc, int32 Kit, const TArray<int32>& Personal);
	void DrawNpc();
	void DrawMini();
	void DrawNutGame(const FBox2D& R);
	void DrawManholeGame(const FBox2D& R);
	void Hex(const FVector2D& C, float R, float AngleRad, const FLinearColor& Col);
	void TurnArrow(const FVector2D& C, float R, int32 Dir, const FLinearColor& Col);
	void DrawCellHints();
	void DotRing(const FVector2D& C, float R, const FLinearColor& Col);
	UPROPERTY() TMap<FString, TObjectPtr<UTexture2D>> PngCache;
	UTexture2D* LoadPng(const FString& Rel);
	void DrawTex(UTexture2D* T, const FBox2D& R, float RotDeg, const FVector2D& Pivot);
	bool GuideTarget(int32 Who, FVector2D& Out) const;
	void DrawGuide(int32 Who, const FVector2D& Feet, float Height);
	void DrawTasks();
	void DrawComic();
	void DrawMiniFriend(const FVector2D& Feet, float K, const FLinearColor& Shirt, float Bob);    // sayfa dokusu ışıksız malzemede: sahneye uysun diye renk çarpanı

	// Çizim yardımcıları (piksel)
	// Sayfa dokusu ışıksız malzemede: sahnedeki ışıklı kağıtla aynı parlaklıkta görünsün diye çarpan (ölçülerek ayarlandı)
	FLinearColor Gn(const FLinearColor& C) const { return FLinearColor(C.R * ColorGain, C.G * ColorGain, C.B * ColorGain, C.A); }
	void Box(float X, float Y, float BW, float BH, const FLinearColor& C);
	void Box(const FBox2D& R, const FLinearColor& C) { Box(R.Min.X, R.Min.Y, R.GetSize().X, R.GetSize().Y, C); }
	void Frame(const FBox2D& R, const FLinearColor& C, float T);
	void Card(const FBox2D& R, const FLinearColor& Fill, const FLinearColor& Edge, float Shadow = 6.f);
	void Disc(const FVector2D& C, float R, const FLinearColor& Col, int32 Sides = 36);
	void Tri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Col);
	void Line(const FVector2D& A, const FVector2D& B, float T, const FLinearColor& C);
	void TickMark(const FVector2D& C, float Size, const FLinearColor& Col);
	void Cross(const FVector2D& C, float Size, const FLinearColor& Col);
	FVector2D Measure(const FString& T, float Px) const;
	void Txt(const FString& T, float X, float Y, float Px, const FLinearColor& C, int32 Align = 0);
	TArray<FString> Wrap(const FString& T, float Px, float MaxW) const;
	float Para(const FString& T, float X, float Y, float Px, float MaxW, const FLinearColor& C, float LineGap = 1.25f, int32 MaxChars = -1, float MaxY = 1e9f);
	bool Button(FName Id, int32 Index, const FBox2D& R, const FString& Label, float Px, const FLinearColor& Fill, const FLinearColor& Edge,
		const FLinearColor& TextC, bool bEnabled = true, bool bSelected = false);
	void Glow(const FBox2D& R);
	void Hit(FName Id, int32 Index, const FBox2D& R) { Hits.Add({ R, Id, Index }); }
	bool Hover(const FBox2D& R) const { return R.IsInside(Mouse); }
	FBox2D Rect(float X, float Y, float BW, float BH) const { return FBox2D(FVector2D(X, Y), FVector2D(X + BW, Y + BH)); }

	// Takvim sayfaları
	void DrawMenuPages();
	void RenderPage(UTextureRenderTarget2D* RT, bool bTop, const FVector2D& PageMouse, TArray<FHit>& OutHits);
	void DrawMainPage(bool bEnabled);
	void DrawSubPage(bool bEnabled);
	const TArray<FHit>* ActivePageHits(FVector2D& OutPageMouse, const FVector2D& Screen) const;

	// Tahta ekranı
	void DrawMarkers();
	void DrawMoney();
	void DrawScannerAndA();
	void DrawBottomBar();
	void DrawPanel();
	void DrawDrawerHandle();
	void DrawTalk();
	void DrawToast();
	void DrawTooltip();
	void DrawSelectionCursor();
	void DrawResult();
	void Portrait(int32 Who, const FVector2D& C, float R);
	void ItemIcon(int32 Item, const FVector2D& C, float R);
	void ToolIcon(int32 Tool, const FVector2D& C, float R);
	void KitBadge(int32 Kit, const FBox2D& R, float Px);

	FBox2D PanelRect() const;
	FBox2D DrawerHandleRect() const;
	FBox2D BottomBarRect() const;
	FBox2D ScannerRect() const;
	FBox2D MoneyRect() const;
};
