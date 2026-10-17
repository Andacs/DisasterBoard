#include "DBPlayerController.h"
#include "DBGameMode.h"
#include "DBHUD.h"
#include "DBWorld.h"
#include "DBMusic.h"
#include "Misc/Paths.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "AudioMixerBlueprintLibrary.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogDBInput, Log, All);

ADBPlayerController::ADBPlayerController()
{
	bShowMouseCursor = true;
	bAutoManageActiveCameraTarget = false; // görüş hedefini GameMode yönetir (menü kamerası / tahta kamerası)
}

ADBGameMode* ADBPlayerController::GM() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<ADBGameMode>() : nullptr;
}

void ADBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);

	if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Sub->AddMappingContext(Mapping, 0);
	}
	// Paketlenmiş oyunda konsol olmayabilir: görsel testi komut satırından başlar
	if (FParse::Param(FCommandLine::Get(), TEXT("DBSesTest")))
	{
		FTimerHandle Start;
		GetWorldTimerManager().SetTimer(Start, FTimerDelegate::CreateUObject(this, &ADBPlayerController::DBSesTest), 1.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("DBVarlikTest")))
	{
		FTimerHandle Start;
		GetWorldTimerManager().SetTimer(Start, FTimerDelegate::CreateUObject(this, &ADBPlayerController::DBVarlikTest), 1.f, false);
	}
}

void ADBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_DisasterBoard"));
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogDBInput, Error, TEXT("EnhancedInputComponent yok: DefaultInput.ini DefaultInputComponentClass'a bak"));
		return;
	}
	auto Bind = [this, EIC](const TCHAR* Name, std::initializer_list<FKey> Keys, void (ADBPlayerController::*Handler)())
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		for (const FKey& Key : Keys)
		{
			Mapping->MapKey(Action, Key);
		}
		EIC->BindAction(Action, ETriggerEvent::Started, this, Handler);
	};
	Bind(TEXT("IA_Click"), { EKeys::LeftMouseButton }, &ADBPlayerController::OnClick);
	Bind(TEXT("IA_Back"), { EKeys::Escape }, &ADBPlayerController::OnBack);
	Bind(TEXT("IA_RotLeft"), { EKeys::Q }, &ADBPlayerController::OnRotLeft);
	Bind(TEXT("IA_RotRight"), { EKeys::E }, &ADBPlayerController::OnRotRight);
	// Sağ tık: "geri" (seçimi bırak / masayı kapat / mini oyundan vazgeç). Andaç: sağ tıkla sürükleyip döndürme yok;
	// tahta alttaki oklarla (ya da Q/E) döner.
	Bind(TEXT("IA_Rmb"), { EKeys::RightMouseButton }, &ADBPlayerController::OnBack);
	{
		UInputAction* Zoom = NewObject<UInputAction>(this, TEXT("IA_Zoom"));
		Zoom->ValueType = EInputActionValueType::Axis1D;
		Mapping->MapKey(Zoom, EKeys::MouseWheelAxis);
		EIC->BindAction(Zoom, ETriggerEvent::Triggered, this, &ADBPlayerController::OnZoom);
	}
	Bind(TEXT("IA_Restart"), { EKeys::F5 }, &ADBPlayerController::OnRestart);
	Bind(TEXT("IA_Music"), { EKeys::M }, &ADBPlayerController::OnMusic);
}

void ADBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	HoveredBuilding = -1;
	HoveredDesk = -1;
	HoveredCar = -1;
	bHoveredCell = false;
	ADBGameMode* G = GM();
	if (G && G->World)
	{
		G->World->HoverBuilding = G->World->HoverCar = G->World->HoverDesk = -1;
	}
	ADBHUD* H = GetHUD<ADBHUD>();
	float MX = 0.f, MY = 0.f;
	const bool bMouse = GetMousePosition(MX, MY);
	bool bHand = false;

	const bool bLmbNow = IsInputKeyDown(EKeys::LeftMouseButton);
	if (G && G->bBeltFast && bLmbPrev && !bLmbNow)
	{
		G->bBeltFast = false;   // TESLİM bırakıldı
	}
	bLmbPrev = bLmbNow;
	if (G && G->bMenuConfirm)
	{
		CurrentMouseCursor = (bMouse && H && H->IsOverButton(FVector2D(MX, MY))) ? EMouseCursor::Hand : EMouseCursor::Default;
		return;
	}

	// Mini oyun: fare sol tuşu (Vazgeç'in dışında basıldıysa) ya da boşluk basılı tutulur; tahtaya tıklanmaz
	if (G && G->Mini != EDBMini::None)
	{
		const bool bLmb = IsInputKeyDown(EKeys::LeftMouseButton);
		if (!bLmb)
		{
			bMiniMouse = false;
		}
		G->bMiniHold = (bLmb && bMiniMouse) || IsInputKeyDown(EKeys::SpaceBar);
		CurrentMouseCursor = (bMouse && H && H->IsOverButton(FVector2D(MX, MY))) ? EMouseCursor::Hand : EMouseCursor::Default;
		return;
	}
	if (bMouse && H)
	{
		bHand = H->IsOverButton(FVector2D(MX, MY));
		if (G && G->World && G->IsBoardPhase() && !H->IsOverUI(FVector2D(MX, MY)))
		{
			FHitResult Hit;
			if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
			{
				HoveredBuilding = G->World->BuildingFromComponent(Hit.GetComponent());
				bHand |= (HoveredBuilding >= 0 && G->Phase == EDBPhase::Play);
				// Masadaki oyuncaklar her zaman tıklanır
				HoveredDesk = HoveredBuilding < 0 ? G->World->DeskFromComponent(Hit.GetComponent()) : -1;
				bHand |= HoveredDesk >= 0;
				// Bina değilse tahtanın hangi karosu (rögar kapağı ızgaraya konur)
				bHoveredCell = HoveredBuilding < 0 && HoveredDesk < 0 && G->World->CellFromWorld(Hit.ImpactPoint, HoveredCell);
				bHand |= bHoveredCell && G->SelTool >= 0 && G->SelTool == FDBContent::Get().FloodTool && G->World->IsRoad(HoveredCell);
			}
		}
	}
	if (bMouse && H && G && G->World && G->IsBoardPhase() && G->Phase != EDBPhase::Result && !H->IsOverUI(FVector2D(MX, MY)) && HoveredDesk < 0)
	{
		int32 SX = 1920, SY = 1080;
		GetViewportSize(SX, SY);
		HoveredCar = G->World->CarAtScreen(this, FVector2D(MX, MY), 30.f * SY / 1080.f);
		bHand |= HoveredCar >= 0;
	}
	if (G && G->World)
	{
		G->World->HoverBuilding = HoveredBuilding;
		G->World->HoverCar = HoveredCar;
		G->World->HoverDesk = HoveredDesk;
	}
	CurrentMouseCursor = bHand ? EMouseCursor::Hand : EMouseCursor::Default;
}

void ADBPlayerController::OnClick()
{
	ADBGameMode* G = GM();
	ADBHUD* H = GetHUD<ADBHUD>();
	if (!G || !H)
	{
		return;
	}
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	H->AddClickFx(FVector2D(MX, MY));   // her tıklamada halka + kıvılcım
	if (G->bMenuConfirm)
	{
		H->HandleClick(FVector2D(MX, MY));
		return;
	}
	if (G->Mini != EDBMini::None)
	{
		bMiniMouse = !H->HandleClick(FVector2D(MX, MY));   // Vazgeç değilse basılı tutma başlar
		return;
	}
	if (H->HandleClick(FVector2D(MX, MY)))
	{
		return;
	}
	// Masadaki oyuncak: kupa çınlar, zar atılır, ördek vıklar, lamba yanar
	if (HoveredDesk >= 0 && G->World)
	{
		G->World->PokeDesk(HoveredDesk);
		return;
	}
	// Arabaya tıklamak: korna çalar, araba zıplar (Andaç)
	if (G->World && G->IsBoardPhase() && G->Phase != EDBPhase::ToBoard && G->Phase != EDBPhase::Result)
	{
		int32 SX = 1920, SY = 1080;
		GetViewportSize(SX, SY);
		const int32 Car = G->World->CarAtScreen(this, FVector2D(MX, MY), 30.f * SY / 1080.f);
		if (Car >= 0)
		{
			G->World->HonkCar(Car);
			return;
		}
	}
	if (HoveredBuilding >= 0)
	{
		G->PlaySfx(EDBSfx::Click, 0.75f, 0.f, FMath::FRandRange(0.8f, 0.9f));
		G->ClickBuilding(HoveredBuilding);
	}
	else if (bHoveredCell)
	{
		G->PlaySfx(EDBSfx::Click, 0.6f, 0.f, 0.8f);
		G->ClickCell(HoveredCell);
	}
	else
	{
		G->CloseNpc();
	}
}

bool ADBPlayerController::CanControlBoard() const
{
	const ADBGameMode* G = GM();
	return G && G->World && G->IsBoardPhase() && G->Phase != EDBPhase::ToBoard && G->Phase != EDBPhase::Result;
}

void ADBPlayerController::OnZoom(const FInputActionValue& Value)
{
	const ADBHUD* H = GetHUD<ADBHUD>();
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	if (CanControlBoard() && !(H && H->IsOverUI(FVector2D(MX, MY))))
	{
		GM()->World->ZoomBoardView(-Value.Get<float>() * 30.f);
	}
}

void ADBPlayerController::OnRotLeft()
{
	if (ADBGameMode* G = GM()) G->TurnBoard(-1);
}

void ADBPlayerController::OnRotRight()
{
	if (ADBGameMode* G = GM()) G->TurnBoard(1);
}

void ADBPlayerController::OnBack()
{
	if (ADBGameMode* G = GM())
	{
		G->Back();
	}
}

void ADBPlayerController::OnRestart()
{
	if (ADBGameMode* G = GM())
	{
		G->Restart(true);
	}
}

void ADBPlayerController::DBExposure(float EV100)
{
	if (ADBGameMode* G = GM(); G && G->World)
	{
		G->World->ApplyExposure(EV100);
	}
}

void ADBPlayerController::DBTime(float Seconds)
{
	if (ADBGameMode* G = GM())
	{
		G->SetTimeLeft(Seconds);
	}
}

void ADBPlayerController::DBQuake()
{
	if (ADBGameMode* G = GM())
	{
		G->ForceQuake();
	}
}

void ADBPlayerController::OnMusic()
{
	if (ADBGameMode* G = GM())
	{
		G->ToggleMusic();
	}
}

void ADBPlayerController::DBMusicWav()
{
	const FString Path = FPaths::ProjectSavedDir() / TEXT("DB_muzik_test.wav");
	FString Report;
	const bool bOk = UDBMusicWave::WriteTestWav(Path, 40.f, &Report);
	UE_LOG(LogDBInput, Log, TEXT("DB_MUSIC wav=%s ok=%d %s"), *Path, bOk ? 1 : 0, *Report);
}

void ADBPlayerController::DBPageGain(float Gain)
{
	if (ADBGameMode* G = GM(); G && G->World)
	{
		G->World->PageGain = Gain;
	}
}

// ---------------------------------------------------------------- otomatik test

void ADBPlayerController::Shot(const FString& Name)
{
	FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("DB_%s.png"), *Name), true, false);
	UE_LOG(LogDBInput, Log, TEXT("DB_TEST shot %s"), *Name);
}

void ADBPlayerController::DBTest()
{
	ADBGameMode* G = GM();
	if (!G)
	{
		return;
	}
	TestSteps.Reset();
	TestStep = 0;
	auto Add = [this](TFunction<void()> F) { TestSteps.Add(MoveTemp(F)); };
	auto Talk = [this, Add](int32 Times) { for (int32 i = 0; i < Times; ++i) { Add([this] { GM()->AdvanceTalk(); GM()->AdvanceTalk(); }); } };

	if (FParse::Param(FCommandLine::Get(), TEXT("DBGainTest")))
	{
		for (float Gain : { 0.4f, 0.7f, 1.0f, 1.4f, 2.0f })
		{
			Add([this, Gain] { DBPageGain(Gain); });
			Add([this, Gain] { Shot(FString::Printf(TEXT("gain_%03d"), FMath::RoundToInt(Gain * 100.f))); });
		}
		Add([] { FGenericPlatformMisc::RequestExit(false); });
		GetWorldTimerManager().SetTimer(TestTimer, this, &ADBPlayerController::RunTestStep, 0.6f, true, 2.f);
		return;
	}
	// Fabrika yardımcısı: çantayı istasyonların önüne getirip tıklar (bant durmadığı için test konumu doğrudan ayarlar)
	auto Fab = [this](int32 Kit, TArray<int32> Personal)
	{
		ADBGameMode* M = GM();
		const FDBContent& C = FDBContent::Get();
		M->SetBagT(ADBGameMode::StationCenter(0));
		M->StationClick(0, Kit);
		for (int32 It : Personal)
		{
			for (int32 K = 1; K <= C.Stations.Num(); ++K)
			{
				const int32 B = C.Stations[K - 1].Items.IndexOfByKey(It);
				if (B != INDEX_NONE)
				{
					M->SetBagT(ADBGameMode::StationCenter(K));
					M->StationClick(K, B);
				}
			}
		}
		M->SetBagT(0.85f);
	};
	// Oyun hikâyeyle açılır (menüden önce)
	Add([this] { Shot(TEXT("00a_hikaye_1")); });
	Add([this] { GM()->SkipComic(); });
	Add([this] { Shot(TEXT("01_menu")); GM()->OpenMenuPage(EDBMenuPage::Settings); });
	Add([] {});
	Add([this] { Shot(TEXT("01b_ayarlar")); GM()->CloseMenuPage(); });
	Add([] {});
	Add([] {});
	Add([this] { GM()->OpenMenuPage(EDBMenuPage::Credits); });
	Add([] {});
	Add([this] { Shot(TEXT("01c_emegi_gecenler")); GM()->CloseMenuPage(); });
	Add([] {});
	Add([] {});
	Add([this] { GM()->ClickPlay(); });
	Add([] {});
	Add([] {});
	Talk(2);
	Add([this] { Shot(TEXT("04b_A_tarayiciyi_gosteriyor")); UE_LOG(LogDBInput, Log, TEXT("DB_TEST tutorial satir_tamam=%d"), GM()->IsLineComplete() ? 1 : 0); });
	// Etkileşimli tutorial: yanlış yere tıklamak geçmez, tarayıcıya tıklamak taramayı başlatır
	Add([this]
	{
		int32 SX = 1600, SY = 900;
		GetViewportSize(SX, SY);
		ADBHUD* H = GetHUD<ADBHUD>();
		H->HandleClick(FVector2D(SX * 0.5f, SY * 0.5f));
		UE_LOG(LogDBInput, Log, TEXT("DB_TEST tutorial yanlis_yer phase=%d"), (int32)GM()->Phase);
		Shot(TEXT("04c_bu_tusa_bas"));
	});
	Add([this]
	{
		ADBHUD* H = GetHUD<ADBHUD>();
		H->HandleClick(H->DoTargetRect().GetCenter());
		UE_LOG(LogDBInput, Log, TEXT("DB_TEST tutorial tarayici phase=%d"), (int32)GM()->Phase);
	});
	Add([this] { Shot(TEXT("06_taraniyor")); });
	Add([] {});
	Add([this] { Shot(TEXT("07_istekler")); });
	Talk(1);
	Add([this] { Shot(TEXT("08_fabrika_B")); });
	Talk(2);
	Add([this] { Shot(TEXT("08b_B_temel_kiti_gosteriyor")); });
	Talk(4);
	// Eşyalar: 8 Kalp İlacı, 9 Astım Spreyi, 10 Gözlük, 11 Bebek Maması, 12 Bebek Bezi, 13 Kedi Maması, 14 Battaniye, 15 Powerbank, 16 Belgeler
	// Siparişler sırası: Ayşe (İY + kalp ilacı, gözlük), Okul (İY + astım), Yılmaz (Deprem + mama, bez), Öğrenci (Deprem + powerbank, kedi maması), Fırın (Yangın + belgeler, battaniye)
	Add([this] { Shot(TEXT("09_bant_akiyor")); });
	Add([this, Fab] { Fab(0, { 8, 10 }); });
	// TESLİM basılı: bant hızlı (1,3 sn'de normalde ~0,09 ilerler)
	Add([this] { GM()->SetBagT(0.3f); GM()->bBeltFast = true; });
	Add([this]
	{
		UE_LOG(LogDBInput, Log, TEXT("DB_TEST teslim_hizli BagT 0.30 -> %.2f"), GM()->BagT);
		Shot(TEXT("09a_teslim_hizli"));
		GM()->SetBagT(0.3f);   // hızlı kalırken bir sonraki adımda teslime varmasın
	});
	Add([this] { GM()->bBeltFast = false; GM()->SetBagT(0.85f); });
	// Paket açık çantaya düşer, çanta kapanır (0,9 sn), sonra teslim: çanta uçup gider
	Add([this]
	{
		Shot(TEXT("09b_canta_hazir_iniyor"));
		GM()->SetBagT(0.999f);
		FTimerHandle Hh;
		GetWorldTimerManager().SetTimer(Hh, FTimerDelegate::CreateLambda([this] { Shot(TEXT("09c_canta_kapaniyor")); }), 0.6f, false);
	});
	Add([this] { Shot(TEXT("09c2_teslim_edildi")); });
	// 3 sipariş (revizyon: her kitten bir). Yılmazlar: önce bilerek eksik (mama ve bez yok), sonra doğru
	Add([this, Fab] { Fab(1, {}); GM()->SetBagT(0.999f); });
	Add([this] { Shot(TEXT("09d_yanlis_canta")); });
	Add([] {});
	Add([this, Fab] { Fab(1, { 11, 12 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this, Fab] { Fab(2, { 16, 14 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this] { GM()->ClosePanel(); });
	Add([this] { Shot(TEXT("11_C_konusuyor")); });
	Talk(5);
	// Q / E dersi: oklara basılmadan geçmez; E'ye basmak tahtayı döndürür ve satırı geçirir
	Add([this] { Shot(TEXT("11b_QE_dersi")); UE_LOG(LogDBInput, Log, TEXT("DB_TEST qe_dersi do=%d"), GM()->DoLineTarget()); });
	Add([this] { OnRotRight(); });
	Add([this] { UE_LOG(LogDBInput, Log, TEXT("DB_TEST qe_sonra do=%d phase=%d"), GM()->DoLineTarget(), (int32)GM()->Phase); });
	Add([this] { OnRotLeft(); });
	Talk(1);
	Add([this] { Shot(TEXT("12_oyun")); });
	// Masadaki oyuncaklar: hepsini kurcala (kupa, iki zar, ördek, lamba yanar), hareketin ortasında çek
	Add([this]
	{
		ADBWorld* Wd = GM()->World;
		for (int32 i = 0; i < Wd->NumDesk(); ++i) Wd->PokeDesk(i);
		FTimerHandle Hh;
		GetWorldTimerManager().SetTimer(Hh, FTimerDelegate::CreateLambda([this] { Shot(TEXT("12d_masa_oyuncaklari")); }), 0.3f, false);
		UE_LOG(LogDBInput, Log, TEXT("DB_TEST masa oyuncak=%d lamba=%d"), Wd->NumDesk(), Wd->bLampOn ? 1 : 0);
	});
	Add([this] { Shot(TEXT("12e_lamba_acik")); GM()->World->PokeDesk(GM()->World->NumDesk() - 1); });
	// Tahta: sağ okla kalkar, döner, iner (havadayken görüntü), sonra geri; tekerlekle yakınlaştır
	Add([this]
	{
		GM()->TurnBoard(1);
		GM()->World->ZoomBoardView(-60.f);
		FTimerHandle Mid;
		GetWorldTimerManager().SetTimer(Mid, FTimerDelegate::CreateLambda([this] { Shot(TEXT("12b_tahta_havada_donuyor")); }), 0.6f, false);
	});
	Add([this] { Shot(TEXT("12c_tahta_dondu")); GM()->TurnBoard(-1); GM()->World->ZoomBoardView(60.f); });
	Add([] {});
	// Binalar: 4 Apartman, 5 Kitapçı, 7 Market, 8 Öğrenci, 10 Sahil, 11 Ahşap. Önlemler: 0 rögar, 1 tahliye, 2 musluk, 3 anahtar, 4 güçlendirme
	// Ünleme tıklamak binaya tıklamakla aynı (gerçek tıklama yolu: HUD işaret alanı), bina jöle gibi esner
	Add([this]
	{
		FVector2D P;
		if (ProjectWorldLocationToScreen(GM()->World->MarkerAnchor(11), P))
		{
			const bool bHit = GetHUD<ADBHUD>()->HandleClick(P);
			UE_LOG(LogDBInput, Log, TEXT("DB_TEST unlem_tik hit=%d npc=%d"), bHit ? 1 : 0, GM()->NpcBuilding);
		}
		FTimerHandle Hh;
		GetWorldTimerManager().SetTimer(Hh, FTimerDelegate::CreateLambda([this] { Shot(TEXT("12a_unlem_tiklandi_jole")); }), 0.12f, false);
	});
	Add([this] { GM()->ClickBuilding(11); });   // Nermin Hanım derdini anlatır
	Add([this] { Shot(TEXT("13_kasabali_konusuyor")); });
	Add([this] { ADBGameMode* M = GM(); M->CloseNpc(); M->SelectTool(1); M->ClickBuilding(4); });
	Add([this] { Shot(TEXT("15_yanlis_onlem")); });
	// Rögar: bozuk kapağı yeşil alanda 4 sn tut (test kendisi oynar)
	Add([this] { ADBGameMode* M = GM(); M->bMiniAutoTest = true; M->SelectTool(0); M->ClickCell(FIntPoint(7, 5)); });
	Add([this] { Shot(TEXT("15b_rogar_oyunu")); });
	Add([] {});
	Add([] {});
	Add([] {});
	Add([this] { Shot(TEXT("15b2_rogar_a")); });
	Add([this] { Shot(TEXT("15c_rogar_kondu")); GM()->ClickCell(FIntPoint(8, 6)); });
	Add([] {});
	Add([] {});
	Add([] {});
	Add([] {});
	Add([] {});
	// İngiliz anahtarı: somunu yeşil çizgiye kadar sık
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(3); M->ClickBuilding(11); });
	Add([this] { Shot(TEXT("15d_somun_oyunu")); });
	Add([this] { Shot(TEXT("15d2_somun_a")); });
	Add([this] { Shot(TEXT("15d3_somun_b")); });
	Add([this] { Shot(TEXT("15e_musluk_tamir")); });
	// Yangın musluğu: kitapçının sokağına yenisi
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(2); M->ClickBuilding(5); });
	Add([this] { Shot(TEXT("15f_yeni_musluk")); });
	// Duraklatma: menü → Takvime dön (ilerleme korunur) → takvimde "Devam et" → kalınan yer
	Add([this] { GM()->AskMenu(); });
	Add([this] { Shot(TEXT("17a_duraklat")); GM()->bMenuConfirm = false; });
	// Menü düğmesi: doğrudan takvime (gerçek tıklama yolu)
	Add([this]
	{
		ADBHUD* H = GetHUD<ADBHUD>();
		const float Sc = H ? H->UiScale() : 1.f;
		const bool bHit = H && H->HandleClick(FVector2D(80.f * Sc, 42.f * Sc));
		UE_LOG(LogDBInput, Log, TEXT("DB_TEST menu_dugmesi hit=%d paused=%d"), bHit ? 1 : 0, GM()->bPausedGame ? 1 : 0);
	});
	Add([] {});
	Add([this] { Shot(TEXT("17b_takvim_devam")); UE_LOG(LogDBInput, Log, TEXT("DB_TEST takvim phase=%d paused=%d"), (int32)GM()->Phase, GM()->bPausedGame ? 1 : 0); });
	Add([this] { GM()->ClickPlay(); });
	Add([] {});
	Add([this] { UE_LOG(LogDBInput, Log, TEXT("DB_TEST devam phase=%d cozulen=%d"), (int32)GM()->Phase, GM()->NumSolved()); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(4); M->ClickBuilding(4); M->SelectTool(1); M->ClickBuilding(10); });
	Add([this] { Shot(TEXT("16_tahliye_konfeti")); });
	Add([] {});
	Add([this] { Shot(TEXT("16b_basarim")); });
	Add([] {});
	Add([this] { Shot(TEXT("18_sonuc")); });
	Add([] {});
	Add([] {});
	Add([this] { Shot(TEXT("18b_sonuc_album")); UE_LOG(LogDBInput, Log, TEXT("DB_TEST basarim=%d/%d"), GM()->NumAchieved(), ADBGameMode::Achievements().Num()); });
	Add([this] { UE_LOG(LogDBInput, Log, TEXT("DB_TEST DONE para=%d hata=%d"), GM()->Money, GM()->Mistakes); });
	Add([] { FGenericPlatformMisc::RequestExit(false); });

	GetWorldTimerManager().SetTimer(TestTimer, this, &ADBPlayerController::RunTestStep, 1.3f, true, 2.f);
}

void ADBPlayerController::TestFab(int32 Kit, const TArray<int32>& Personal)
{
	ADBGameMode* M = GM();
	const FDBContent& C = FDBContent::Get();
	M->SetBagT(ADBGameMode::StationCenter(0));
	M->StationClick(0, Kit);
	for (int32 It : Personal)
	{
		for (int32 K = 1; K <= C.Stations.Num(); ++K)
		{
			const int32 B = C.Stations[K - 1].Items.IndexOfByKey(It);
			if (B != INDEX_NONE)
			{
				M->SetBagT(ADBGameMode::StationCenter(K));
				M->StationClick(K, B);
			}
		}
	}
	M->SetBagT(0.5f);
}

void ADBPlayerController::DBVarlikTest()
{
	ADBGameMode* G = GM();
	if (!G)
	{
		return;
	}
	TestSteps.Reset();
	TestStep = 0;
	auto Add = [this](TFunction<void()> F) { TestSteps.Add(MoveTemp(F)); };
	auto Talk = [this, Add](int32 Times) { for (int32 i = 0; i < Times; ++i) { Add([this] { GM()->AdvanceTalk(); GM()->AdvanceTalk(); }); } };
	auto VShot = [this](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("V_%s.png"), Name), true, false); };

	// Görüntü adı → içinde görünen görseller (Tools/varlik_kontrol.py aynı eşlemeyi kullanır)
	// Hikâye sahneleri (Cizgiroman/sahne1..4)
	Add([VShot] { VShot(TEXT("01_sahne1")); });
	Add([this] { GM()->NextComic(); });
	Add([VShot] { VShot(TEXT("02_sahne2")); });
	Add([this] { GM()->NextComic(); });
	Add([VShot] { VShot(TEXT("03_sahne3")); });
	Add([this] { GM()->NextComic(); });
	Add([VShot] { VShot(TEXT("04_sahne4")); });
	Add([this] { GM()->SkipComic(); });
	Add([] {});
	Add([] {});
	// Takvim menüsü (Arayuz/logo)
	Add([this, VShot] { VShot(TEXT("05_menu_logo")); GM()->ClickPlay(); });
	Add([] {});
	Add([] {});
	// Mert (A) tarayıcıyı gösterir (Karakterler/A_*, Arayuz/tarayici)
	Talk(2);
	Add([VShot] { VShot(TEXT("06_mert_tarayici")); });
	Talk(1);
	Add([this] { GM()->StartScan(); });
	Add([] {});
	// Uyarı işaretleri (Ikonlar/isaret_uyari)
	Add([VShot] { VShot(TEXT("07_uyari_isaretleri")); });
	Talk(1);
	// Asil (B) kendini tanıtır: duruş pozu (Karakterler/B_durus)
	Add([VShot] { VShot(TEXT("08a_asil_durus")); });
	// Asil temel kiti gösterir, Mert bekler (B_goster, A_durus), fabrika: eşya / kit ikonları, bütçe, çanta
	Talk(2);
	Add([VShot] { VShot(TEXT("08_asil_fabrika")); });
	Talk(4);
	Add([this] { TestFab(0, { 8, 10 }); });
	Add([VShot] { VShot(TEXT("09_canta_bantta")); });
	Add([this] { GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this] { GM()->ClosePanel(); });
	// Yiğit (C) kendini tanıtır (C_durus), önlem ikonları (Ikonlar/onlem_*); sonra önlemleri gösterir (C_goster)
	Add([VShot] { VShot(TEXT("10_yigit_onlemler")); });
	Talk(1);
	Add([VShot] { VShot(TEXT("10b_yigit_gosteriyor")); });
	Talk(6);
	// Tamam işareti (Ikonlar/isaret_tamam): apartmanı güçlendir
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(4); M->ClickBuilding(4); });
	Add([this] { GM()->CloseNpc(); });
	Add([VShot] { VShot(TEXT("11_tamam_isareti")); });
	// Kasabalı portreleri (Karakterler/NPC_*): 12'si tek karede
	Add([this] { GM()->bDebugPortraits = true; });
	Add([this, VShot] { VShot(TEXT("12_kasabali_portreleri")); });
	Add([this] { GM()->bDebugPortraits = false; });
	Add([] {});
	Add([this]
	{
		const ADBHUD* H = GetHUD<ADBHUD>();
		FString Path = FPaths::ProjectSavedDir() / TEXT("DB_varlik_rapor.txt");
		FParse::Value(FCommandLine::Get(), TEXT("DBRapor="), Path);
		const FString Report = (H ? H->PngReport() : FString()) + TEXT("TUR TAMAM\n");
		FFileHelper::SaveStringToFile(Report, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		UE_LOG(LogDBInput, Log, TEXT("DB_VARLIK rapor=%s"), *Path);
	});
	Add([] { FGenericPlatformMisc::RequestExit(false); });

	GetWorldTimerManager().SetTimer(TestTimer, this, &ADBPlayerController::RunTestStep, 1.1f, true, 1.f);
}

void ADBPlayerController::SesOlay(const FString& Ad)
{
	SesOlaylar.Add(FString::Printf(TEXT("%.3f\t%s"), FPlatformTime::Seconds() - SesT0, *Ad));
}

void ADBPlayerController::DBSesTest()
{
	ADBGameMode* G = GM();
	if (!G)
	{
		return;
	}
	TestSteps.Reset();
	TestStep = 0;
	SesOlaylar.Reset();
	auto Add = [this](TFunction<void()> F) { TestSteps.Add(MoveTemp(F)); };
	auto SShot = [this](const TCHAR* Name, float Delay)
	{
		const FString N = FString::Printf(TEXT("S_%s.png"), Name);
		FTimerHandle Hh;
		GetWorldTimerManager().SetTimer(Hh, FTimerDelegate::CreateLambda([N] { FScreenshotRequest::RequestScreenshot(N, true, false); }), Delay, false);
	};
	auto Center = [this]()
	{
		int32 SX = 1600, SY = 900;
		GetViewportSize(SX, SY);
		return FVector2D(SX * 0.5f, SY * 0.5f);
	};
	// 0: kayıt başlar, hikâye geçilir → menü (müzik)
	Add([this]
	{
		UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 40.f);
		SesT0 = FPlatformTime::Seconds();
		SesOlay(TEXT("kayit_basladi"));
		GM()->SkipComic();
		SesOlay(TEXT("menu_muzik"));
	});
	Add([] {});
	Add([] {});
	Add([] {});
	// 4: Ayarlar: tık + yandan rüzgar (sayfa 0,9 sn sonra kalkar)
	Add([this, SShot, Center]
	{
		GetHUD<ADBHUD>()->AddClickFx(Center());
		GM()->PlaySfx(EDBSfx::Click, 0.9f);
		SesOlay(TEXT("tik"));
		GM()->OpenMenuPage(EDBMenuPage::Settings);
		SesOlay(TEXT("ruzgar"));
		SShot(TEXT("1_takvime_ruzgar"), 0.5f);
	});
	Add([] {});
	Add([] {});
	// 7: Geri: sayfa iner
	Add([this] { GM()->CloseMenuPage(); SesOlay(TEXT("sayfa_kapat")); });
	Add([] {});
	Add([] {});
	// 10: Oyna
	Add([this] { GM()->PlaySfx(EDBSfx::Click, 0.9f); GM()->ClickPlay(); SesOlay(TEXT("oyna")); });
	Add([] {});
	Add([] {});
	// 13-15: konuşmayı tıklayarak geç (her tıklama tık sesi)
	for (int32 i = 0; i < 3; ++i)
	{
		Add([this, Center]
		{
			GM()->PlaySfx(EDBSfx::Click, 0.55f, 0.f, 1.15f);
			GM()->AdvanceTalk();
			GM()->AdvanceTalk();
			SesOlay(TEXT("konusma_tik"));
		});
	}
	// 16: tahta dönüşü: kalkış sesi, havada su sıçraması, iniş
	Add([this, SShot]
	{
		GM()->TurnBoard(1);
		SesOlay(TEXT("donus"));
		SShot(TEXT("2_donus_havada"), 0.62f);
		SShot(TEXT("3_donus_indi"), 1.55f);
	});
	Add([] {});
	Add([] {});
	// 19: fare izi ve tıklama halkası
	Add([this, SShot]
	{
		CursorT = 0.f;
		GetWorldTimerManager().SetTimer(CursorTimer, FTimerDelegate::CreateLambda([this]
		{
			CursorT += 0.016f;
			int32 SX = 1600, SY = 900;
			GetViewportSize(SX, SY);
			SetMouseLocation(FMath::RoundToInt(SX * (0.35f + 0.25f * FMath::Cos(CursorT * 5.f))), FMath::RoundToInt(SY * (0.5f + 0.2f * FMath::Sin(CursorT * 7.f))));
			if (CursorT > 1.2f)
			{
				GetWorldTimerManager().ClearTimer(CursorTimer);
			}
		}), 0.016f, true);
		FTimerHandle Cl;
		GetWorldTimerManager().SetTimer(Cl, FTimerDelegate::CreateLambda([this]
		{
			float MX = 0.f, MY = 0.f;
			GetMousePosition(MX, MY);
			GetHUD<ADBHUD>()->AddClickFx(FVector2D(MX, MY));
		}), 0.5f, false);
		SShot(TEXT("4_fare_izi_tiklama"), 0.62f);
		SesOlay(TEXT("fare_izi"));
	});
	Add([] {});
	// 21: doğru, 22: yanlış
	Add([this] { GM()->PlaySfx(EDBSfx::Good); SesOlay(TEXT("dogru")); });
	Add([this] { GM()->PlaySfx(EDBSfx::Bad); SesOlay(TEXT("yanlis")); });
	Add([] {});
	Add([this]
	{
		SesOlay(TEXT("kayit_bitti"));
		UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile, TEXT("DB_ses"), FPaths::ProjectSavedDir());
		FFileHelper::SaveStringToFile(FString::Join(SesOlaylar, TEXT("\n")) + TEXT("\n"), *(FPaths::ProjectSavedDir() / TEXT("DB_ses_olaylar.txt")),
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		UE_LOG(LogDBInput, Log, TEXT("DB_SES kayit bitti"));
	});
	Add([] {});
	Add([] { FGenericPlatformMisc::RequestExit(false); });
	GetWorldTimerManager().SetTimer(TestTimer, this, &ADBPlayerController::RunTestStep, 1.f, true, 0.5f);
}

void ADBPlayerController::RunTestStep()
{
	if (!TestSteps.IsValidIndex(TestStep))
	{
		GetWorldTimerManager().ClearTimer(TestTimer);
		return;
	}
	UE_LOG(LogDBInput, Log, TEXT("DB_TEST step %d phase=%d"), TestStep, GM() ? (int32)GM()->Phase : -1);
	TestSteps[TestStep++]();
}
