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
	// Sağ tık: sürüklenirse tahtayı döndürür, sürüklenmeden bırakılırsa "geri" (seçimi bırak / masayı kapat)
	{
		UInputAction* Rmb = NewObject<UInputAction>(this, TEXT("IA_Rmb"));
		Mapping->MapKey(Rmb, EKeys::RightMouseButton);
		EIC->BindAction(Rmb, ETriggerEvent::Started, this, &ADBPlayerController::OnRmbDown);
		EIC->BindAction(Rmb, ETriggerEvent::Completed, this, &ADBPlayerController::OnRmbUp);
	}
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

	// Sağ tıkla sürükleyerek tahtayı döndür
	if (bRmbDown)
	{
		float RX = 0.f, RY = 0.f;
		if (GetMousePosition(RX, RY))
		{
			const float Dx = RX - RmbLastX;
			RmbLastX = RX;
			RmbTotal += FMath::Abs(Dx);
			if (RmbTotal > 6.f)
			{
				bRmbDragged = true;
			}
			if (bRmbDragged && CanControlBoard())
			{
				GM()->World->RotateBoardView(Dx * 0.35f);
			}
		}
	}

	HoveredBuilding = -1;
	bHoveredCell = false;
	ADBGameMode* G = GM();
	ADBHUD* H = GetHUD<ADBHUD>();
	float MX = 0.f, MY = 0.f;
	const bool bMouse = GetMousePosition(MX, MY);
	bool bHand = false;
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
				// Bina değilse tahtanın hangi karosu (rögar kapağı ızgaraya konur)
				bHoveredCell = HoveredBuilding < 0 && G->World->CellFromWorld(Hit.ImpactPoint, HoveredCell);
				bHand |= bHoveredCell && G->SelTool >= 0 && G->SelTool == FDBContent::Get().FloodTool && G->World->IsRoad(HoveredCell);
			}
		}
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
	if (H->HandleClick(FVector2D(MX, MY)))
	{
		return;
	}
	if (HoveredBuilding >= 0)
	{
		G->ClickBuilding(HoveredBuilding);
	}
	else if (bHoveredCell)
	{
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
	if (CanControlBoard()) GM()->World->RotateBoardView(-90.f);
}

void ADBPlayerController::OnRotRight()
{
	if (CanControlBoard()) GM()->World->RotateBoardView(90.f);
}

void ADBPlayerController::OnRmbDown()
{
	float MX = 0.f, MY = 0.f;
	GetMousePosition(MX, MY);
	bRmbDown = true;
	bRmbDragged = false;
	RmbLastX = MX;
	RmbTotal = 0.f;
}

void ADBPlayerController::OnRmbUp()
{
	bRmbDown = false;
	if (!bRmbDragged)
	{
		OnBack();
	}
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
	Add([this] { Shot(TEXT("01_menu")); GM()->ClickPlay(); });
	Add([] {});
	Add([] {});
	Talk(2);
	Add([this] { Shot(TEXT("04b_A_tarayiciyi_gosteriyor")); });
	Talk(1);
	Add([this] { GM()->StartScan(); });
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
	Add([this] { Shot(TEXT("09b_canta_hazir_iniyor")); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this] { Shot(TEXT("09c_teslim_edildi")); });
	// Okul: bilerek astım spreyi unutuldu
	Add([this, Fab] { Fab(0, {}); GM()->SetBagT(0.999f); });
	Add([this] { Shot(TEXT("09d_yanlis_canta")); });
	Add([] {});
	Add([this, Fab] { Fab(0, { 9 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this, Fab] { Fab(1, { 11, 12 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this] { GM()->CycleOrder(1); });
	Add([] {});
	Add([this, Fab] { Fab(2, { 16, 14 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([this] { GM()->ClosePanel(); });
	Add([this] { Shot(TEXT("11_C_konusuyor")); });
	Talk(6);
	Add([this] { Shot(TEXT("12_oyun")); });
	// Kamera: döndür ve yakınlaştır
	Add([this] { GM()->World->RotateBoardView(90.f); GM()->World->ZoomBoardView(-90.f); });
	Add([this] { Shot(TEXT("12b_kamera_dondu_yaklasti")); GM()->World->RotateBoardView(-90.f); GM()->World->ZoomBoardView(90.f); });
	Add([] {});
	// Binalar: 4 Apartman, 7 Market, 8 Öğrenci, 10 Sahil, 11 Ahşap
	Add([this] { GM()->ClickBuilding(11); });   // Nermin Hanım derdini anlatır
	Add([this] { Shot(TEXT("13_kasabali_konusuyor")); });
	Add([this] { ADBGameMode* M = GM(); M->CloseNpc(); M->SelectTool(1); M->ClickBuilding(4); });
	Add([this] { Shot(TEXT("15_yanlis_onlem")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(0); M->ClickCell(FIntPoint(7, 5)); });
	Add([this] { Shot(TEXT("15b_rogar_izgara")); });
	Add([this] { ADBGameMode* M = GM(); M->ClickCell(FIntPoint(8, 6)); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(2); M->ClickBuilding(11); });
	Add([this] { Shot(TEXT("15c_musluk_tamir")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(3); M->ClickBuilding(4); M->SelectTool(1); M->ClickBuilding(10); });
	Add([this] { Shot(TEXT("16_tahliye")); });
	Add([this] { GM()->ClickBuilding(8); });   // Can derdini anlatır, "Çantasını hazırla"
	Add([this] { Shot(TEXT("16b_can_konusuyor")); GM()->NpcOpenFactory(); });
	Add([] {});
	Add([this] { Shot(TEXT("17_binadan_siparis")); });
	Add([this, Fab] { Fab(1, { 15, 13 }); GM()->SetBagT(0.999f); });
	Add([] {});
	Add([] {});
	Add([this] { Shot(TEXT("18_sonuc")); });
	Add([this] { UE_LOG(LogDBInput, Log, TEXT("DB_TEST DONE para=%d hata=%d"), GM()->Money, GM()->Mistakes); });
	Add([] { FGenericPlatformMisc::RequestExit(false); });

	GetWorldTimerManager().SetTimer(TestTimer, this, &ADBPlayerController::RunTestStep, 1.3f, true, 2.f);
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
