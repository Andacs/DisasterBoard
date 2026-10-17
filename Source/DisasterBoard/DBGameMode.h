#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DBTypes.h"
#include "DBGameMode.generated.h"

class ADBWorld;
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
	UPROPERTY(EditAnywhere, Category = "DB") int32 BagCapacity = 4;
	UPROPERTY(EditAnywhere, Category = "DB") int32 MaxBags = 4;
	UPROPERTY(EditAnywhere, Category = "DB") float CameraBlendSeconds = 2.2f;
	UPROPERTY(EditAnywhere, Category = "DB") float CalendarBlendSeconds = 0.9f;  // menü ↔ takvim yakın plan
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

	// --- Sorgular ---
	const FDBLine* CurrentLine() const;
	bool IsTalking(const TArray<FDBLine>& Lines, int32 Index) const { return Phase == EDBPhase::Talk && Talk == &Lines && TalkIndex == Index; }
	bool IsLineComplete() const { const FDBLine* L = CurrentLine(); return L && VisibleChars() >= L->Text.Len(); }
	int32 VisibleChars() const;
	int32 ProblemOfBuilding(int32 Building) const;
	int32 NumSolved() const;
	int32 NumRevealed() const;
	bool CanEditBags() const { return bPanelOpen && (Phase == EDBPhase::BagPrep || Phase == EDBPhase::Play); }
	// Menü yazıları yalnızca kamera durup sayfa yerine oturunca çizilir
	bool IsMenuReady() const { return Phase == EDBPhase::Menu && PhaseTime >= MenuUIDelay; }
	bool IsMenuPageReady() const;
	bool IsBoardPhase() const { return Phase != EDBPhase::Menu && Phase != EDBPhase::MenuPage; }
	bool IsTimerVisible() const { return bPlayStarted && (Phase == EDBPhase::Play || Phase == EDBPhase::Quake); }
	FString MissingItemsText(int32 Kit, const TArray<int32>& BagItems) const;

	// --- Eylemler ---
	void ClickPlay();
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
	bool bPageFlipPending = false;  // takvim kamerasına varınca sayfa kalkacak
	bool bReturnPending = false;    // sayfa inince menü kamerasına dönülecek
	float MenuUIDelay = 0.f;
};
