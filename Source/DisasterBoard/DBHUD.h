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
	// Varlık testi: oyunun istediği her görsel ve durumu ("OK 128x128 aday", "YOK", "HATA")
	FString PngReport() const;
	// Geri bildirim: tıklanan yerde halka + kıvılcımlar (her tıklamada PlayerController çağırır)
	void AddClickFx(const FVector2D& P);
	bool IsOverButton(const FVector2D& P) const;
	FBox2D DoTargetRect() const;   // etkileşimli tutorial satırının hedefi (test de kullanır)
	bool IsOverDeliver(const FVector2D& P) const { return TgDeliver.bIsValid && TgDeliver.IsInside(P); }
	bool IsOverUI(const FVector2D& P) const;

	// Test: takvim sayfasındaki bir düğmenin ekrandaki yeri (sayfa dokusu ↔ fare eşlemesini doğrulamak için)
	bool PageButtonScreenPos(FName Id, bool bTop, FVector2D& OutScreen) const;

	// Tanıtım videosu çekimi (DBSinema): başlık kartı ve altyazı; Menü düğmesi, görev şeridi ve fare izi gizli,
	// animasyon saati oyun zamanı (-benchmark sabit kare). Yalnız çekimde açık.
	FBox2D TgTutSkip = FBox2D(ForceInit);   // "Eğitimi geç" (DBEscTest gerçek tıklama yolundan basar)
	bool bCinema = false;
	FString CinemaTitle;
	FString CinemaSub;
	FString CinemaCaption;
	float CinemaTitleAlpha = 0.f;
	float CinemaCaptionAlpha = 0.f;
	FVector2D DebugDot = FVector2D(-1.f, -1.f);
	float UiScale() const { return S; }

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
	UPROPERTY() TObjectPtr<UFont> FontBold;   // başlıklar ve düğmeler (Baloo 2 Bold); metinler Font (Baloo 2 Medium)
	bool bBoldText = false;                    // Button etiketini kalın yazdırır
	// Baloo 2'nin satırı Roboto'dan uzun (1,60 / 1,17 em), büyük harfi kısa: aynı Px'te aynı görünsün diye ölçek ve
	// taban çizgisi düzeltmesi (fonttools ile ölçüldü; font değişirse yeniden ölç)
	float FontScaleFix = 1.f;
	float FontBaselineFix = 0.f;
	UFont* PickFont(float Px) const { return (FontBold && (bBoldText || Px >= 24.f * S)) ? FontBold.Get() : Font.Get(); }
	float PanelSlide = 0.f;   // hazırlık masası çekmecesi: 0 kapalı (sağ kenarda kulp), 1 açık
	float ColorGain = 1.f;

	// Rehber karakterin gösterdiği yerler (her karede çizilirken güncellenir)
	FBox2D TgScanner = FBox2D(ForceInit), TgTasks = FBox2D(ForceInit), TgTabs = FBox2D(ForceInit), TgItems = FBox2D(ForceInit),
		TgPack = FBox2D(ForceInit), TgClose = FBox2D(ForceInit), TgTools = FBox2D(ForceInit), TgMoney = FBox2D(ForceInit),
		TgBackpack = FBox2D(ForceInit), TgDrawer = FBox2D(ForceInit),
		TgOrder = FBox2D(ForceInit), TgKitStation = FBox2D(ForceInit), TgStations = FBox2D(ForceInit), TgDeliver = FBox2D(ForceInit),
		TgTurnL = FBox2D(ForceInit), TgTurnR = FBox2D(ForceInit);
	// 17 Ekim ~20.00: başarım kartı, çanta-bitti yönlendirmesi, teslim çantası, radar tarayıcı, fotoğraf albümü
	void DrawAchievementPopup();
	void DrawAchBadge(int32 Index, const FVector2D& C, float R, bool bOn);
	void Star(const FVector2D& C, float R, const FLinearColor& Col, float Rot = 0.f);
	void DrawBagsDoneHint();
	void DrawPhotoToast();
	void DrawRT(UTextureRenderTarget2D* T, const FBox2D& R);
	void DrawPhoto(int32 Problem, const FBox2D& R);
	void DrawTray(const FVector2D& C, float Sc, int32 Kit, const TArray<int32>& Personal);
	void DrawDeliveryBag(const FVector2D& C, float Sc, const FLinearColor& Body, float Flap, bool bKitMark, int32 Kit);
	void DrawRadar(const FVector2D& C, float R, bool bScanning);
	bool Nine(UTexture2D* T, const FBox2D& R, float DstB, const FLinearColor& Tint);   // el çizimi çerçeve, 9 dilim
	// Hareket tasarımı: istasyonda seçilen eşya / kit bantta giden pakete uçar, paket yaylanır
	struct FFly { int32 Id; bool bKit; FVector2D From; float T; };
	TArray<FFly> Flies;
	float TrayPopT = -100.f;
	float ResultStart = -1.f;
	int32 ResultPhotosShown = 0;
	int32 ResultHover = -1;
	void DrawBagIcon(const FVector2D& C, float Sc, int32 Kit, const TArray<int32>& Personal);
	void DrawNpc();
	// Fare izi, tıklama patlaması, takvime yaklaşırken rüzgar çizgileri
	struct FClickFx { FVector2D P; float T; };
	TArray<FClickFx> ClickFxs;
	struct FTrail { FVector2D P; float T; };
	TArray<FTrail> Trail;
	float FxNow = 0.f;
	void DrawCursorFx();
	void DrawCinema();
	void DrawMenuButton();
	void DrawMenuConfirm();
	// Revizyon (17 Ekim akşam): yuvarlak kart, yüzdeler, uçan para, başarı/hata efektleri, düğme basışı, görev şeridi
	void RoundCard(const FBox2D& R, const FLinearColor& Fill, const FLinearColor& Edge, float Radius, float Shadow = 6.f);
	void RoundBox(const FBox2D& R, float Radius, const FLinearColor& C);
	void DrawMeters();
	void DrawEventFx();
	float ShownMoney = -1.f;
	int32 LastMoney = INT_MIN;
	struct FMoneyFx { int32 Delta; float T; };
	TArray<FMoneyFx> MoneyFxs;
	float ShownHazirlik = 0.f, ShownFarkindalik = 0.f;
	float LastFxGood = -100.f;
	struct FConfetti { FVector2D P, V; FLinearColor C; float Rot; };
	TArray<FConfetti> Confetti;
	FName PressId;
	int32 PressIndex = -1;
	float PressTime = -100.f;
	bool bTasksOpen = true;
	float DoMissTime = -100.f;   // tutorial: yanlış yere tıklandı, "Bu tuşa bas" etiketi sallanır
	void DrawDoHint();
	void DrawPortrait(int32 Resident, const FVector2D& C, float R);
	void DrawPortraitGallery();
	void DrawTexUV(UTexture2D* T, const FBox2D& R, const FVector2D& UV0, const FVector2D& UV1, float Alpha = 1.f);
	void Sparkles(const FBox2D& Area, int32 N, float T, float Alpha, uint32 Seed);
	void Fountain(const FVector2D& Origin, float Age, int32 N, const FLinearColor& Col, uint32 Seed);
	UFont* MakeRuntimeFont(const TCHAR* File);
	void DrawWind();
	void DrawMini();
	void DrawNutGame(const FBox2D& R);
	void DrawManholeGame(const FBox2D& R);
	void Hex(const FVector2D& C, float R, float AngleRad, const FLinearColor& Col);
	void TurnArrow(const FVector2D& C, float R, int32 Dir, const FLinearColor& Col);
	void DrawCellHints();
	void DotRing(const FVector2D& C, float R, const FLinearColor& Col);
	UPROPERTY() TMap<FString, TObjectPtr<UTexture2D>> PngCache;
	TMap<FString, FString> PngStatus;   // varlık testi raporu: her istenen görsel yüklendi mi
	UTexture2D* LoadPng(const FString& Rel);
	void DrawTex(UTexture2D* T, const FBox2D& R, float RotDeg, const FVector2D& Pivot, const FLinearColor& Tint = FLinearColor::White, bool bFlipX = false);
	bool KitIcon(int32 Kit, const FVector2D& C, float R);   // kit görseli varsa çizer
	bool GuideTarget(int32 Who, FVector2D& Out) const;
	void DrawGuide(int32 Who, const FVector2D& Feet, float Height);
	void DrawTasks();
	void DrawComic();
	void DrawLogoIntro();   // açılış: kutu logosu karanlıktan belirir (Arayuz/logo_kutu.png)
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
