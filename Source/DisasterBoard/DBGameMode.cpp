#include "DBGameMode.h"
#include "DBHUD.h"
#include "DBPlayerController.h"
#include "DBWorld.h"
#include "Camera/CameraActor.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogDB, Log, All);

ADBGameMode::ADBGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	HUDClass = ADBHUD::StaticClass();
	PlayerControllerClass = ADBPlayerController::StaticClass();
}

void ADBGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// "Tekrar oyna" haritayı ?Direkt=1 ile açar: menü ve eğitim konuşmaları atlanır
	bSkipTutorial = UGameplayStatics::HasOption(Options, TEXT("Direkt"));
}

void ADBGameMode::StartPlay()
{
	Super::StartPlay();

	const FDBContent& C = FDBContent::Get();
	Revealed.Init(false, C.Problems.Num());
	Solved.Init(false, C.Problems.Num());
	RevealTime.Init(-100.f, C.Problems.Num());
	SolveTime.Init(-100.f, C.Problems.Num());

	World = GetWorld()->SpawnActor<ADBWorld>();
	World->Build();

	if (bSkipTutorial)
	{
		if (APlayerController* P = PC())
		{
			P->SetViewTarget(World->BoardCam);
		}
		SetPhase(EDBPhase::WaitScan);
	}
	else
	{
		if (APlayerController* P = PC())
		{
			P->SetViewTarget(World->MenuCam);
		}
		SetPhase(EDBPhase::Menu);
	}
	UE_LOG(LogDB, Log, TEXT("DB_START direkt=%d sorun=%d"), bSkipTutorial ? 1 : 0, C.Problems.Num());
}

APlayerController* ADBGameMode::PC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

void ADBGameMode::SetPhase(EDBPhase NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;
}

void ADBGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	Now += Dt;
	PhaseTime += Dt;
	TalkTime += Dt;

	// İlk karelerde oyuncu kamerası kendi hedefini kurabiliyor: bizimkini koru
	if (Now < 0.3f && World)
	{
		if (APlayerController* P = PC())
		{
			AActor* Want = (Phase == EDBPhase::Menu) ? World->MenuCam.Get() : World->BoardCam.Get();
			if (P->GetViewTarget() != Want)
			{
				P->SetViewTarget(Want);
			}
		}
	}

	switch (Phase)
	{
	case EDBPhase::ToBoard:
		if (PhaseTime >= CameraBlendSeconds)
		{
			StartTalk(FDBContent::Get().IntroA, [this] { SetPhase(EDBPhase::WaitScan); });
		}
		break;

	case EDBPhase::Scanning:
	{
		const FDBContent& C = FDBContent::Get();
		for (int32 p = 0; p < C.Problems.Num(); ++p)
		{
			if (!Revealed[p] && World->ScanPos() >= World->SweepCoord(C.Problems[p].Building))
			{
				Revealed[p] = true;
				RevealTime[p] = Now;
			}
		}
		if (PhaseTime >= ScanSeconds + 0.6f)
		{
			World->StopScan();
			for (int32 p = 0; p < C.Problems.Num(); ++p)
			{
				if (!Revealed[p])
				{
					Revealed[p] = true;
					RevealTime[p] = Now;
				}
			}
			StartTalk(C.AfterScan, [this]
			{
				bPanelOpen = true;
				StartTalk(FDBContent::Get().BagIntroB, [this] { SetPhase(EDBPhase::BagPrep); });
			});
		}
		break;
	}

	case EDBPhase::Play:
		TimeLeft -= Dt;
		if (AllSolvedAt >= 0.f && Now - AllSolvedAt > 1.4f)
		{
			BeginQuake();
		}
		else if (TimeLeft <= 0.f)
		{
			TimeLeft = 0.f;
			BeginQuake();
		}
		break;

	case EDBPhase::Quake:
		if (PhaseTime >= QuakeDuration + 0.8f)
		{
			SetPhase(EDBPhase::Result);
		}
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------- menü

void ADBGameMode::ClickPlay()
{
	if (Phase != EDBPhase::Menu)
	{
		return;
	}
	SetPhase(EDBPhase::ToBoard);
	if (APlayerController* P = PC())
	{
		P->SetViewTargetWithBlend(World->BoardCam, CameraBlendSeconds, VTBlend_EaseInOut, 2.2f);
	}
}

void ADBGameMode::OpenMenuPage(EDBMenuPage Page)
{
	if (Phase != EDBPhase::Menu || World->IsPageFlipping())
	{
		return;
	}
	MenuPage = Page;
	World->FlipPage(true);
	SetPhase(EDBPhase::MenuPage);
}

void ADBGameMode::CloseMenuPage()
{
	if (Phase != EDBPhase::MenuPage || World->IsPageFlipping())
	{
		return;
	}
	World->FlipPage(false);
	SetPhase(EDBPhase::Menu);
}

void ADBGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, PC(), EQuitPreference::Quit, false);
}

void ADBGameMode::ToggleFullscreen()
{
	if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const bool bWindowed = GS->GetFullscreenMode() == EWindowMode::Windowed;
		GS->SetFullscreenMode(bWindowed ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
		GS->ApplySettings(false);
	}
}

void ADBGameMode::CycleQuality()
{
	if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		int32 Q = GS->GetOverallScalabilityLevel();
		Q = (Q < 0) ? 3 : (Q + 1) % 4;
		GS->SetOverallScalabilityLevel(Q);
		GS->ApplySettings(false);
	}
}

FString ADBGameMode::FullscreenLabel() const
{
	const UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return (GS && GS->GetFullscreenMode() != EWindowMode::Windowed) ? TEXT("Tam ekran") : TEXT("Pencere");
}

FString ADBGameMode::QualityLabel() const
{
	const UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const int32 Q = GS ? GS->GetOverallScalabilityLevel() : -1;
	switch (Q)
	{
	case 0: return TEXT("Düşük");
	case 1: return TEXT("Orta");
	case 2: return TEXT("Yüksek");
	case 3: return TEXT("En yüksek");
	default: return TEXT("Özel");
	}
}

// ---------------------------------------------------------------- konuşma

void ADBGameMode::StartTalk(const TArray<FDBLine>& Lines, TFunction<void()> OnDone)
{
	if (bSkipTutorial || Lines.Num() == 0)
	{
		OnDone();
		return;
	}
	Talk = &Lines;
	TalkIndex = 0;
	TalkTime = 0.f;
	TalkDone = MoveTemp(OnDone);
	SetPhase(EDBPhase::Talk);
}

const FDBLine* ADBGameMode::CurrentLine() const
{
	return (Phase == EDBPhase::Talk && Talk && Talk->IsValidIndex(TalkIndex)) ? &(*Talk)[TalkIndex] : nullptr;
}

int32 ADBGameMode::VisibleChars() const
{
	const FDBLine* L = CurrentLine();
	return L ? FMath::Min(L->Text.Len(), FMath::FloorToInt(TalkTime * TalkCharsPerSecond)) : 0;
}

void ADBGameMode::AdvanceTalk()
{
	const FDBLine* L = CurrentLine();
	if (!L)
	{
		return;
	}
	if (VisibleChars() < L->Text.Len())
	{
		TalkTime = 1000.f; // önce cümleyi tamamla
		return;
	}
	if (++TalkIndex < Talk->Num())
	{
		TalkTime = 0.f;
		return;
	}
	Talk = nullptr;
	TFunction<void()> Done = MoveTemp(TalkDone);
	TalkDone = nullptr;
	if (Done)
	{
		Done();
	}
}

// ---------------------------------------------------------------- tarama ve çanta masası

void ADBGameMode::StartScan()
{
	if (Phase != EDBPhase::WaitScan)
	{
		return;
	}
	World->StartScan(ScanSeconds);
	SetPhase(EDBPhase::Scanning);
}

void ADBGameMode::OpenPanel()
{
	if (Phase == EDBPhase::Play)
	{
		bPanelOpen = true;
		ClearSelection();
	}
}

void ADBGameMode::ClosePanel()
{
	if (!bPanelOpen || (Phase != EDBPhase::BagPrep && Phase != EDBPhase::Play))
	{
		return;
	}
	bPanelOpen = false;
	if (!bPlayStarted)
	{
		StartTalk(FDBContent::Get().PanelIntroC, [this]
		{
			bPlayStarted = true;
			TimeLeft = QuakeSeconds;
			SetPhase(EDBPhase::Play);
		});
	}
}

void ADBGameMode::AddItem(int32 Item)
{
	if (!CanEditBags() || !FDBContent::Get().Items.IsValidIndex(Item))
	{
		return;
	}
	if (Draft.Contains(Item))
	{
		ShowToast(TEXT("Bu eşya zaten çantada."), true);
		return;
	}
	if (Draft.Num() >= BagCapacity)
	{
		ShowToast(FString::Printf(TEXT("Çanta dolu! En fazla %d eşya sığar."), BagCapacity), true);
		return;
	}
	Draft.Add(Item);
}

void ADBGameMode::RemoveDraftAt(int32 Slot)
{
	if (CanEditBags() && Draft.IsValidIndex(Slot))
	{
		Draft.RemoveAt(Slot);
	}
}

void ADBGameMode::ClearDraft()
{
	if (CanEditBags())
	{
		Draft.Reset();
	}
}

void ADBGameMode::PackBag()
{
	if (!CanEditBags())
	{
		return;
	}
	if (Draft.Num() == 0)
	{
		ShowToast(TEXT("Çanta boş. Önce eşyalara tıkla."), true);
		return;
	}
	int32 Slot = Bags.IndexOfByPredicate([](const FDBBag& B) { return B.bUsed; });
	if (Slot == INDEX_NONE)
	{
		if (Bags.Num() >= MaxBags)
		{
			ShowToast(TEXT("Sırt çantasında yer kalmadı. Önce bir çantayı kullan."), true);
			return;
		}
		Slot = Bags.AddDefaulted();
	}
	Bags[Slot].Items = Draft;
	Bags[Slot].bUsed = false;
	Draft.Reset();
	ShowToast(TEXT("Çanta hazır! Alttaki sırt çantasına eklendi."), false);
}

// ---------------------------------------------------------------- uygulama

void ADBGameMode::SelectTool(int32 Tool)
{
	if (Phase != EDBPhase::Play || !FDBContent::Get().Tools.IsValidIndex(Tool))
	{
		return;
	}
	SelTool = (SelTool == Tool) ? -1 : Tool;
	SelBag = -1;
}

void ADBGameMode::SelectBag(int32 Bag)
{
	if (Phase != EDBPhase::Play || !Bags.IsValidIndex(Bag) || Bags[Bag].bUsed)
	{
		return;
	}
	SelBag = (SelBag == Bag) ? -1 : Bag;
	SelTool = -1;
}

void ADBGameMode::ClearSelection()
{
	SelTool = -1;
	SelBag = -1;
}

int32 ADBGameMode::ProblemOfBuilding(int32 Building) const
{
	return FDBContent::Get().Problems.IndexOfByPredicate([Building](const FDBProblemDef& P) { return P.Building == Building; });
}

int32 ADBGameMode::NumSolved() const
{
	return Solved.FilterByPredicate([](bool b) { return b; }).Num();
}

int32 ADBGameMode::NumRevealed() const
{
	return Revealed.FilterByPredicate([](bool b) { return b; }).Num();
}

FString ADBGameMode::MissingItemsText(int32 Problem, const TArray<int32>& BagItems) const
{
	const FDBContent& C = FDBContent::Get();
	TArray<FString> Missing;
	for (int32 Item : C.Problems[Problem].Items)
	{
		if (!BagItems.Contains(Item))
		{
			Missing.Add(C.Items[Item].Name);
		}
	}
	return FString::Join(Missing, TEXT(", "));
}

void ADBGameMode::ShowToast(const FString& Text, bool bBad, int32 Building)
{
	Toast = Text;
	ToastTime = Now;
	bToastBad = bBad;
	ToastBuilding = Building;
}

void ADBGameMode::Penalty(const FString& Why, int32 Building)
{
	TimeLeft = FMath::Max(0.f, TimeLeft - WrongPenaltySeconds);
	ShowToast(FString::Printf(TEXT("%s (-%d sn)"), *Why, FMath::RoundToInt(WrongPenaltySeconds)), true, Building);
}

void ADBGameMode::ClickBuilding(int32 Building)
{
	if (Phase != EDBPhase::Play)
	{
		return;
	}
	const FDBContent& C = FDBContent::Get();
	if (!C.Buildings.IsValidIndex(Building))
	{
		return;
	}
	const FString& Name = C.Buildings[Building].Name;
	const int32 P = ProblemOfBuilding(Building);

	if (P == INDEX_NONE || !Revealed[P])
	{
		ShowToast(FString::Printf(TEXT("%s: tarayıcı burada sorun bulmadı."), *Name), false, Building);
		return;
	}
	if (Solved[P])
	{
		ShowToast(FString::Printf(TEXT("%s zaten hazır."), *Name), false, Building);
		return;
	}
	const FDBProblemDef& Prob = C.Problems[P];

	if (SelTool < 0 && SelBag < 0)
	{
		ShowToast(Prob.Note, false, Building);
		return;
	}

	if (SelTool >= 0)
	{
		if (Prob.Tool == SelTool)
		{
			ShowToast(FString::Printf(TEXT("%s: %s tamam. Bina güvende!"), *Name, *C.Tools[SelTool].Name), false, Building);
			Solve(P, Building);
		}
		else if (Prob.Tool < 0)
		{
			Penalty(FString::Printf(TEXT("%s bir şehir aracı değil, afet çantası istiyor."), *Name), Building);
		}
		else
		{
			Penalty(FString::Printf(TEXT("%s burada işe yaramaz. Notu tekrar oku."), *C.Tools[SelTool].Name), Building);
		}
		return;
	}

	// Çanta
	FDBBag& Bag = Bags[SelBag];
	if (Prob.Tool >= 0)
	{
		Penalty(TEXT("Çanta burada işe yaramaz: bu bir şehir sorunu, araç gerekir."), Building);
		return;
	}
	const FString Missing = MissingItemsText(P, Bag.Items);
	if (!Missing.IsEmpty())
	{
		Penalty(FString::Printf(TEXT("Çantada eksik var: %s."), *Missing), Building);
		return;
	}
	Bag.bUsed = true;
	SelBag = -1;
	ShowToast(FString::Printf(TEXT("%s afet çantasını aldı. Hazırlar!"), *Name), false, Building);
	Solve(P, Building);
}

void ADBGameMode::Solve(int32 Problem, int32 Building)
{
	Solved[Problem] = true;
	SolveTime[Problem] = Now;
	SelTool = -1;
	World->PopBuilding(Building);
	UE_LOG(LogDB, Log, TEXT("DB_SOLVE sorun=%d bina=%d toplam=%d/%d"), Problem, Building, NumSolved(), Solved.Num());
	if (NumSolved() == Solved.Num())
	{
		AllSolvedAt = Now;
	}
}

void ADBGameMode::BeginQuake()
{
	if (Phase == EDBPhase::Quake || Phase == EDBPhase::Result)
	{
		return;
	}
	const FDBContent& C = FDBContent::Get();
	TArray<bool> Safe;
	Safe.Init(true, C.Buildings.Num());
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		Safe[C.Problems[p].Building] = Solved[p];
	}
	bWon = NumSolved() == Solved.Num();
	bPanelOpen = false;
	ClearSelection();
	World->StartQuake(Safe, QuakeDuration);
	SetPhase(EDBPhase::Quake);
	UE_LOG(LogDB, Log, TEXT("DB_QUAKE hazir=%d/%d kalan=%.1f"), NumSolved(), Solved.Num(), TimeLeft);
}

void ADBGameMode::Restart(bool bDirect)
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, bDirect ? TEXT("Direkt=1") : TEXT(""));
}

void ADBGameMode::Back()
{
	switch (Phase)
	{
	case EDBPhase::MenuPage: CloseMenuPage(); break;
	case EDBPhase::BagPrep: ClosePanel(); break;
	case EDBPhase::Play:
		if (SelTool >= 0 || SelBag >= 0)
		{
			ClearSelection();
		}
		else if (bPanelOpen)
		{
			ClosePanel();
		}
		break;
	default: break;
	}
}
