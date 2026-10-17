#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DBTypes.h"
#include "DBGameMode.generated.h"

class ADBWorld;
class UDBMusicWave;
class UAudioComponent;
class APlayerController;

struct FDBBag
{
	TArray<int32> Items;
	int32 Kit = 0;
	bool bUsed = false;
};

// Oyunun akışı: menü → kamera → A → tarama → B + çanta masası → C → oyna (geri sayım) → deprem → sonuç.
// HUD bu sınıfın durumunu okur ve tıklamaları buradaki eylemlere çevirir.
UCLASS()
class DISASTERBOARD_API ADBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADBGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; } // piyon yok, yalnızca kamera

	// --- Ayarlar ---
	UPROPERTY(EditAnywhere, Category = "DB") bool bQuakeAtEnd = false;    // 17 Ekim: oyunda deprem sahnesi ve süre yok
	UPROPERTY(EditAnywhere, Category = "DB") float QuakeSeconds = 150.f;  // yalnızca bQuakeAtEnd açıksa
	UPROPERTY(EditAnywhere, Category = "DB") int32 BagCapacity = 3;   // her çanta 3'lü kombinasyon
	UPROPERTY(EditAnywhere, Category = "DB") int32 MaxBags = 4;
	UPROPERTY(EditAnywhere, Category = "DB") float CameraBlendSeconds = 2.2f;
	UPROPERTY(EditAnywhere, Category = "DB") float CalendarBlendSeconds = 0.9f;  // menü ↔ takvim yakın plan
	UPROPERTY(EditAnywhere, Category = "DB") float ComicSeconds = 5.f;           // çizgi roman: sahne başına
	UPROPERTY(EditAnywhere, Category = "DB") float MusicVolume = 0.55f;
	UPROPERTY(EditAnywhere, Category = "DB") float BeltSeconds = 14.f;   // fabrika: çanta tepeden teslime (durmaz)
	UPROPERTY(EditAnywhere, Category = "DB") float ScanSeconds = 2.4f;
	UPROPERTY(EditAnywhere, Category = "DB") float QuakeDuration = 5.f;
	UPROPERTY(EditAnywhere, Category = "DB") float TalkCharsPerSecond = 55.f;

	// --- Durum (HUD okur) ---
	EDBPhase Phase = EDBPhase::Menu;
	EDBMenuPage MenuPage = EDBMenuPage::None;
	float PhaseTime = 0.f;
	float Now = 0.f;

	bool bPanelOpen = false;
	bool bPlayStarted = false;
	bool bSkipTutorial = false;
	bool bSkipIntro = false;   // "Menü"ye dönüşte hikâye tekrar oynamaz

	TArray<bool> Revealed;
	TArray<bool> Solved;
	TArray<float> RevealTime;
	TArray<float> SolveTime;

	TArray<FDBBag> Bags;
	TArray<int32> Draft;
	int32 DraftKit = 0;
	int32 Money = 0;
	int32 SelTool = -1;
	int32 SelBag = -1;
	float TimeLeft = 0.f;

	FString Toast;
	float ToastTime = -100.f;
	bool bToastBad = false;
	int32 ToastBuilding = -1;

	bool bWon = false;
	int32 Mistakes = 0;

	UPROPERTY() TObjectPtr<ADBWorld> World;
	UPROPERTY() TObjectPtr<UDBMusicWave> Music;
	UPROPERTY() TObjectPtr<UAudioComponent> MusicComp;
	int32 ComicIndex = 0;

	// --- Çanta fabrikası (Purble Place gibi; bant yukarıdan aşağı akar, durmaz) ---
	static constexpr int32 NumStations = 4;    // 0 temel kit, 1..3 kişiye özel
	static float StationCenter(int32 K) { return 0.14f + K * 0.21f; }
	static float StationHalf() { return 0.085f; }
	int32 OrderProblem = -1;     // hazırlanan sipariş (çanta isteyen sorun)
	bool bFactoryTutorial = true; // ilk doğru teslime kadar seçilmesi gerekenler parlar (hatada da sürer)

	// --- Kasabalılarla konuşma ---
	int32 NpcBuilding = -1;
	int32 NpcKind = 0;           // 0 selam, 1 dert, 2 teşekkür
	float NpcTime = -100.f;
	FString NpcText() const;
	bool NpcWantsBag() const;    // derdi çanta: balonda "Çantasını hazırla"
	void ShowNpc(int32 Building);
	void CloseNpc() { NpcBuilding = -1; }
	void NpcOpenFactory();
	void ClickCell(FIntPoint Cell);
	void TurnBoard(int32 Dir);   // alttaki oklar / Q-E: tahta kalkar, 90° döner, iner

	// --- Mini oyun (Andaç, 17 Ekim): İngiliz anahtarı → somunu yeşil çizgiye kadar sık;
	//     rögar → yerinden oynamış kapağı yeşil alanda 4 sn tut. Önlemin bedeli kazanınca düşer, vazgeçmek bedava.
	UPROPERTY(EditAnywhere, Category = "DB") float ManholeHoldSeconds = 4.f;
	EDBMini Mini = EDBMini::None;
	int32 MiniProblem = -1;
	int32 MiniBuilding = -1;
	FIntPoint MiniCell = FIntPoint(-1, -1);
	float MiniPos = 0.f;         // somun: anahtarın açısı 0..1 (1'e varırsa fazla sıkıldı); rögar: kapağın yeri 0..1
	float MiniVel = 0.f;
	float MiniHoldT = 0.f;
	float MiniZone = 0.7f;       // yeşil çizginin / alanın ortası
	float MiniZoneHalf = 0.06f;
	float MiniProgress = 0.f;    // rögar: yeşilde geçen süre (sn)
	float MiniT = 0.f;
	float MiniWinAt = -1.f;      // kazanınca kısa "tamam", sonra önlem uygulanır
	FString MiniMsg;
	float MiniMsgTime = -100.f;
	bool bMiniMsgBad = false;
	bool bMiniHold = false;      // oyuncu basılı tutuyor (fare sol tuşu ya da boşluk; PlayerController yazar)
	bool bMiniAutoTest = false;  // DBTest: mini oyunu kendisi oynar
	void CancelMini();

	float BagT = 0.f;            // 0 tepe, 1 teslim
	bool bBagOnBelt = false;
	int32 BagKit = -1;
	TArray<int32> BagPersonal;
	float DeliverTime = -100.f;
	bool bDeliverOk = false;
	bool IsBeltRunning() const { return bPanelOpen && (Phase == EDBPhase::BagPrep || Phase == EDBPhase::Play) && OrderProblem >= 0 && bBagOnBelt; }
	bool IsStationActive(int32 K) const { return IsBeltRunning() && FMath::Abs(BagT - StationCenter(K)) <= StationHalf(); }
	TArray<int32> PendingOrders() const;
	void StationClick(int32 K, int32 Button);
	void SelectOrder(int32 Problem);
	void CycleOrder(int32 Dir);
	void ResetBag();
	void SetBagT(float T) { BagT = T; }

	// --- Sorgular ---
	const FDBLine* CurrentLine() const;
	bool IsTalking(const TArray<FDBLine>& Lines, int32 Index) const { return Phase == EDBPhase::Talk && Talk == &Lines && TalkIndex == Index; }
	bool IsLineComplete() const { const FDBLine* L = CurrentLine(); return L && VisibleChars() >= L->Text.Len(); }
	int32 VisibleChars() const;
	int32 ProblemOfBuilding(int32 Building) const;
	int32 NumSolved() const;
	int32 CurrentTask() const;     // sıradaki görev: ilk çözülmemiş, bulunmuş sorun (yoksa -1)
	int32 NumRevealed() const;
	bool CanEditBags() const { return bPanelOpen && (Phase == EDBPhase::BagPrep || Phase == EDBPhase::Play); }
	// Menü yazıları yalnızca kamera durup sayfa yerine oturunca çizilir
	bool IsMenuReady() const { return Phase == EDBPhase::Menu && PhaseTime >= MenuUIDelay; }
	bool IsMenuPageReady() const;
	bool IsBoardPhase() const { return Phase != EDBPhase::Menu && Phase != EDBPhase::MenuPage && Phase != EDBPhase::Comic; }
	bool IsTimerVisible() const { return bPlayStarted && (Phase == EDBPhase::Play || Phase == EDBPhase::Quake); }
	FString MissingItemsText(int32 Kit, const TArray<int32>& BagItems) const;

	// --- Eylemler ---
	void ClickPlay();
	void NextComic();
	void SkipComic();
	void ToggleMusic();
	FString MusicLabel() const;
	void OpenMenuPage(EDBMenuPage Page);
	void CloseMenuPage();
	void QuitGame();
	void ToggleFullscreen();
	void CycleQuality();
	FString FullscreenLabel() const;
	FString QualityLabel() const;

	void AdvanceTalk();
	void StartScan();
	void OpenPanel();
	void ClosePanel();
	void SetDraftKit(int32 Kit);
	void AddItem(int32 Item);
	void RemoveDraftAt(int32 Slot);
	void ClearDraft();
	void PackBag();
	void SelectTool(int32 Tool);
	void SelectBag(int32 Bag);
	void ClearSelection();
	void ClickBuilding(int32 Building);
	void Restart(bool bDirect);
	void Back();          // Esc
	void TogglePanel() { if (bPanelOpen) ClosePanel(); else OpenPanel(); }

	// Test / ayar
	void SetTimeLeft(float Seconds) { TimeLeft = Seconds; }
	void ForceQuake() { BeginQuake(); }

private:
	void SetPhase(EDBPhase NewPhase);
	void BeginBoardTransition();
	void StartTalk(const TArray<FDBLine>& Lines, TFunction<void()> OnDone);
	void ShowToast(const FString& Text, bool bBad, int32 Building = -1);
	void Penalty(const FString& Why, int32 Building);
	void Solve(int32 Problem, int32 Building);
	void BeginQuake();
	APlayerController* PC() const;

	const TArray<FDBLine>* Talk = nullptr;
	int32 TalkIndex = 0;
	float TalkTime = 0.f;
	TFunction<void()> TalkDone;
	float AllSolvedAt = -1.f;
	float NextBagAt = -1.f;
	void TickBelt(float Dt);
	void DeliverBag();
	void StartMini(EDBMini Kind, int32 Problem, int32 Building, FIntPoint Cell);
	void TickMini(float Dt);
	void MiniSay(const FString& Text, bool bBad);
	void FinishMini();
	void ApplyManhole(FIntPoint Cell);
	bool bMiniWasHold = false;
	bool bMiniNeedRelease = false;
	bool bPageFlipPending = false;  // takvim kamerasına varınca sayfa kalkacak
	bool bReturnPending = false;    // sayfa inince menü kamerasına dönülecek
	float MenuUIDelay = 0.f;
};
