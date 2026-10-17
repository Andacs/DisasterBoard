#include "DBGameMode.h"
#include "DBHUD.h"
#include "DBPlayerController.h"
#include "DBWorld.h"
#include "DBMusic.h"
#include "Components/AudioComponent.h"
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
	bSkipIntro = UGameplayStatics::HasOption(Options, TEXT("Menu"));
}

void ADBGameMode::StartPlay()
{
	Super::StartPlay();

	const FDBContent& C = FDBContent::Get();
	Revealed.Init(false, C.Problems.Num());
	Solved.Init(false, C.Problems.Num());
	RevealTime.Init(-100.f, C.Problems.Num());
	SolveTime.Init(-100.f, C.Problems.Num());
	Money = C.StartMoney;

	World = GetWorld()->SpawnActor<ADBWorld>();
	World->Build();

	// Müzik: koddan üretilir, ilerledikçe hızlanır
	Music = NewObject<UDBMusicWave>(this);
	MusicComp = UGameplayStatics::SpawnSound2D(this, Music, MusicVolume);
	Sfx = NewObject<UDBSfxWave>(this);
	SfxComp = UGameplayStatics::SpawnSound2D(this, Sfx, SfxVolume);
	World->SfxWave = Sfx;

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
		// Hikâye en başta, menüden önce (ekip, 17 Ekim)
		if (!bSkipIntro && C.Comic.Num() > 0)
		{
			ComicIndex = 0;
			SetPhase(EDBPhase::Comic);
		}
		else
		{
			SetPhase(EDBPhase::Menu);
		}
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
			AActor* Want = (Phase == EDBPhase::Menu || Phase == EDBPhase::Comic) ? World->MenuCam.Get() : World->BoardCam.Get();
			if (P->GetViewTarget() != Want)
			{
				P->SetViewTarget(Want);
			}
		}
	}

	if (Music)
	{
		Music->SetIntensity(Solved.Num() > 0 ? (float)NumSolved() / Solved.Num() : 0.f);
	}
	TickBelt(Dt);
	TickMini(Dt);

	switch (Phase)
	{
	case EDBPhase::Comic:
		if (PhaseTime >= ComicSeconds)
		{
			NextComic();
		}
		break;

	case EDBPhase::MenuPage:
		if (bPageFlipPending && PhaseTime >= CalendarBlendSeconds)
		{
			bPageFlipPending = false;
			World->FlipPage(true);
			PlaySfx(EDBSfx::Page, 1.f);
		}
		else if (bReturnPending && !World->IsPageFlipping())
		{
			bReturnPending = false;
			if (APlayerController* P = PC())
			{
				P->SetViewTargetWithBlend(World->MenuCam, CalendarBlendSeconds, VTBlend_EaseInOut, 2.f);
			}
			MenuUIDelay = CalendarBlendSeconds;
			SetPhase(EDBPhase::Menu);
		}
		break;

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
				World->ShowHaze(C.Problems[p].Building, C.Problems[p].Hazard);
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
					World->ShowHaze(C.Problems[p].Building, C.Problems[p].Hazard);
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
		if (AllSolvedAt >= 0.f && Now - AllSolvedAt > 1.4f)
		{
			if (bQuakeAtEnd)
			{
				BeginQuake();
			}
			else
			{
				bWon = true;
				bPanelOpen = false;
				ClearSelection();
				SetPhase(EDBPhase::Result);
			}
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

float ADBGameMode::FarkindalikYuzde() const
{
	const FDBContent& C = FDBContent::Get();
	int32 Talked = 0;
	for (int32 B : TalkedResidents)
	{
		Talked += C.Residents.IsValidIndex(B) ? 1 : 0;
	}
	const float Max = C.Residents.Num() + 2.f * Solved.Num();
	return Max > 0.f ? 100.f * (Talked + 2.f * NumSolved()) / Max : 0.f;
}

void ADBGameMode::PauseToCalendar()
{
	if (!bMenuConfirm || bPausedGame)
	{
		return;
	}
	bMenuConfirm = false;
	bPausedGame = true;
	SavedPhase = Phase;
	ClearSelection();
	CloseNpc();
	MenuUIDelay = CalendarBlendSeconds;
	SetPhase(EDBPhase::Menu);
	if (APlayerController* P = PC())
	{
		P->SetViewTargetWithBlend(World->MenuCam, CalendarBlendSeconds, VTBlend_EaseInOut, 2.f);
	}
	PlaySfx(EDBSfx::Wind, 0.7f, 0.6f, 1.f, -0.6f);
}

void ADBGameMode::ResumeFromCalendar()
{
	bPausedGame = false;
	SetPhase(SavedPhase);
	if (APlayerController* P = PC())
	{
		P->SetViewTargetWithBlend(World->BoardCam, CameraBlendSeconds * 0.7f, VTBlend_EaseInOut, 2.2f);
	}
	PlaySfx(EDBSfx::Wind, 0.7f, -0.6f, 0.9f, 0.6f);
}

void ADBGameMode::ClickPlay()
{
	if (!IsMenuReady())
	{
		return;
	}
	if (bPausedGame)
	{
		ResumeFromCalendar();
		return;
	}
	BeginBoardTransition();
}

void ADBGameMode::PlaySfx(EDBSfx Id, float Volume, float Pan, float Pitch, float PanTo)
{
	if (Sfx)
	{
		Sfx->Play(Id, Volume, Pan, Pitch, PanTo);
	}
}

void ADBGameMode::BeginBoardTransition()
{
	SetPhase(EDBPhase::ToBoard);
	PlaySfx(EDBSfx::Wind, 0.9f, 0.6f, 0.9f, -0.6f);   // kamera masaya inerken hafif esinti
	if (APlayerController* P = PC())
	{
		P->SetViewTargetWithBlend(World->BoardCam, CameraBlendSeconds, VTBlend_EaseInOut, 2.2f);
	}
}

void ADBGameMode::NextComic()
{
	if (Phase != EDBPhase::Comic)
	{
		return;
	}
	if (++ComicIndex >= FDBContent::Get().Comic.Num())
	{
		MenuUIDelay = 0.f;
		SetPhase(EDBPhase::Menu);
		return;
	}
	SetPhase(EDBPhase::Comic);
}

void ADBGameMode::SkipComic()
{
	if (Phase == EDBPhase::Comic)
	{
		MenuUIDelay = 0.f;
		SetPhase(EDBPhase::Menu);
	}
}

void ADBGameMode::ToggleMusic()
{
	if (Music)
	{
		Music->SetMuted(!Music->IsMuted());
	}
}

FString ADBGameMode::MusicLabel() const
{
	return (Music && Music->IsMuted()) ? TEXT("Kapalı") : TEXT("Açık");
}

int32 ADBGameMode::CurrentTask() const
{
	for (int32 p = 0; p < Solved.Num(); ++p)
	{
		if (Revealed[p] && !Solved[p])
		{
			return p;
		}
	}
	return -1;
}

// Ayarlar / Emeği Geçenler: kamera takvime yaklaşır → sayfa kalkar. Geri: sayfa iner → kamera menü açısına döner.
void ADBGameMode::OpenMenuPage(EDBMenuPage Page)
{
	if (!IsMenuReady() || World->IsPageFlipping())
	{
		return;
	}
	MenuPage = Page;
	SetPhase(EDBPhase::MenuPage);
	bPageFlipPending = true;
	// Kamera takvime yaklaşırken yandan rüzgar: ses soldan sağa geçer, sayfa dalgalanır, ekranda rüzgar çizgileri
	PlaySfx(EDBSfx::Wind, 1.f, -0.9f, 1.f, 0.8f);
	WindTime = Now;
	World->Gust(1.f);
	if (APlayerController* P = PC())
	{
		P->SetViewTargetWithBlend(World->CalendarCam, CalendarBlendSeconds, VTBlend_EaseInOut, 2.f);
	}
}

void ADBGameMode::CloseMenuPage()
{
	if (!IsMenuPageReady())
	{
		return;
	}
	World->FlipPage(false);
	PlaySfx(EDBSfx::Page, 0.9f, 0.f, 0.92f);
	World->Gust(0.4f);
	bReturnPending = true;
}

bool ADBGameMode::IsMenuPageReady() const
{
	return Phase == EDBPhase::MenuPage && !bPageFlipPending && !bReturnPending && World && !World->IsPageFlipping();
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

// ---------------------------------------------------------------- kasabalılar ve ızgara

void ADBGameMode::ShowNpc(int32 Building)
{
	const FDBContent& C = FDBContent::Get();
	if (!C.Residents.IsValidIndex(Building))
	{
		return;
	}
	const int32 P = ProblemOfBuilding(Building);
	TalkedResidents.Add(Building);
	NpcBuilding = Building;
	NpcTime = Now;
	if (P != INDEX_NONE && Solved[P]) NpcKind = 2;
	else if (P != INDEX_NONE && Revealed[P]) NpcKind = 1;
	else NpcKind = 0;
}

FString ADBGameMode::NpcText() const
{
	const FDBContent& C = FDBContent::Get();
	if (!C.Residents.IsValidIndex(NpcBuilding)) return FString();
	const FDBResident& R = C.Residents[NpcBuilding];
	const FString& T = NpcKind == 1 ? R.Ask : (NpcKind == 2 ? R.Thanks : R.Hello);
	return T.IsEmpty() ? R.Hello : T;
}

bool ADBGameMode::NpcWantsBag() const
{
	const int32 P = ProblemOfBuilding(NpcBuilding);
	return NpcKind == 1 && P != INDEX_NONE && FDBContent::Get().Problems[P].Kit >= 0 && !Solved[P];
}

void ADBGameMode::NpcOpenFactory()
{
	const int32 P = ProblemOfBuilding(NpcBuilding);
	if (P != INDEX_NONE)
	{
		SelectOrder(P);
		bPanelOpen = true;
	}
	CloseNpc();
}

void ADBGameMode::ClickCell(FIntPoint Cell)
{
	if (Phase != EDBPhase::Play)
	{
		return;
	}
	CloseNpc();
	const FDBContent& C = FDBContent::Get();
	if (SelTool < 0 || SelTool != C.FloodTool)
	{
		return;
	}
	if (!World->IsRoad(Cell))
	{
		ShowToast(TEXT("Rögar kapağı yola konur."), false);
		return;
	}
	if (World->HasManhole(Cell))
	{
		ShowToast(TEXT("Burada zaten rögar kapağı var."), false);
		return;
	}
	if (!C.FloodCells.Contains(Cell))
	{
		ShowToast(TEXT("Burada su birikmiyor. Mavi su birikintilerini bul."), false);
		return;
	}
	const int32 Cost = C.Tools[C.FloodTool].Cost;
	if (Money < Cost)
	{
		ShowToast(FString::Printf(TEXT("Rögar kapağı %d TL, bütçede %d TL var. Çanta teslim ederek bütçe kazan."), Cost, Money), true);
		return;
	}
	// Bozuk kapak: oturtmak için mini oyun; bedel kazanınca düşer
	StartMini(EDBMini::Manhole, C.FloodProblem, -1, Cell);
}

void ADBGameMode::ApplyManhole(FIntPoint Cell)
{
	const FDBContent& C = FDBContent::Get();
	const int32 Cost = C.Tools[C.FloodTool].Cost;
	Money -= Cost;
	World->PlaceManhole(Cell);
	int32 Covered = 0;
	for (const FIntPoint& F : C.FloodCells)
	{
		Covered += World->HasManhole(F) ? 1 : 0;
	}
	const int32 FP = C.FloodProblem;
	if (Covered == C.FloodCells.Num() && Solved.IsValidIndex(FP) && Revealed[FP] && !Solved[FP])
	{
		ShowToast(FString::Printf(TEXT("Rögar kapakları tamam (-%d TL). Su artık akıp gidiyor!"), Cost), false, C.Problems[FP].Building);
		Solve(FP, C.Problems[FP].Building);
	}
	else
	{
		ShowToast(FString::Printf(TEXT("Rögar kapağı kondu (-%d TL). %d su birikintisi kaldı."), Cost, C.FloodCells.Num() - Covered), false);
	}
}

// ---------------------------------------------------------------- tahta döndürme, mini oyunlar

void ADBGameMode::TurnBoard(int32 Dir)
{
	if (World && Mini == EDBMini::None && (Phase == EDBPhase::Play || Phase == EDBPhase::BagPrep || Phase == EDBPhase::WaitScan))
	{
		CloseNpc();
		World->TurnBoard(Dir);
	}
}

void ADBGameMode::StartMini(EDBMini Kind, int32 Problem, int32 Building, FIntPoint Cell)
{
	Mini = Kind;
	MiniProblem = Problem;
	MiniBuilding = Building;
	MiniCell = Cell;
	MiniPos = 0.f;
	MiniVel = 0.f;
	MiniHoldT = 0.f;
	MiniProgress = 0.f;
	MiniT = 0.f;
	MiniWinAt = -1.f;
	MiniMsg.Reset();
	bMiniWasHold = false;
	bMiniNeedRelease = true;   // önlemi uygulayan tıklama hâlâ basılı olabilir: önce bırakılsın
	MiniZone = Kind == EDBMini::Nut ? FMath::FRandRange(0.55f, 0.8f) : 0.5f;
	MiniZoneHalf = Kind == EDBMini::Nut ? 0.06f : 0.11f;
	bPanelOpen = false;
	CloseNpc();
	UE_LOG(LogDB, Log, TEXT("DB_MINI basla tur=%d sorun=%d"), (int32)Kind, Problem);
}

void ADBGameMode::CancelMini()
{
	if (Mini != EDBMini::None && MiniWinAt < 0.f)
	{
		Mini = EDBMini::None;
		ShowToast(TEXT("Vazgeçtin; bütçeden bir şey düşmedi. İstediğin zaman yeniden dene."), false);
	}
}

void ADBGameMode::MiniSay(const FString& Text, bool bBad)
{
	MiniMsg = Text;
	MiniMsgTime = Now;
	if (bBad)
	{
		PlaySfx(EDBSfx::Bad, 0.8f);
	}
	bMiniMsgBad = bBad;
}

void ADBGameMode::TickMini(float Dt)
{
	if (Mini == EDBMini::None)
	{
		return;
	}
	MiniT += Dt;
	if (MiniWinAt >= 0.f)
	{
		if (Now - MiniWinAt >= 1.f)
		{
			FinishMini();
		}
		return;
	}
	bool bHold = bMiniHold;
	if (bMiniAutoTest)
	{
		bHold = Mini == EDBMini::Nut ? MiniPos < MiniZone - 0.012f : MiniPos + MiniVel * 0.35f < MiniZone;
	}
	if (bMiniNeedRelease && !bMiniAutoTest)
	{
		bMiniNeedRelease = bHold;
		bHold = false;
	}

	if (Mini == EDBMini::Nut)
	{
		if (bHold)
		{
			// Ne kadar uzun sıkarsan anahtar o kadar hızlanır
			MiniHoldT += Dt;
			MiniPos += (0.15f + 0.6f * MiniHoldT) * Dt;
			if (MiniPos >= 1.f)
			{
				MiniSay(TEXT("Fazla sıktın, somun boşa döndü! Bırak, baştan dene."), true);
				MiniPos = 0.f;
				MiniHoldT = 0.f;
				MiniZone = FMath::FRandRange(0.55f, 0.8f);
				bMiniNeedRelease = true;
				bHold = false;
			}
		}
		else
		{
			if (bMiniWasHold)
			{
				if (FMath::Abs(MiniPos - MiniZone) <= MiniZoneHalf)
				{
					MiniWinAt = Now;
					PlaySfx(EDBSfx::Land, 0.5f, 0.f, 2.2f);   // somun "tık" diye oturur
					MiniSay(TEXT("Tık! Somun tam yerine oturdu."), false);
				}
				else if (MiniPos < MiniZone)
				{
					MiniSay(TEXT("Biraz daha sık! Yeşil çizgiye gelince bırak."), false);
				}
				else
				{
					MiniSay(TEXT("Fazla sıktın, somun boşa döndü! Baştan dene."), true);
					MiniPos = 0.f;
					MiniZone = FMath::FRandRange(0.55f, 0.8f);
				}
			}
			MiniHoldT = 0.f;
			MiniPos = FMath::FInterpConstantTo(MiniPos, 0.f, Dt, 0.3f);   // bırakılan somun yavaşça gevşer
		}
		bMiniWasHold = bHold;
		return;
	}

	// Rögar: yeşil alan yavaşça gezinir; basılı tutunca kapak sağa, bırakınca sola kayar
	MiniZone = 0.5f + 0.3f * FMath::Sin(MiniT * 0.8f) * FMath::Cos(MiniT * 0.33f + 0.6f);
	MiniVel = FMath::Clamp(MiniVel + (bHold ? 1.7f : -1.7f) * Dt, -0.75f, 0.75f);
	MiniPos += MiniVel * Dt;
	if (MiniPos < 0.f)
	{
		MiniPos = 0.f;
		MiniVel = FMath::Max(0.f, MiniVel);
	}
	if (MiniPos > 1.f)
	{
		MiniPos = 1.f;
		MiniVel = FMath::Min(0.f, MiniVel);
	}
	const bool bIn = FMath::Abs(MiniPos - MiniZone) <= MiniZoneHalf;
	MiniProgress = bIn ? MiniProgress + Dt : FMath::Max(0.f, MiniProgress - Dt * 0.5f);   // dışarıda yavaşça geri gider
	if (MiniProgress >= ManholeHoldSeconds)
	{
		MiniProgress = ManholeHoldSeconds;
		MiniWinAt = Now;
		PlaySfx(EDBSfx::Land, 0.6f, 0.f, 1.6f);   // kapak yerine oturur
		MiniSay(TEXT("Kapak yerine oturdu!"), false);
	}
}

void ADBGameMode::FinishMini()
{
	const EDBMini Kind = Mini;
	Mini = EDBMini::None;
	const FDBContent& C = FDBContent::Get();
	UE_LOG(LogDB, Log, TEXT("DB_MINI kazandi tur=%d sure=%.1f"), (int32)Kind, MiniT);
	if (Kind == EDBMini::Manhole)
	{
		ApplyManhole(MiniCell);
		return;
	}
	if (C.Problems.IsValidIndex(MiniProblem) && !Solved[MiniProblem])
	{
		const FDBToolDef& T = C.Tools[C.WrenchTool];
		Money -= T.Cost;
		ShowToast(FString::Printf(TEXT("%s: %s (-%d TL). %s riskine hazır!"), *C.Buildings[MiniBuilding].Name, *T.Done, T.Cost, *C.Problems[MiniProblem].Hazard), false, MiniBuilding);
		Solve(MiniProblem, MiniBuilding);
	}
}

// ---------------------------------------------------------------- çanta fabrikası

TArray<int32> ADBGameMode::PendingOrders() const
{
	TArray<int32> Out;
	const FDBContent& C = FDBContent::Get();
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (C.Problems[p].Kit >= 0 && Revealed[p] && !Solved[p])
		{
			Out.Add(p);
		}
	}
	return Out;
}

void ADBGameMode::ResetBag()
{
	bBagOnBelt = true;
	BagT = 0.f;
	BagKit = -1;
	BagPersonal.Reset();
}

void ADBGameMode::SelectOrder(int32 Problem)
{
	if (!PendingOrders().Contains(Problem) || Problem == OrderProblem)
	{
		return;
	}
	OrderProblem = Problem;
	bBagOnBelt = false;
	NextBagAt = Now + 0.3f;
}

void ADBGameMode::CycleOrder(int32 Dir)
{
	const TArray<int32> P = PendingOrders();
	if (P.Num() == 0)
	{
		return;
	}
	const int32 I = P.IndexOfByKey(OrderProblem);
	SelectOrder(P[((I < 0 ? 0 : I + Dir) % P.Num() + P.Num()) % P.Num()]);
}

void ADBGameMode::TickBelt(float Dt)
{
	if (!bPanelOpen)
	{
		return;   // masa kapalıyken bant bekler
	}
	// Sipariş masa açılır açılmaz seçilir (Asil anlatırken kart boş kalmasın)
	if (OrderProblem < 0 || Solved[OrderProblem])
	{
		const TArray<int32> P = PendingOrders();
		OrderProblem = P.Num() ? P[0] : -1;
		bBagOnBelt = false;
		NextBagAt = FMath::Max(NextBagAt, Now + 0.3f);
	}
	if (Phase != EDBPhase::BagPrep && Phase != EDBPhase::Play)
	{
		return;   // konuşurken bant akmaz
	}
	if (OrderProblem < 0)
	{
		return;
	}
	if (!bBagOnBelt && Now >= NextBagAt)
	{
		ResetBag();
	}
	if (IsBeltRunning())
	{
		BagT += Dt / FMath::Max(1.f, BeltSeconds) * (bBeltFast ? BeltFastFactor : 1.f);
		if (BagT >= 1.f)
		{
			DeliverBag();
		}
	}
}

void ADBGameMode::StationClick(int32 K, int32 Button)
{
	const FDBContent& C = FDBContent::Get();
	if (!IsStationActive(K))
	{
		return;   // çanta bu istasyonun önünde değil
	}
	if (K == 0)
	{
		if (C.Kits.IsValidIndex(Button))
		{
			BagKit = Button;   // temel kit: içindekiler kendiliğinden eklenir (çizimde gösterilir)
		}
		return;
	}
	const FDBStation& St = C.Stations[K - 1];
	if (St.Items.IsValidIndex(Button))
	{
		BagPersonal.AddUnique(St.Items[Button]);
	}
}

void ADBGameMode::DeliverBag()
{
	const FDBContent& C = FDBContent::Get();
	const FDBProblemDef& Pr = C.Problems[OrderProblem];
	const FString& Name = C.Buildings[Pr.Building].Name;
	TArray<FString> Missing, Extra;
	for (int32 It : Pr.Personal)
	{
		if (!BagPersonal.Contains(It)) Missing.Add(C.Items[It].Name);
	}
	for (int32 It : BagPersonal)
	{
		if (!Pr.Personal.Contains(It)) Extra.Add(C.Items[It].Name);
	}
	bBagOnBelt = false;
	DeliverTime = Now;
	NextBagAt = Now + 1.3f;
	if (BagKit == Pr.Kit && Missing.Num() == 0 && Extra.Num() == 0)
	{
		bDeliverOk = true;
		bFactoryTutorial = false;
		Money += C.KitReward;
		ShowToast(FString::Printf(TEXT("%s teslim edildi: %s aldı (+%d TL)."), *C.Kits[Pr.Kit].Name, *Name, C.KitReward), false, Pr.Building);
		const int32 P = OrderProblem;
		OrderProblem = -1;
		Solve(P, Pr.Building);
		return;
	}
	bDeliverOk = false;
	++Mistakes;
	FString Why;
	if (BagKit < 0) Why = TEXT("Temel kit seçilmedi.");
	else if (BagKit != Pr.Kit) Why = FString::Printf(TEXT("%s istiyordu, bu %s."), *C.Kits[Pr.Kit].Name, *C.Kits[BagKit].Name);
	if (Missing.Num()) Why += FString::Printf(TEXT(" Eksik: %s."), *FString::Join(Missing, TEXT(", ")));
	if (Extra.Num()) Why += FString::Printf(TEXT(" Fazla: %s."), *FString::Join(Extra, TEXT(", ")));
	ShowToast(FString::Printf(TEXT("%s için çanta yanlış.%s Çanta baştan geliyor."), *Name, *Why), true, Pr.Building);
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

void ADBGameMode::SetDraftKit(int32 Kit)
{
	if (CanEditBags() && FDBContent::Get().Kits.IsValidIndex(Kit))
	{
		DraftKit = Kit;
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
	// Kombinasyon kontrolü: seçilen türün tam 3 eşyası olmalı (ekip: "doğru kombinasyonu yapıp devam etmek")
	const FDBContent& C = FDBContent::Get();
	const TArray<int32> Need = C.KitItems(DraftKit);
	TArray<FString> Wrong, Missing;
	for (int32 It : Draft)
	{
		if (!Need.Contains(It))
		{
			Wrong.Add(C.Items[It].Name);
		}
	}
	for (int32 It : Need)
	{
		if (!Draft.Contains(It))
		{
			Missing.Add(C.Items[It].Name);
		}
	}
	if (Wrong.Num() > 0 || Missing.Num() > 0)
	{
		++Mistakes;
		FString Why = FString::Printf(TEXT("%s böyle olmaz."), *C.Kits[DraftKit].Name);
		if (Wrong.Num() > 0)
		{
			Why += FString::Printf(TEXT(" Girmez: %s."), *FString::Join(Wrong, TEXT(", ")));
		}
		if (Missing.Num() > 0)
		{
			Why += Wrong.Num() > 0 ? FString(TEXT(" Bir eşya daha gerekiyor.")) : FString::Printf(TEXT(" %d eşya eksik."), Missing.Num());
		}
		ShowToast(Why, true);
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
	Bags[Slot].Kit = DraftKit;
	Bags[Slot].bUsed = false;
	Draft.Reset();
	ShowToast(FString::Printf(TEXT("%s hazır! Alttaki sırt çantasına eklendi."), *FDBContent::Get().Kits[DraftKit].Name), false);
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

FString ADBGameMode::MissingItemsText(int32 Kit, const TArray<int32>& BagItems) const
{
	const FDBContent& C = FDBContent::Get();
	TArray<FString> Missing;
	for (int32 Item : C.KitItems(Kit))
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
	if (bBad)
	{
		PlaySfx(EDBSfx::Bad, 0.7f);
		FxBadTime = Now;
	}
	ToastBuilding = Building;
}

void ADBGameMode::Penalty(const FString& Why, int32 Building)
{
	++Mistakes;
	ShowToast(Why, true, Building);
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
	World->JiggleBuilding(Building);   // seçildiği belli olsun

	if (P == INDEX_NONE || !Revealed[P] || Solved[P])
	{
		if (SelTool < 0)
		{
			ShowNpc(Building);
		}
		else
		{
			ShowToast(FString::Printf(TEXT("%s: burada bir sorun yok."), *Name), false, Building);
		}
		return;
	}
	const FDBProblemDef& Prob = C.Problems[P];

	if (SelTool < 0 && SelBag < 0)
	{
		ShowNpc(Building);
		return;
	}
	if (SelTool >= 0 && SelTool == C.FloodTool && Prob.Tool == C.FloodTool)
	{
		ShowToast(TEXT("Rögar kapağını binaya değil, su birikintisinin olduğu yol ızgarasına koy."), false, Building);
		return;
	}

	if (SelTool >= 0)
	{
		const FDBToolDef& Tool = C.Tools[SelTool];
		if (Prob.Tool == SelTool)
		{
			if (Money < Tool.Cost)
			{
				// Hata sayılmaz: doğru önlem, bütçe yetmiyor
				ShowToast(FString::Printf(TEXT("%s için %d TL gerekiyor, bütçede %d TL var. Çanta göndererek bütçe kazan."), *Tool.Name, Tool.Cost, Money), true, Building);
				return;
			}
			if (SelTool == C.WrenchTool)
			{
				StartMini(EDBMini::Nut, P, Building, FIntPoint(-1, -1));   // somunu sıkma mini oyunu; bedel kazanınca
				return;
			}
			Money -= Tool.Cost;
			ShowToast(FString::Printf(TEXT("%s: %s (-%d TL). %s riskine hazır!"), *Name, *Tool.Done, Tool.Cost, *Prob.Hazard), false, Building);
			Solve(P, Building);
		}
		else if (Prob.Tool < 0)
		{
			Penalty(FString::Printf(TEXT("%s bir şehir önlemi değil, %s istiyor."), *Name, *C.Kits[Prob.Kit].Name), Building);
		}
		else
		{
			Penalty(FString::Printf(TEXT("%s burada işe yaramaz: buradaki risk %s. Notu tekrar oku."), *Tool.Name, *Prob.Hazard), Building);
		}
		return;
	}

	// Çanta
	FDBBag& Bag = Bags[SelBag];
	if (Prob.Kit < 0)
	{
		Penalty(FString::Printf(TEXT("Çanta burada işe yaramaz: %s riski için şehir önlemi gerekir."), *Prob.Hazard), Building);
		return;
	}
	if (Bag.Kit != Prob.Kit)
	{
		Penalty(FString::Printf(TEXT("%s %s istiyor, bu bir %s."), *Name, *C.Kits[Prob.Kit].Name, *C.Kits[Bag.Kit].Name), Building);
		return;
	}
	const FString Missing = MissingItemsText(Prob.Kit, Bag.Items);
	if (!Missing.IsEmpty())
	{
		Penalty(FString::Printf(TEXT("%s eksik: %s."), *C.Kits[Prob.Kit].Name, *Missing), Building);
		return;
	}
	Bag.bUsed = true;
	SelBag = -1;
	Money += C.KitReward;
	ShowToast(FString::Printf(TEXT("%s %s aldı (+%d TL). Hazırlar!"), *Name, *C.Kits[Prob.Kit].Name, C.KitReward), false, Building);
	Solve(P, Building);
}

void ADBGameMode::Solve(int32 Problem, int32 Building)
{
	Solved[Problem] = true;
	PlaySfx(EDBSfx::Good, 1.f);
	FxGoodTime = Now;
	FxBuilding = Building;
	World->Shake(0.6f);
	SolveTime[Problem] = Now;
	SelTool = -1;
	World->PopBuilding(Building);
	World->ClearHaze(Building);
	if (Building == FDBContent::Get().HydrantBuilding)
	{
		World->RepairHydrant();
	}
	if (Building == FDBContent::Get().NewHydrantBuilding)
	{
		World->PlaceNewHydrant();
	}
	// Kasabalı teşekkür eder
	NpcBuilding = Building;
	NpcKind = 2;
	NpcTime = Now;
	const FDBContent& C = FDBContent::Get();
	if (C.Problems[Problem].Tool >= 0 && C.Problems[Problem].Tool == C.EvacuateTool)
	{
		World->StartEvacuation(Building, C.ShelterBuilding);
	}
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
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true, bDirect ? TEXT("Direkt=1") : TEXT("Menu=1"));
}

void ADBGameMode::Back()
{
	if (bMenuConfirm)
	{
		bMenuConfirm = false;
		return;
	}
	// Oyunda kapatılacak bir şey yoksa Esc / sağ tık duraklatma menüsünü açar
	if (Phase == EDBPhase::Play && Mini == EDBMini::None && NpcBuilding < 0 && SelTool < 0 && SelBag < 0 && !bPanelOpen)
	{
		AskMenu();
		return;
	}
	switch (Phase)
	{
	case EDBPhase::MenuPage: CloseMenuPage(); break;
	case EDBPhase::BagPrep: ClosePanel(); break;
	case EDBPhase::Play:
		if (Mini != EDBMini::None)
		{
			CancelMini();
		}
		else if (NpcBuilding >= 0)
		{
			CloseNpc();
		}
		else if (SelTool >= 0 || SelBag >= 0)
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
