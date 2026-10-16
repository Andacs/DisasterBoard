#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DBHUD.generated.h"

class ADBGameMode;
class UFont;
struct FDBLine;

// Bütün arayüz Canvas ile çiziliyor (asset yok). Ölçüler 1080p'ye göre yazılır, S ile ölçeklenir.
// Tıklanabilir alanlar her karede Hits'e yazılır; PlayerController tıklamayı önce buraya sorar.
UCLASS()
class DISASTERBOARD_API ADBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	bool HandleClick(const FVector2D& P);
	bool IsOverButton(const FVector2D& P) const;
	bool IsOverUI(const FVector2D& P) const;

private:
	struct FHit
	{
		FBox2D R;
		FName Id;
		int32 Index;
	};
	TArray<FHit> Hits;
	TArray<FBox2D> Blockers;

	ADBGameMode* GM = nullptr;
	float S = 1.f;
	float W = 1920.f;
	float H = 1080.f;
	FVector2D Mouse = FVector2D::ZeroVector;
	UPROPERTY() TObjectPtr<UFont> Font;
	float FontBaseH = 1.f;

	// Çizim yardımcıları (piksel)
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

	// Bölümler
	void DrawMenu();
	void DrawMarkers();
	void DrawTimer();
	void DrawScannerAndA();
	void DrawBottomBar();
	void DrawPanel();
	void DrawTalk();
	void DrawToast();
	void DrawTooltip();
	void DrawSelectionCursor();
	void DrawQuake();
	void DrawResult();
	void Portrait(int32 Who, const FVector2D& C, float R);
	void ItemIcon(int32 Item, const FVector2D& C, float R, bool bName, float NamePx);
	void ToolIcon(int32 Tool, const FVector2D& C, float R);

	FBox2D PanelRect() const;
	FBox2D BottomBarRect() const;
	FBox2D ScannerRect() const;
};
