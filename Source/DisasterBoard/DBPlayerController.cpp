#include "DBPlayerController.h"
#include "DBGameMode.h"
#include "DBHUD.h"
#include "DBWorld.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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
	Bind(TEXT("IA_Back"), { EKeys::Escape, EKeys::RightMouseButton }, &ADBPlayerController::OnBack);
	Bind(TEXT("IA_Restart"), { EKeys::F5 }, &ADBPlayerController::OnRestart);
}

void ADBPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	HoveredBuilding = -1;
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

	Add([this] { Shot(TEXT("01_menu")); });
	Add([this] { GM()->OpenMenuPage(EDBMenuPage::Credits); });
	Add([this] { Shot(TEXT("02a_takvime_yaklasiyor")); });
	Add([this] { Shot(TEXT("02_jenerik")); });
	Add([this] { GM()->CloseMenuPage(); });
	Add([] {});
	Add([] {});
	Add([this] { Shot(TEXT("02b_menuye_dondu")); GM()->OpenMenuPage(EDBMenuPage::Settings); });
	Add([] {});
	Add([this] { Shot(TEXT("03_ayarlar")); });
	Add([this] { GM()->CloseMenuPage(); });
	Add([] {});
	Add([] {});
	Add([this] { GM()->ClickPlay(); });
	Add([this] { Shot(TEXT("03b_tahtaya_gidiyor")); });
	Add([] {});
	Add([this] { Shot(TEXT("04_A_konusuyor")); });
	Talk(3);
	Add([this] { Shot(TEXT("05_tarayici_bekliyor")); });
	Add([this] { GM()->StartScan(); });
	Add([this] { Shot(TEXT("06_taraniyor")); });
	Add([] {});
	Add([this] { Shot(TEXT("07_tarama_bitti")); });
	Talk(2);
	Add([this] { Shot(TEXT("08_B_panel")); });
	Talk(5);
	// Ayşe Teyze: ilaç + fener + düdük; önce yanlış bir eşya koyup çıkar
	Add([this] { ADBGameMode* M = GM(); M->AddItem(6); M->RemoveDraftAt(0); M->AddItem(3); M->AddItem(4); M->AddItem(5); });
	Add([this] { Shot(TEXT("09_canta_dolu")); });
	Add([this] { GM()->PackBag(); });
	Add([this] { ADBGameMode* M = GM(); M->AddItem(7); M->AddItem(0); M->AddItem(6); M->PackBag(); });
	// Öğrenci evi: bilerek ilk yardım eksik
	Add([this] { ADBGameMode* M = GM(); M->AddItem(1); M->AddItem(0); M->PackBag(); });
	Add([this] { Shot(TEXT("10_cantalar_hazir")); });
	Add([this] { GM()->ClosePanel(); });
	Add([this] { Shot(TEXT("11_C_konusuyor")); });
	Talk(5);
	Add([this] { Shot(TEXT("12_oyun")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectBag(0); M->ClickBuilding(0); });
	Add([this] { Shot(TEXT("13_ayse_tamam")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectBag(1); M->ClickBuilding(2); M->SelectTool(3); M->ClickBuilding(1); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(0); M->ClickBuilding(4); });
	Add([this] { Shot(TEXT("14_yanlis_arac")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectBag(2); M->ClickBuilding(3); });
	Add([this] { Shot(TEXT("15_eksik_canta")); });
	Add([this] { ADBGameMode* M = GM(); M->SelectTool(1); M->ClickBuilding(4); M->SelectTool(2); M->ClickBuilding(5); });
	Add([this] { GM()->ClickBuilding(6); });
	Add([this] { Shot(TEXT("16_park_notu")); });
	Add([this] { GM()->ForceQuake(); });
	Add([] {});
	Add([this] { Shot(TEXT("17_deprem")); });
	Add([] {});
	Add([] {});
	Add([this] { Shot(TEXT("18_sonuc")); });
	Add([this] { UE_LOG(LogDBInput, Log, TEXT("DB_TEST DONE")); });
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
