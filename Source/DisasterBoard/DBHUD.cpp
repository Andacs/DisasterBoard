#include "DBHUD.h"
#include "DBGameMode.h"
#include "DBPlayerController.h"
#include "DBTypes.h"
#include "DBWorld.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GlobalRenderResources.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	FLinearColor U(uint8 R, uint8 G, uint8 B, uint8 A = 255) { return FLinearColor(FColor(R, G, B, A)); }
	FLinearColor WithA(FLinearColor C, float A) { C.A *= A; return C; }

	const FLinearColor Ink = U(58, 42, 30);
	const FLinearColor Paper = U(250, 243, 228);
	const FLinearColor CardC = U(236, 216, 180);
	const FLinearColor DarkK = U(140, 100, 66);
	const FLinearColor Coral = U(226, 118, 94);
	const FLinearColor Teal = U(70, 152, 146);
	const FLinearColor Green = U(84, 168, 88);
	const FLinearColor Red = U(206, 72, 58);
	const FLinearColor NoteY = U(252, 238, 160);
	const FLinearColor ShadowC = U(60, 40, 25, 80);
	const TCHAR* Letters[] = { TEXT("A"), TEXT("B"), TEXT("C") };
}

// ---------------------------------------------------------------- yardımcılar

void ADBHUD::Box(float X, float Y, float BW, float BH, const FLinearColor& C)
{
	FCanvasTileItem T(FVector2D(X, Y), FVector2D(BW, BH), Gn(C));
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void ADBHUD::Frame(const FBox2D& R, const FLinearColor& C, float T)
{
	const FVector2D Sz = R.GetSize();
	Box(R.Min.X, R.Min.Y, Sz.X, T, C);
	Box(R.Min.X, R.Max.Y - T, Sz.X, T, C);
	Box(R.Min.X, R.Min.Y + T, T, Sz.Y - 2.f * T, C);
	Box(R.Max.X - T, R.Min.Y + T, T, Sz.Y - 2.f * T, C);
}

void ADBHUD::Card(const FBox2D& R, const FLinearColor& Fill, const FLinearColor& Edge, float Shadow)
{
	Box(R.ShiftBy(FVector2D(Shadow * 0.6f, Shadow) * S), WithA(ShadowC, Fill.A));
	Box(R, Fill);
	Frame(R, Edge, 2.5f * S);
}

void ADBHUD::Disc(const FVector2D& C, float R, const FLinearColor& Col, int32 Sides)
{
	FCanvasNGonItem N(C, FVector2D(R, R), Sides, Gn(Col));
	Canvas->DrawItem(N);
}

void ADBHUD::Tri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Col)
{
	FCanvasTriangleItem T(A, B, C, GWhiteTexture);
	T.SetColor(Gn(Col));
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void ADBHUD::Line(const FVector2D& A, const FVector2D& B, float T, const FLinearColor& C)
{
	Canvas->K2_DrawLine(A, B, T, Gn(C));
}

void ADBHUD::TickMark(const FVector2D& C, float Size, const FLinearColor& Col)
{
	const float T = FMath::Max(2.f, Size * 0.18f);
	Line(C + FVector2D(-0.42f, 0.02f) * Size, C + FVector2D(-0.12f, 0.32f) * Size, T, Col);
	Line(C + FVector2D(-0.14f, 0.32f) * Size, C + FVector2D(0.44f, -0.34f) * Size, T, Col);
}

void ADBHUD::Cross(const FVector2D& C, float Size, const FLinearColor& Col)
{
	const float T = FMath::Max(2.f, Size * 0.18f);
	Line(C + FVector2D(-0.35f, -0.35f) * Size, C + FVector2D(0.35f, 0.35f) * Size, T, Col);
	Line(C + FVector2D(-0.35f, 0.35f) * Size, C + FVector2D(0.35f, -0.35f) * Size, T, Col);
}

FVector2D ADBHUD::Measure(const FString& T, float Px) const
{
	if (!Canvas || !Font || T.IsEmpty())
	{
		return FVector2D::ZeroVector;
	}
	const float Sc = Px / FontBaseH;
	float XL = 0.f, YL = 0.f;
	Canvas->TextSize(Font, T, XL, YL, Sc, Sc);
	return FVector2D(XL, YL);
}

void ADBHUD::Txt(const FString& T, float X, float Y, float Px, const FLinearColor& C, int32 Align)
{
	if (T.IsEmpty())
	{
		return;
	}
	if (Align != 0)
	{
		const float TW = Measure(T, Px).X;
		X -= (Align == 1) ? TW * 0.5f : TW;
	}
	FCanvasTextItem It(FVector2D(X, Y), FText::FromString(T), Font, Gn(C));
	It.Scale = FVector2D(Px / FontBaseH);
	It.BlendMode = SE_BLEND_Translucent;
	if (ColorGain != 1.f)
	{
		// Sayfa dokusu ekranda ~%40'a küçülüyor: ince harfler açık kalıyordu (mürekkep 99 ölçüldü), aynı renkte kontur kalınlaştırır
		It.bOutlined = true;
		It.OutlineColor = Gn(C);
	}
	Canvas->DrawItem(It);
}

TArray<FString> ADBHUD::Wrap(const FString& T, float Px, float MaxW) const
{
	TArray<FString> Words;
	T.ParseIntoArray(Words, TEXT(" "));
	TArray<FString> Lines;
	FString Cur;
	for (const FString& Wd : Words)
	{
		const FString Try = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd;
		if (!Cur.IsEmpty() && Measure(Try, Px).X > MaxW)
		{
			Lines.Add(Cur);
			Cur = Wd;
		}
		else
		{
			Cur = Try;
		}
	}
	if (!Cur.IsEmpty())
	{
		Lines.Add(Cur);
	}
	return Lines;
}

float ADBHUD::Para(const FString& T, float X, float Y, float Px, float MaxW, const FLinearColor& C, float LineGap, int32 MaxChars, float MaxY)
{
	int32 Left = MaxChars < 0 ? MAX_int32 : MaxChars;
	for (const FString& L : Wrap(T, Px, MaxW))
	{
		if (Y + Px > MaxY || Left <= 0)
		{
			break;
		}
		Txt(Left >= L.Len() ? L : L.Left(Left), X, Y, Px, C);
		Left -= L.Len() + 1;
		Y += Px * LineGap;
	}
	return Y;
}

bool ADBHUD::Button(FName Id, int32 Index, const FBox2D& R, const FString& Label, float Px, const FLinearColor& Fill,
	const FLinearColor& Edge, const FLinearColor& TextC, bool bEnabled, bool bSelected)
{
	const bool bHov = bEnabled && Hover(R);
	const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -3.f * S)) : R;
	FLinearColor F = Fill;
	if (bHov)
	{
		F = FLinearColor::LerpUsingHSV(Fill, FLinearColor::White, 0.18f);
	}
	if (!bEnabled)
	{
		F.A *= 0.45f;
	}
	Card(RR, F, bEnabled ? Edge : WithA(Edge, 0.5f), bHov ? 7.f : 4.f);
	if (bSelected)
	{
		Frame(RR.ExpandBy(4.f * S), Teal, 4.f * S);
	}
	Txt(Label, RR.GetCenter().X, RR.GetCenter().Y - Px * 0.52f, Px, bEnabled ? TextC : WithA(TextC, 0.5f), 1);
	if (bEnabled)
	{
		Hit(Id, Index, R);
	}
	return bHov;
}

void ADBHUD::Glow(const FBox2D& R)
{
	const float A = 0.55f + 0.45f * FMath::Sin(GM->Now * 6.f);
	Frame(R.ExpandBy(7.f * S), U(255, 210, 60, uint8(255 * A)), 6.f * S);
}

void ADBHUD::Portrait(int32 Who, const FVector2D& C, float R)
{
	const FDBContent& Ct = FDBContent::Get();
	Disc(C + FVector2D(3.f, 5.f) * S, R + 4.f * S, ShadowC);
	Disc(C, R + 4.f * S, Ink);
	Disc(C, R, Ct.SpeakerColor[Who]);
	Txt(Letters[Who], C.X, C.Y - R * 0.62f, R * 1.15f, Ink, 1);
	const FString& Name = Ct.SpeakerName[Who];
	const float TW = Measure(Name, 18.f * S).X + 20.f * S;
	const FBox2D Tag = Rect(C.X - TW * 0.5f, C.Y + R - 10.f * S, TW, 28.f * S);
	Card(Tag, Paper, Ink, 3.f);
	Txt(Name, C.X, Tag.Min.Y + 4.f * S, 18.f * S, Ink, 1);
}

void ADBHUD::ItemIcon(int32 Item, const FVector2D& C, float R)
{
	const FDBItemDef& D = FDBContent::Get().Items[Item];
	if (UTexture2D* T = LoadPng(TEXT("Ikonlar/esya_") + D.Key + TEXT(".png")))
	{
		Disc(C, R + 2.f * S, Ink);
		Disc(C, R, Paper);
		DrawTex(T, FBox2D(C - FVector2D(R), C + FVector2D(R)), 0.f, FVector2D(0.5f, 0.5f));
		return;
	}
	Disc(C, R + 2.f * S, Ink);
	Disc(C, R, D.Color);
	Txt(D.Icon, C.X, C.Y - R * 0.5f, R, Ink, 1);
}

void ADBHUD::ToolIcon(int32 Tool, const FVector2D& C, float R)
{
	const FDBToolDef& D = FDBContent::Get().Tools[Tool];
	const FBox2D B(C - FVector2D(R), C + FVector2D(R));
	Box(B.ExpandBy(2.f * S), Ink);
	if (UTexture2D* T = LoadPng(TEXT("Ikonlar/onlem_") + D.Key + TEXT(".png")))
	{
		Box(B, Paper);
		DrawTex(T, B, 0.f, FVector2D(0.5f, 0.5f));
		return;
	}
	Box(B, D.Color);
	Txt(D.Icon, C.X, C.Y - R * 0.45f, R * 0.9f, Paper, 1);
}

bool ADBHUD::KitIcon(int32 Kit, const FVector2D& C, float R)
{
	const FDBContent& Ct = FDBContent::Get();
	UTexture2D* T = Ct.Kits.IsValidIndex(Kit) ? LoadPng(TEXT("Ikonlar/kit_") + Ct.Kits[Kit].Key + TEXT(".png")) : nullptr;
	if (!T)
	{
		return false;
	}
	Disc(C, R + 2.f * S, Ink);
	Disc(C, R, Paper);
	DrawTex(T, FBox2D(C - FVector2D(R), C + FVector2D(R)), 0.f, FVector2D(0.5f, 0.5f));
	return true;
}

void ADBHUD::KitBadge(int32 Kit, const FBox2D& R, float Px)
{
	const FDBKitDef& K = FDBContent::Get().Kits[Kit];
	Box(R, K.Color);
	Txt(K.Short, R.GetCenter().X, R.GetCenter().Y - Px * 0.55f, Px, Paper, 1);
}

FBox2D ADBHUD::PanelRect() const
{
	// Çekmece sağ ortada; açılınca sola kayar (çizim 5)
	const float PW = 720.f * S;
	const float X = W - 24.f * S - PW + (1.f - PanelSlide) * (PW + 30.f * S);
	return Rect(X, 20.f * S, PW, H - 216.f * S);
}

FBox2D ADBHUD::DrawerHandleRect() const
{
	const FBox2D P = PanelRect();
	return Rect(P.Min.X - 46.f * S, P.GetCenter().Y - 100.f * S, 48.f * S, 200.f * S);
}

FBox2D ADBHUD::BottomBarRect() const { return Rect(W * 0.5f - 325.f * S, H - 182.f * S, 650.f * S, 164.f * S); }
FBox2D ADBHUD::ScannerRect() const { return Rect(178.f * S, H - 200.f * S, 84.f * S, 150.f * S); }
FBox2D ADBHUD::MoneyRect() const { return Rect(W * 0.5f - 140.f * S, 16.f * S, 280.f * S, 58.f * S); }

// ---------------------------------------------------------------- giriş

const TArray<ADBHUD::FHit>* ADBHUD::ActivePageHits(FVector2D& OutPageMouse, const FVector2D& Screen) const
{
	if (!GM || !GM->World)
	{
		return nullptr;
	}
	bool bTop;
	if (GM->IsMenuReady())
	{
		bTop = true;
	}
	else if (GM->IsMenuPageReady())
	{
		bTop = false;
	}
	else
	{
		return nullptr;
	}
	FVector2D UV;
	if (!GM->World->PagePointFromScreen(PlayerOwner, Screen, bTop, UV))
	{
		return nullptr;
	}
	OutPageMouse = UV * GM->World->PageTextureSize();
	return bTop ? &TopHits : &UnderHits;
}

bool ADBHUD::IsOverButton(const FVector2D& P) const
{
	if (GM && (GM->Phase == EDBPhase::Menu || GM->Phase == EDBPhase::MenuPage))
	{
		return bPageHover;
	}
	return Hits.ContainsByPredicate([&P](const FHit& Hh) { return Hh.R.IsInside(P); });
}

bool ADBHUD::IsOverUI(const FVector2D& P) const
{
	return IsOverButton(P) || Blockers.ContainsByPredicate([&P](const FBox2D& R) { return R.IsInside(P); });
}

bool ADBHUD::PageButtonScreenPos(FName Id, bool bTop, FVector2D& OutScreen) const
{
	const TArray<FHit>& L = bTop ? TopHits : UnderHits;
	const FHit* Hh = L.FindByPredicate([Id](const FHit& X) { return X.Id == Id; });
	return Hh && GM && GM->World && GM->World->PageUVToScreen(PlayerOwner, bTop, Hh->R.GetCenter() / GM->World->PageTextureSize(), OutScreen);
}

bool ADBHUD::HandleClick(const FVector2D& P)
{
	if (!GM)
	{
		return false;
	}
	if (GM->Mini != EDBMini::None)
	{
		for (const FHit& Hh : Hits)
		{
			if (Hh.Id == TEXT("MiniCancel") && Hh.R.IsInside(P))
			{
				GM->CancelMini();
				return true;
			}
		}
		return false;
	}
	if (GM->Phase == EDBPhase::Talk)
	{
		GM->PlaySfx(EDBSfx::Click, 0.55f, 0.f, 1.15f);
		GM->AdvanceTalk();
		return true;
	}

	auto Dispatch = [this](FName Id, int32 Ix)
	{
		GM->PlaySfx(EDBSfx::Click, 0.9f, 0.f, FMath::FRandRange(0.94f, 1.06f));
		if (Id == TEXT("Play")) GM->ClickPlay();
		else if (Id == TEXT("Settings")) GM->OpenMenuPage(EDBMenuPage::Settings);
		else if (Id == TEXT("Credits")) GM->OpenMenuPage(EDBMenuPage::Credits);
		else if (Id == TEXT("Quit")) GM->QuitGame();
		else if (Id == TEXT("Back")) GM->CloseMenuPage();
		else if (Id == TEXT("Fullscreen")) GM->ToggleFullscreen();
		else if (Id == TEXT("Quality")) GM->CycleQuality();
		else if (Id == TEXT("Scanner")) GM->StartScan();
		else if (Id == TEXT("PanelClose")) GM->ClosePanel();
		else if (Id == TEXT("Drawer")) GM->TogglePanel();
		else if (Id == TEXT("Kit")) GM->SetDraftKit(Ix);
		else if (Id == TEXT("Station")) GM->StationClick(Ix / 10, Ix % 10);
		else if (Id == TEXT("OrderPrev")) GM->CycleOrder(-1);
		else if (Id == TEXT("OrderNext")) GM->CycleOrder(1);
		else if (Id == TEXT("BeltReset")) GM->ResetBag();
		else if (Id == TEXT("Item")) GM->AddItem(Ix);
		else if (Id == TEXT("Draft")) GM->RemoveDraftAt(Ix);
		else if (Id == TEXT("Pack")) GM->PackBag();
		else if (Id == TEXT("Clear")) GM->ClearDraft();
		else if (Id == TEXT("Tool")) GM->SelectTool(Ix);
		else if (Id == TEXT("Bag")) GM->SelectBag(Ix);
		else if (Id == TEXT("Again")) GM->Restart(true);
		else if (Id == TEXT("Skip")) GM->SkipComic();
		else if (Id == TEXT("NpcBag")) GM->NpcOpenFactory();
		else if (Id == TEXT("NpcClose")) GM->CloseNpc();
		else if (Id == TEXT("Music")) GM->ToggleMusic();
		else if (Id == TEXT("Menu")) GM->Restart(false);
		else if (Id == TEXT("Turn")) GM->TurnBoard(Ix);
	};

	if (GM->Phase == EDBPhase::Menu || GM->Phase == EDBPhase::MenuPage)
	{
		FVector2D PM;
		if (const TArray<FHit>* PH = ActivePageHits(PM, P))
		{
			for (int32 i = PH->Num() - 1; i >= 0; --i)
			{
				if ((*PH)[i].R.IsInside(PM))
				{
					Dispatch((*PH)[i].Id, (*PH)[i].Index);
					return true;
				}
			}
		}
		return false;
	}

	for (int32 i = Hits.Num() - 1; i >= 0; --i)
	{
		if (Hits[i].R.IsInside(P))
		{
			Dispatch(Hits[i].Id, Hits[i].Index);
			return true;
		}
	}
	if (GM->Phase == EDBPhase::Comic)
	{
		GM->PlaySfx(EDBSfx::Page, 0.8f);
		GM->NextComic();
		return true;
	}
	return IsOverUI(P);
}

// ---------------------------------------------------------------- ana çizim

void ADBHUD::DrawHUD()
{
	Super::DrawHUD();
	Hits.Reset();
	Blockers.Reset();
	GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADBGameMode>() : nullptr;
	if (!GM || !GM->World || !Canvas)
	{
		return;
	}
	W = Canvas->ClipX;
	H = Canvas->ClipY;
	S = H / 1080.f;
	float MX = 0.f, MY = 0.f;
	if (PlayerOwner && PlayerOwner->GetMousePosition(MX, MY))
	{
		Mouse = FVector2D(MX, MY);
	}
	if (!Font)
	{
		Font = GEngine->GetLargeFont();
		float XL = 0.f, YL = 0.f;
		Canvas->TextSize(Font, TEXT("Ağ"), XL, YL);
		FontBaseH = FMath::Max(1.f, YL);
	}

	FxNow = GetWorld()->GetRealTimeSeconds();
	switch (GM->Phase)
	{
	case EDBPhase::Menu:
	case EDBPhase::MenuPage:
		DrawMenuPages();
		if (DebugDot.X >= 0.f)
		{
			Disc(DebugDot, 7.f * S, Red);
		}
		DrawWind();
		DrawCursorFx();
		return;
	case EDBPhase::Comic:
		DrawComic();
		DrawCursorFx();
		return;
	case EDBPhase::ToBoard:
		DrawWind();
		DrawCursorFx();
		return;
	default:
		break;
	}

	if (GM->Phase != EDBPhase::Result)
	{
		DrawCellHints();
		DrawMarkers();
		DrawScannerAndA();
		DrawBottomBar();
		DrawMoney();
	}
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float Target = GM->bPanelOpen ? 1.f : 0.f;
	PanelSlide = FMath::FInterpTo(PanelSlide, Target, Dt, 10.f);
	if (FMath::Abs(PanelSlide - Target) < 0.002f)
	{
		PanelSlide = Target;
	}
	if (PanelSlide > 0.001f)
	{
		DrawPanel();
	}
	if ((GM->bPanelOpen || GM->bPlayStarted || PanelSlide > 0.001f) && GM->Phase != EDBPhase::Result)
	{
		DrawDrawerHandle();
	}
	if (GM->Phase == EDBPhase::Play)
	{
		DrawNpc();
	}
	if (GM->Phase != EDBPhase::Talk && GM->Phase != EDBPhase::Result && GM->NpcBuilding < 0 && GM->Mini == EDBMini::None)
	{
		DrawTooltip();
		DrawSelectionCursor();
	}
	if (GM->Phase != EDBPhase::Result)
	{
		DrawToast();
	}
	// Rehberler: Mert hep sol altta; Asil ve Andaç konuşurken gelir. Eliyle gösterir.
	if (GM->Phase != EDBPhase::Result)
	{
		DrawGuide(0, FVector2D(100.f * S, H - 44.f * S), 170.f * S);
		if (const FDBLine* L = GM->CurrentLine())
		{
			if (L->Who == EDBSpeaker::B)
			{
				DrawGuide(1, FVector2D(PanelRect().Min.X - 110.f * S, H - 228.f * S), 180.f * S);
			}
			else if (L->Who == EDBSpeaker::C)
			{
				DrawGuide(2, FVector2D(W * 0.5f - 520.f * S, H - 196.f * S), 170.f * S);
			}
		}
	}
	DrawTalk();
	if (GM->Mini != EDBMini::None)
	{
		DrawMini();
	}
	if (GM->Phase == EDBPhase::Result)
	{
		DrawResult();
	}
	DrawCursorFx();
}

// ---------------------------------------------------------------- takvim sayfaları (dokuya çizilir)

void ADBHUD::DrawMenuPages()
{
	ADBWorld* Wd = GM->World;
	FVector2D TopMouse(-1.f, -1.f), UnderMouse(-1.f, -1.f), UV;
	if (GM->IsMenuReady() && Wd->PagePointFromScreen(PlayerOwner, Mouse, true, UV))
	{
		TopMouse = UV * Wd->PageTextureSize();
	}
	if (GM->IsMenuPageReady() && Wd->PagePointFromScreen(PlayerOwner, Mouse, false, UV))
	{
		UnderMouse = UV * Wd->PageTextureSize();
	}
	RenderPage(Wd->TopRT, true, TopMouse, TopHits);
	RenderPage(Wd->UnderRT, false, UnderMouse, UnderHits);
	bPageHover = TopHits.ContainsByPredicate([&](const FHit& Hh) { return Hh.R.IsInside(TopMouse); })
		|| UnderHits.ContainsByPredicate([&](const FHit& Hh) { return Hh.R.IsInside(UnderMouse); });
}

void ADBHUD::RenderPage(UTextureRenderTarget2D* RT, bool bTop, const FVector2D& PageMouse, TArray<FHit>& OutHits)
{
	if (!RT)
	{
		return;
	}
	UCanvas* PageCanvas = nullptr;
	FVector2D Size;
	FDrawToRenderTargetContext Ctx;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RT, PageCanvas, Size, Ctx);
	if (!PageCanvas)
	{
		return;
	}
	// Ekran çizimini sakla, sayfaya çiz, geri yükle
	UCanvas* const ScreenCanvas = Canvas;
	const float SW = W, SH = H, SS = S;
	const FVector2D SMouse = Mouse;
	TArray<FHit> ScreenHits = MoveTemp(Hits);
	Hits.Reset();

	Canvas = PageCanvas;
	W = Size.X;
	H = Size.Y;
	S = 2.2f;
	Mouse = PageMouse;
	ColorGain = GM->World->PageGain;
	if (bTop)
	{
		DrawMainPage(GM->IsMenuReady());
	}
	else
	{
		DrawSubPage(GM->IsMenuPageReady());
	}
	OutHits = MoveTemp(Hits);

	ColorGain = 1.f;
	Canvas = ScreenCanvas;
	W = SW;
	H = SH;
	S = SS;
	Mouse = SMouse;
	Hits = MoveTemp(ScreenHits);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Ctx);
}

void ADBHUD::DrawMainPage(bool bEnabled)
{
	const float U1 = H / 100.f;
	const float CX = W * 0.5f;
	const float BandH = H * 0.18f;
	Box(0.f, 0.f, W, H, U(248, 240, 224));
	Box(0.f, 0.f, W, BandH, U(232, 128, 104));
	Txt(TEXT("EKİM 2026"), CX, BandH * 0.5f - 4.5f * U1, 9.f * U1, Paper, 1);
	if (UTexture2D* Logo = LoadPng(TEXT("Arayuz/logo.png")))
	{
		DrawTex(Logo, Rect(CX - 30.f * U1, BandH + 1.5f * U1, 60.f * U1, 15.f * U1), 0.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Txt(TEXT("DisasterBoard"), CX, BandH + 4.f * U1, 12.f * U1, Ink, 1);
	}
	Txt(TEXT("Afetlere hazır mısın?"), CX, BandH + 18.f * U1, 6.f * U1, DarkK, 1);
	const TCHAR* Labels[] = { TEXT("Oyna"), TEXT("Ayarlar"), TEXT("Emeği Geçenler"), TEXT("Çıkış") };
	const FName Ids[] = { TEXT("Play"), TEXT("Settings"), TEXT("Credits"), TEXT("Quit") };
	for (int32 i = 0; i < 4; ++i)
	{
		const FBox2D R = Rect(W * (i % 2 == 0 ? 0.08f : 0.52f), BandH + (32.f + (i / 2) * 22.f) * U1, W * 0.40f, 17.f * U1);
		Button(Ids[i], -1, R, Labels[i], 7.5f * U1, i == 0 ? Coral : CardC, DarkK, i == 0 ? Paper : Ink, bEnabled);
	}
}

void ADBHUD::DrawSubPage(bool bEnabled)
{
	const float U1 = H / 100.f;
	const float CX = W * 0.5f;
	const float BandH = H * 0.18f;
	const bool bSettings = GM->MenuPage != EDBMenuPage::Credits;
	Box(0.f, 0.f, W, H, U(248, 240, 224));
	Box(0.f, 0.f, W, BandH, U(96, 170, 164));
	Txt(bSettings ? TEXT("AYARLAR") : TEXT("EMEĞİ GEÇENLER"), CX, BandH * 0.5f - 4.5f * U1, 9.f * U1, Paper, 1);
	float Y = BandH + 3.f * U1;
	if (bSettings)
	{
		Button(TEXT("Fullscreen"), -1, Rect(W * 0.15f, Y, W * 0.7f, 14.f * U1),
			FString::Printf(TEXT("Ekran: %s"), *GM->FullscreenLabel()), 6.5f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 18.f * U1;
		Button(TEXT("Quality"), -1, Rect(W * 0.15f, Y, W * 0.7f, 14.f * U1),
			FString::Printf(TEXT("Grafik: %s"), *GM->QualityLabel()), 6.5f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 18.f * U1;
		Button(TEXT("Music"), -1, Rect(W * 0.15f, Y, W * 0.7f, 14.f * U1),
			FString::Printf(TEXT("Müzik: %s"), *GM->MusicLabel()), 6.5f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 17.f * U1;
		Txt(TEXT("Esc / sağ tık: geri  ·  F5: baştan  ·  M: müzik"), CX, Y, 4.4f * U1, DarkK, 1);
	}
	else
	{
		const TArray<FString>& Lines = FDBContent::Get().Credits;
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			Txt(Lines[i], CX, Y, (i == 0 ? 6.5f : 4.9f) * U1, i == 0 ? Coral : Ink, 1);
			Y += (Lines[i].IsEmpty() ? 1.2f : (i == 0 ? 7.5f : 5.9f)) * U1;
		}
	}
	Button(TEXT("Back"), -1, Rect(W * 0.35f, H - 15.f * U1, W * 0.3f, 11.f * U1), TEXT("Geri"), 6.f * U1, Coral, DarkK, Paper, bEnabled);
}

// ---------------------------------------------------------------- tahta üstü

void ADBHUD::DrawMarkers()
{
	const FDBContent& C = FDBContent::Get();
	const ADBPlayerController* PC = Cast<ADBPlayerController>(PlayerOwner);
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (!GM->Revealed[p])
		{
			continue;
		}
		const int32 B = C.Problems[p].Building;
		FVector2D Sp;
		if (!PlayerOwner->ProjectWorldLocationToScreen(GM->World->MarkerAnchor(B), Sp))
		{
			continue;
		}
		const bool bSolved = GM->Solved[p];
		const float Age = GM->Now - (bSolved ? GM->SolveTime[p] : GM->RevealTime[p]);
		const float Pop = Age < 0.35f ? FMath::Sin(FMath::Clamp(Age / 0.35f, 0.f, 1.f) * PI * 0.75f) * 1.22f : 1.f;
		const bool bHov = PC && PC->HoveredBuilding == B;
		const bool bCur = p == GM->CurrentTask() && GM->Phase == EDBPhase::Play;
		float R = 21.f * S * FMath::Max(0.2f, Pop) * (bHov ? 1.18f : 1.f) * (bCur ? 1.25f : 1.f);
		if (bCur)
		{
			Disc(Sp, R + 9.f * S + 3.f * S * FMath::Sin(GM->Now * 5.f), U(255, 210, 60, 150));
		}
		if (!bSolved)
		{
			R *= 1.f + 0.06f * FMath::Sin(GM->Now * 6.f + p);
		}
		if (UTexture2D* Mark = LoadPng(bSolved ? TEXT("Ikonlar/isaret_tamam.png") : TEXT("Ikonlar/isaret_uyari.png")))
		{
			// Tasarımcının işareti (96×96): alt ucu sivri, binayı gösterir
			DrawTex(Mark, Rect(Sp.X - R * 1.25f, Sp.Y - R * 1.0f, R * 2.5f, R * 2.5f), 0.f, FVector2D(0.5f, 0.5f));
		}
		else
		{
			Tri(Sp + FVector2D(-R * 0.5f, R * 0.6f), Sp + FVector2D(R * 0.5f, R * 0.6f), Sp + FVector2D(0.f, R * 1.5f), Ink);
			Disc(Sp, R + 3.f * S, Ink);
			Disc(Sp, R, bSolved ? U(232, 246, 226) : Paper);
			if (bSolved)
			{
				TickMark(Sp, R * 1.15f, Green);
			}
			else
			{
				Tri(Sp + FVector2D(0.f, -R * 0.66f), Sp + FVector2D(-R * 0.68f, R * 0.48f), Sp + FVector2D(R * 0.68f, R * 0.48f), Red);
				Txt(TEXT("!"), Sp.X, Sp.Y - R * 0.34f, R * 0.78f, Paper, 1);
			}
		}
		if (!bSolved)
		{
			// İstek açıkça: hangi çanta / hangi önlem (ekip: "istekler net belirtilecek")
			const FDBProblemDef& Pr = C.Problems[p];
			const bool bKit = Pr.Kit >= 0;
			const FString Want = bKit ? C.Kits[Pr.Kit].Name : C.Tools[Pr.Tool].Name;
			const FLinearColor WantC = bKit ? C.Kits[Pr.Kit].Color : C.Tools[Pr.Tool].Color;
			const float Px = 16.f * S;
			const float LW = Measure(Want, Px).X + 18.f * S;
			const FBox2D L = Rect(Sp.X + R + 6.f * S, Sp.Y - 12.f * S, LW, 24.f * S);
			Box(L.ShiftBy(FVector2D(2.f, 3.f) * S), ShadowC);
			Box(L, WantC);
			Frame(L, Ink, 1.5f * S);
			Txt(Want, L.Min.X + 9.f * S, L.Min.Y + 3.f * S, Px, Paper);
		}
	}
}

void ADBHUD::DrawMoney()
{
	if (!GM->bPanelOpen && !GM->bPlayStarted)
	{
		return;
	}
	const FBox2D R = MoneyRect();
	TgMoney = R;
	if (GM->IsTalking(FDBContent::Get().PanelIntroC, 2))
	{
		Glow(R);
	}
	Card(R, Paper, DarkK);
	Blockers.Add(R);
	const FVector2D Coin(R.Min.X + 32.f * S, R.GetCenter().Y);
	if (UTexture2D* T = LoadPng(TEXT("Ikonlar/butce.png")))
	{
		DrawTex(T, Rect(Coin.X - 20.f * S, Coin.Y - 20.f * S, 40.f * S, 40.f * S), 0.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Disc(Coin, 17.f * S, U(214, 160, 56));
		Disc(Coin, 12.f * S, U(240, 196, 86));
		Txt(TEXT("TL"), Coin.X, Coin.Y - 7.f * S, 13.f * S, U(120, 84, 30), 1);
	}
	Txt(TEXT("Bütçe"), R.Min.X + 60.f * S, R.Min.Y + 15.f * S, 22.f * S, DarkK);
	Txt(FString::Printf(TEXT("%d TL"), GM->Money), R.Max.X - 18.f * S, R.Min.Y + 9.f * S, 34.f * S, Ink, 2);
}

void ADBHUD::DrawScannerAndA()
{
	const FDBContent& C = FDBContent::Get();
	Blockers.Add(Rect(0.f, H - 230.f * S, 290.f * S, 230.f * S));
	const FBox2D R = ScannerRect();
	TgScanner = R;
	const bool bCan = GM->Phase == EDBPhase::WaitScan;
	if (bCan || GM->IsTalking(C.IntroA, 2))
	{
		Glow(R);
	}
	const bool bHov = bCan && Hover(R);
	const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -4.f * S)) : R;
	UTexture2D* ScannerTex = LoadPng(TEXT("Arayuz/tarayici.png"));
	if (ScannerTex)
	{
		DrawTex(ScannerTex, RR, 0.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Line(FVector2D(RR.Min.X + 22.f * S, RR.Min.Y), FVector2D(RR.Min.X + 12.f * S, RR.Min.Y - 34.f * S), 3.f * S, Ink);
		Line(FVector2D(RR.Max.X - 24.f * S, RR.Min.Y), FVector2D(RR.Max.X - 16.f * S, RR.Min.Y - 46.f * S), 3.f * S, Ink);
		Disc(FVector2D(RR.Min.X + 12.f * S, RR.Min.Y - 34.f * S), 5.f * S, Coral);
		Disc(FVector2D(RR.Max.X - 16.f * S, RR.Min.Y - 46.f * S), 5.f * S, Coral);
		Card(RR, U(92, 116, 124), Ink, 5.f);
		const FBox2D Screen = Rect(RR.Min.X + 10.f * S, RR.Min.Y + 12.f * S, RR.GetSize().X - 20.f * S, 62.f * S);
		Box(Screen, U(196, 232, 206));
		const FVector2D Dial(Screen.GetCenter().X, Screen.Max.Y - 8.f * S);
		Disc(Dial, 5.f * S, Ink);
		const bool bScanning = GM->Phase == EDBPhase::Scanning;
		const float Ang = bScanning ? FMath::Sin(GM->Now * 9.f) * 1.2f : -1.1f;
		Line(Dial, Dial + FVector2D(FMath::Sin(Ang), -FMath::Cos(Ang)) * 40.f * S, 3.f * S, Red);
		for (int32 i = 0; i < 3; ++i)
		{
			Box(Rect(RR.Min.X + 14.f * S + i * 20.f * S, RR.Min.Y + 88.f * S, 14.f * S, 14.f * S), i == 1 ? Coral : U(236, 188, 86));
		}
		Txt(TEXT("Tarayıcı"), RR.GetCenter().X, RR.Max.Y - 34.f * S, 17.f * S, Paper, 1);
	}
	if (bCan)
	{
		Hit(TEXT("Scanner"), -1, R);
		const float Bob = FMath::Sin(GM->Now * 5.f) * 6.f * S;
		const FVector2D Tip(R.GetCenter().X, R.Min.Y - 58.f * S + Bob);
		Tri(Tip, Tip + FVector2D(-16.f, -22.f) * S, Tip + FVector2D(16.f, -22.f) * S, U(255, 210, 60));
		Txt(TEXT("Tıkla!"), Tip.X, Tip.Y - 52.f * S, 24.f * S, U(255, 230, 140), 1);
	}
}

void ADBHUD::DrawBottomBar()
{
	const FDBContent& C = FDBContent::Get();
	const FBox2D BR = BottomBarRect();
	Card(BR, CardC, DarkK, 8.f);
	Blockers.Add(BR);
	const bool bPlay = GM->Phase == EDBPhase::Play;

	// Şehir önlemleri (bedeli bütçeden düşer)
	const float X = BR.Min.X + 20.f * S;
	const float Y = BR.Min.Y + 40.f * S;
	Txt(TEXT("Şehir önlemleri"), X, BR.Min.Y + 10.f * S, 19.f * S, DarkK);
	TgTools = Rect(X, Y, 5.f * 112.f * S + 4.f * 10.f * S, 112.f * S);
	if (GM->IsTalking(C.PanelIntroC, 1))
	{
		Glow(TgTools);
	}
	for (int32 t = 0; t < C.Tools.Num() && t < 5; ++t)
	{
		const FDBToolDef& T = C.Tools[t];
		const FBox2D R = Rect(X + t * 122.f * S, Y, 112.f * S, 112.f * S);
		const bool bSel = GM->SelTool == t;
		const bool bHov = bPlay && Hover(R);
		const FBox2D RR = (bSel || bHov) ? R.ShiftBy(FVector2D(0.f, -5.f * S)) : R;
		Card(RR, bSel ? U(255, 248, 220) : Paper, bSel ? Teal : DarkK, 4.f);
		if (bSel)
		{
			Frame(RR.ExpandBy(3.f * S), Teal, 4.f * S);
		}
		ToolIcon(t, FVector2D(RR.GetCenter().X, RR.Min.Y + 28.f * S), 20.f * S);
		float LY = RR.Min.Y + 52.f * S;
		for (const FString& L : Wrap(T.Name, 15.f * S, 102.f * S))
		{
			Txt(L, RR.GetCenter().X, LY, 15.f * S, Ink, 1);
			LY += 16.f * S;
		}
		const bool bAfford = GM->Money >= T.Cost;
		Txt(FString::Printf(TEXT("%d TL"), T.Cost), RR.GetCenter().X, RR.Max.Y - 22.f * S, 16.f * S, bAfford ? U(150, 110, 30) : Red, 1);
		if (bPlay)
		{
			Hit(TEXT("Tool"), t, RR);
		}
		else
		{
			Box(RR, U(236, 216, 180, 110));
		}
	}

	// Tahtayı döndür (Andaç: sağ ve sol ok; basınca tahta kalkar, döner, yerine iner)
	const bool bCanTurn = GM->Mini == EDBMini::None && (bPlay || GM->Phase == EDBPhase::BagPrep || GM->Phase == EDBPhase::WaitScan);
	for (int32 Dir : { -1, 1 })
	{
		const float Sz = 76.f * S;
		const FBox2D R = Rect(Dir < 0 ? BR.Min.X - Sz - 14.f * S : BR.Max.X + 14.f * S, BR.GetCenter().Y - Sz * 0.5f, Sz, Sz);
		const bool bHov = bCanTurn && Hover(R);
		const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -4.f * S)) : R;
		Card(RR, bHov ? U(255, 248, 220) : Paper, DarkK, 4.f);
		TurnArrow(RR.GetCenter() + FVector2D(0.f, 4.f * S), 22.f * S, Dir, bCanTurn ? Ink : U(170, 160, 150));
		Txt(Dir < 0 ? TEXT("Q") : TEXT("E"), Dir < 0 ? RR.Min.X + 7.f * S : RR.Max.X - 7.f * S, RR.Min.Y + 4.f * S, 17.f * S, DarkK, Dir < 0 ? 0 : 2);
		Blockers.Add(R);
		if (bCanTurn)
		{
			Hit(TEXT("Turn"), Dir, RR);
		}
	}
}

void ADBHUD::DrawDrawerHandle()
{
	const FBox2D R = DrawerHandleRect();
	TgDrawer = R;
	const bool bCan = GM->Phase == EDBPhase::Play || GM->Phase == EDBPhase::BagPrep;
	if (GM->IsTalking(FDBContent::Get().PanelIntroC, 4))
	{
		Glow(R);
	}
	const bool bHov = bCan && Hover(R);
	const FBox2D RR = bHov ? R.ShiftBy(FVector2D(-5.f * S, 0.f)) : R;
	Card(RR, U(206, 160, 112), Ink, 5.f);
	Box(Rect(RR.Min.X + 10.f * S, RR.Min.Y + 14.f * S, 6.f * S, RR.GetSize().Y - 28.f * S), U(120, 84, 56));
	const TCHAR* Word = TEXT("HAZIRLIK");
	const float LetterPx = 17.f * S;
	float LY = RR.GetCenter().Y - 8.f * LetterPx * 0.55f;
	for (int32 i = 0; Word[i]; ++i)
	{
		Txt(FString::Chr(Word[i]), RR.GetCenter().X + 5.f * S, LY, LetterPx, Ink, 1);
		LY += LetterPx * 1.1f;
	}
	Blockers.Add(R);
	if (bCan)
	{
		Hit(TEXT("Drawer"), -1, R);
	}
}

void ADBHUD::DrawBagIcon(const FVector2D& C, float Sc, int32 Kit, const TArray<int32>& Personal)
{
	// Sırt çantası: kit seçilince kit renginde; içindekiler küçük daireler (üstte temel kit, altta kişiye özel)
	const FDBContent& Ct = FDBContent::Get();
	const FLinearColor Body = Kit >= 0 ? Ct.Kits[Kit].Color : U(196, 154, 108);
	if (UTexture2D* T = LoadPng(TEXT("Arayuz/canta.png")))
	{
		DrawTex(T, Rect(C.X - 47.f * Sc, C.Y - 54.f * Sc, 94.f * Sc, 98.f * Sc), 0.f, FVector2D(0.5f, 0.5f), Kit >= 0 ? Ct.Kits[Kit].Color : FLinearColor::White);
	}
	else
	{
		Frame(Rect(C.X - 18.f * Sc, C.Y - 54.f * Sc, 36.f * Sc, 16.f * Sc), Ink, 4.f * Sc);
		Card(Rect(C.X - 42.f * Sc, C.Y - 42.f * Sc, 84.f * Sc, 86.f * Sc), Body, Ink, 4.f);
		Box(Rect(C.X - 30.f * Sc, C.Y + 2.f * Sc, 60.f * Sc, 34.f * Sc), U(255, 255, 255, 70));
	}
	if (Kit >= 0)
	{
		const TArray<int32>& D = Ct.Kits[Kit].Defaults;
		for (int32 i = 0; i < D.Num(); ++i)
		{
			ItemIcon(D[i], FVector2D(C.X + (i - (D.Num() - 1) * 0.5f) * 24.f * Sc, C.Y - 20.f * Sc), 10.f * Sc);
		}
	}
	for (int32 i = 0; i < Personal.Num(); ++i)
	{
		ItemIcon(Personal[i], FVector2D(C.X + (i - (Personal.Num() - 1) * 0.5f) * 24.f * Sc, C.Y + 19.f * Sc), 10.f * Sc);
	}
}

void ADBHUD::DrawPanel()
{
	const FDBContent& C = FDBContent::Get();
	const FBox2D P = PanelRect();
	Card(P, U(232, 226, 236), DarkK, 10.f);
	Blockers.Add(P);
	const bool bEdit = GM->CanEditBags() && PanelSlide > 0.97f;
	const float X0 = P.Min.X + 24.f * S;
	const float Y0 = P.Min.Y;

	Txt(TEXT("Çanta Fabrikası"), X0, Y0 + 14.f * S, 32.f * S, Ink);
	const FBox2D XR = Rect(P.Max.X - 66.f * S, Y0 + 12.f * S, 48.f * S, 48.f * S);
	TgClose = XR;
	if (GM->IsTalking(C.BagIntroB, 5))
	{
		Glow(XR);
	}
	Button(TEXT("PanelClose"), -1, XR, TEXT("X"), 26.f * S, Coral, Ink, Paper, bEdit);

	// Bant: sağda, yukarıdan aşağı (Purble Place'teki gibi ama dikey ve durmaz)
	const float BeltL = P.Max.X - 154.f * S;
	const float BeltW = 110.f * S;
	const float BeltTop = Y0 + 236.f * S;
	const float BeltBot = P.Max.Y - 96.f * S;
	const float BeltLen = BeltBot - BeltTop;
	const float BeltCX = BeltL + BeltW * 0.5f;
	Box(Rect(BeltL - 8.f * S, BeltTop - 20.f * S, BeltW + 16.f * S, BeltLen + 40.f * S), U(120, 124, 132));
	Box(Rect(BeltL, BeltTop - 12.f * S, BeltW, BeltLen + 24.f * S), U(70, 72, 80));
	const float Slat = 28.f * S;
	const float Run = GM->IsBeltRunning() ? FMath::Fmod(GM->Now * BeltLen / FMath::Max(1.f, GM->BeltSeconds), Slat) : 0.f;
	for (float Yy = BeltTop - 12.f * S + Run; Yy < BeltBot + 12.f * S; Yy += Slat)
	{
		Box(Rect(BeltL + 4.f * S, Yy, BeltW - 8.f * S, 3.f * S), U(96, 98, 108));
	}
	Disc(FVector2D(BeltCX, BeltTop - 14.f * S), 9.f * S, U(170, 172, 180));
	Disc(FVector2D(BeltCX, BeltBot + 14.f * S), 9.f * S, U(170, 172, 180));

	// Sipariş kartı: hazırlanacak çantanın resmi ve tarifi (en üstte)
	const int32 Ord = GM->OrderProblem;
	const FBox2D Card0 = Rect(X0, Y0 + 62.f * S, 450.f * S, 156.f * S);
	TgOrder = Card0;
	if (GM->IsTalking(C.BagIntroB, 1))
	{
		Glow(Card0);
	}
	Box(Card0.ShiftBy(FVector2D(4.f, 6.f) * S), ShadowC);
	Box(Card0, NoteY);
	Frame(Card0, Ink, 2.f * S);
	const TArray<int32> Pending = GM->PendingOrders();
	if (Ord >= 0)
	{
		const FDBProblemDef& Pr = C.Problems[Ord];
		const int32 Num = Pending.IndexOfByKey(Ord) + 1;
		Txt(FString::Printf(TEXT("SİPARİŞ %d / %d"), FMath::Max(1, Num), Pending.Num()), Card0.Min.X + 14.f * S, Card0.Min.Y + 10.f * S, 15.f * S, DarkK);
		Txt(C.Buildings[Pr.Building].Name, Card0.Min.X + 14.f * S, Card0.Min.Y + 30.f * S, 24.f * S, Ink);
		// 1. temel kit
		Txt(TEXT("1. Temel kit:"), Card0.Min.X + 14.f * S, Card0.Min.Y + 68.f * S, 17.f * S, Ink);
		const FDBKitDef& K = C.Kits[Pr.Kit];
		const float KW = Measure(K.Name, 16.f * S).X + 16.f * S;
		Box(Rect(Card0.Min.X + 124.f * S, Card0.Min.Y + 66.f * S, KW, 24.f * S), K.Color);
		Txt(K.Name, Card0.Min.X + 132.f * S, Card0.Min.Y + 69.f * S, 16.f * S, Paper);
		KitIcon(Pr.Kit, FVector2D(Card0.Min.X + 142.f * S + KW, Card0.Min.Y + 78.f * S), 14.f * S);
		// 2. kişiye özel
		Txt(TEXT("2. Kişiye özel:"), Card0.Min.X + 14.f * S, Card0.Min.Y + 104.f * S, 17.f * S, Ink);
		float IX = Card0.Min.X + 140.f * S;
		for (int32 It : Pr.Personal)
		{
			ItemIcon(It, FVector2D(IX + 12.f * S, Card0.Min.Y + 114.f * S), 11.f * S);
			Txt(C.Items[It].Name, IX + 28.f * S, Card0.Min.Y + 105.f * S, 15.f * S, Ink);
			IX += Measure(C.Items[It].Name, 15.f * S).X + 46.f * S;
		}
		// Çantanın resmi: olması gereken hâli
		DrawBagIcon(FVector2D(Card0.Max.X - 54.f * S, Card0.Min.Y + 62.f * S), 0.62f * S, Pr.Kit, Pr.Personal);
		Button(TEXT("OrderPrev"), -1, Rect(Card0.Max.X + 10.f * S, Card0.Min.Y + 20.f * S, 44.f * S, 44.f * S), TEXT("<"), 22.f * S, CardC, DarkK, Ink, bEdit && Pending.Num() > 1);
		Button(TEXT("OrderNext"), -1, Rect(Card0.Max.X + 10.f * S, Card0.Min.Y + 76.f * S, 44.f * S, 44.f * S), TEXT(">"), 22.f * S, CardC, DarkK, Ink, bEdit && Pending.Num() > 1);
	}
	else
	{
		Txt(TEXT("Bütün çanta siparişleri tamam!"), Card0.Min.X + 14.f * S, Card0.Min.Y + 60.f * S, 22.f * S, Green);
	}

	// İstasyonlar: 0 temel kit, 1..3 kişiye özel (bant önlerinden geçerken tıkla)
	TgStations = FBox2D(ForceInit);
	for (int32 K = 0; K < ADBGameMode::NumStations && K <= C.Stations.Num(); ++K)
	{
		const float Yc = BeltTop + ADBGameMode::StationCenter(K) * BeltLen;
		const FBox2D Row = Rect(X0, Yc - 50.f * S, 500.f * S, 100.f * S);
		const bool bActive = GM->IsStationActive(K);
		if (K == 0) TgKitStation = Row; else TgStations += Row;
		Box(Row, bActive ? U(255, 246, 214) : U(214, 208, 220));
		Frame(Row, bActive ? U(236, 170, 40) : DarkK, (bActive ? 4.f : 2.f) * S);
		// Boru: istasyondan banda
		Box(Rect(Row.Max.X, Yc - 6.f * S, BeltL - Row.Max.X, 12.f * S), bActive ? U(236, 170, 40) : U(150, 150, 160));
		Txt(K == 0 ? TEXT("TEMEL KİT") : C.Stations[K - 1].Name, Row.Min.X + 10.f * S, Row.Min.Y + 6.f * S, 14.f * S, DarkK);
		for (int32 B = 0; B < 3; ++B)
		{
			const FBox2D Bt = Rect(Row.Min.X + 10.f * S + B * 162.f * S, Row.Min.Y + 26.f * S, 152.f * S, 66.f * S);
			const bool bHov = bActive && Hover(Bt);
			const FBox2D BR = bHov ? Bt.ShiftBy(FVector2D(0.f, -3.f * S)) : Bt;
			if (K == 0)
			{
				const FDBKitDef& Kd = C.Kits[B];
				const bool bSel = GM->bBagOnBelt && GM->BagKit == B;
				Card(BR, Kd.Color, bSel ? Ink : DarkK, bHov ? 6.f : 3.f);
				if (bSel) Frame(BR.ExpandBy(3.f * S), Ink, 3.f * S);
				const bool bKitPng = KitIcon(B, FVector2D(BR.Min.X + 20.f * S, BR.Min.Y + 20.f * S), 15.f * S);
				Txt(Kd.Short, BR.GetCenter().X + (bKitPng ? 12.f * S : 0.f), BR.Min.Y + 8.f * S, 19.f * S, Paper, 1);
				for (int32 i = 0; i < Kd.Defaults.Num(); ++i)
				{
					ItemIcon(Kd.Defaults[i], FVector2D(BR.GetCenter().X + (i - 1) * 26.f * S, BR.Min.Y + 46.f * S), 10.f * S);
				}
			}
			else
			{
				const int32 It = C.Stations[K - 1].Items[B];
				const bool bIn = GM->bBagOnBelt && GM->BagPersonal.Contains(It);
				Card(BR, bIn ? U(226, 240, 214) : Paper, bIn ? Green : DarkK, bHov ? 6.f : 3.f);
				ItemIcon(It, FVector2D(BR.Min.X + 30.f * S, BR.GetCenter().Y), 22.f * S);
				Para(C.Items[It].Name, BR.Min.X + 60.f * S, BR.Min.Y + 12.f * S, 16.f * S, 88.f * S, Ink, 1.1f);
			}
			if (!bActive)
			{
				Box(BR, U(214, 208, 220, 110));
			}
			else if (bEdit)
			{
				Hit(TEXT("Station"), K * 10 + B, BR);
			}
			if (GM->bFactoryTutorial && Ord >= 0)
			{
				const FDBProblemDef& TP = C.Problems[Ord];
				const bool bRight = (K == 0) ? (B == TP.Kit) : TP.Personal.Contains(C.Stations[K - 1].Items[B]);
				const bool bDone = (K == 0) ? (GM->BagKit == TP.Kit) : GM->BagPersonal.Contains(C.Stations[K - 1].Items[B]);
				if (bRight && !(GM->bBagOnBelt && bDone))
				{
					Glow(BR);
				}
			}
		}
	}
	if (GM->IsTalking(C.BagIntroB, 2)) Glow(TgKitStation);
	if (GM->IsTalking(C.BagIntroB, 3) && TgStations.bIsValid) Glow(TgStations);

	// Teslim: bandın dibi
	const FBox2D Del = Rect(BeltL - 14.f * S, BeltBot + 26.f * S, BeltW + 28.f * S, 52.f * S);
	TgDeliver = Del;
	if (GM->IsTalking(C.BagIntroB, 4)) Glow(Del);
	Card(Del, U(244, 230, 204), Ink, 4.f);
	Txt(TEXT("TESLİM"), Del.GetCenter().X, Del.Min.Y + 14.f * S, 20.f * S, Ink, 1);
	Button(TEXT("BeltReset"), -1, Rect(X0, P.Max.Y - 64.f * S, 160.f * S, 44.f * S), TEXT("Baştan"), 19.f * S, CardC, DarkK, Ink, bEdit && GM->bBagOnBelt);
	Txt(TEXT("Bant durmaz: çanta önünden geçerken tıkla."), X0 + 176.f * S, P.Max.Y - 52.f * S, 16.f * S, DarkK);

	// Çanta bantta
	if (GM->bBagOnBelt)
	{
		DrawBagIcon(FVector2D(BeltCX, BeltTop + GM->BagT * BeltLen), S, GM->BagKit, GM->BagPersonal);
	}
	// Teslim sonucu: kısa yeşil tik / kırmızı çarpı
	const float Age = GM->Now - GM->DeliverTime;
	if (Age >= 0.f && Age < 1.3f)
	{
		const FVector2D Cc = Del.GetCenter() - FVector2D(0.f, 70.f * S);
		const float A = FMath::Clamp(1.3f - Age, 0.f, 1.f);
		Disc(Cc, 34.f * S, GM->bDeliverOk ? U(226, 244, 220) : U(250, 222, 212));
		if (GM->bDeliverOk) TickMark(Cc, 44.f * S * A + 10.f * S, Green); else Cross(Cc, 40.f * S * A + 10.f * S, Red);
	}
}

// ---------------------------------------------------------------- konuşma, bildirim, ipucu

void ADBHUD::DrawTalk()
{
	const FDBLine* L = GM->CurrentLine();
	if (!L)
	{
		return;
	}
	const FDBContent& C = FDBContent::Get();
	const int32 Who = (int32)L->Who;
	FBox2D BR;
	FVector2D TailA, TailB, TailTip;
	if (Who == 0)
	{
		BR = Rect(40.f * S, H - 470.f * S, 620.f * S, 160.f * S);
		TailA = FVector2D(120.f * S, BR.Max.Y - 2.f);
		TailB = FVector2D(180.f * S, BR.Max.Y - 2.f);
		TailTip = FVector2D(116.f * S, H - 232.f * S);
	}
	else if (Who == 1)
	{
		const FBox2D P = PanelRect();
		const FVector2D PC(P.Min.X - 96.f * S, H - 330.f * S);
		BR = Rect(PC.X - 60.f * S - 600.f * S, H - 600.f * S, 600.f * S, 160.f * S);
		TailA = FVector2D(BR.Max.X - 120.f * S, BR.Max.Y - 2.f);
		TailB = FVector2D(BR.Max.X - 60.f * S, BR.Max.Y - 2.f);
		TailTip = FVector2D(PanelRect().Min.X - 120.f * S, H - 420.f * S);
	}
	else
	{
		const FVector2D PC(W * 0.5f - 340.f * S, H - 276.f * S);
		BR = Rect(W * 0.5f - 250.f * S, H - 470.f * S, 620.f * S, 160.f * S);
		TailA = FVector2D(BR.Min.X + 2.f, BR.Max.Y - 90.f * S);
		TailB = FVector2D(BR.Min.X + 2.f, BR.Max.Y - 40.f * S);
		TailTip = FVector2D(W * 0.5f - 310.f * S, H - 330.f * S);
	}
	Card(BR, Paper, Ink, 8.f);
	Tri(TailA, TailB, TailTip, Paper);
	Line(TailA, TailTip, 2.5f * S, Ink);
	Line(TailB, TailTip, 2.5f * S, Ink);
	Disc(FVector2D(BR.Min.X + 32.f * S, BR.Min.Y + 30.f * S), 9.f * S, C.SpeakerColor[Who]);
	Txt(C.SpeakerName[Who] + TEXT("  ·  ") + C.SpeakerRole[Who], BR.Min.X + 50.f * S, BR.Min.Y + 17.f * S, 22.f * S, DarkK);
	Para(L->Text, BR.Min.X + 26.f * S, BR.Min.Y + 54.f * S, 26.f * S, BR.GetSize().X - 52.f * S, Ink, 1.25f, GM->VisibleChars());
	if (GM->IsLineComplete())
	{
		const float A = 0.5f + 0.5f * FMath::Sin(GM->Now * 5.f);
		Txt(TEXT("devam için tıkla"), BR.Max.X - 22.f * S, BR.Max.Y - 30.f * S, 16.f * S, WithA(DarkK, 0.4f + 0.6f * A), 2);
	}
}

void ADBHUD::DrawToast()
{
	if (GM->Toast.IsEmpty())
	{
		return;
	}
	const float Age = GM->Now - GM->ToastTime;
	const float Life = 3.f + GM->Toast.Len() * 0.035f;
	if (Age > Life || Age < 0.f)
	{
		return;
	}
	const float A = FMath::Clamp((Life - Age) / 0.4f, 0.f, 1.f);
	const float Px = 24.f * S;
	const TArray<FString> Lines = Wrap(GM->Toast, Px, 820.f * S);
	float TW = 0.f;
	for (const FString& L : Lines)
	{
		TW = FMath::Max(TW, Measure(L, Px).X);
	}
	// Alt panelin üstünde: yukarıda kasabalı konuşma balonlarıyla çakışıyordu
	const float TH = Lines.Num() * Px * 1.25f + 26.f * S;
	const float Y0 = H - 196.f * S - TH;
	const FBox2D R = Rect(W * 0.5f - (TW + 48.f * S) * 0.5f, Y0, TW + 48.f * S, TH);
	Card(R, WithA(GM->bToastBad ? U(250, 222, 212) : U(236, 246, 228), A), WithA(GM->bToastBad ? Red : Green, A), 6.f);
	float Y = R.Min.Y + 13.f * S;
	for (const FString& L : Lines)
	{
		Txt(L, R.GetCenter().X, Y, Px, WithA(Ink, A), 1);
		Y += Px * 1.25f;
	}
}

void ADBHUD::DrawTooltip()
{
	const ADBPlayerController* PC = Cast<ADBPlayerController>(PlayerOwner);
	if (!PC || PC->HoveredBuilding < 0)
	{
		return;
	}
	const FDBContent& C = FDBContent::Get();
	const int32 B = PC->HoveredBuilding;
	const int32 P = GM->ProblemOfBuilding(B);
	const bool bKnown = P >= 0 && GM->Revealed[P];
	FString Sub;
	if (bKnown)
	{
		Sub = GM->Solved[P] ? FString(TEXT("Hazır.")) : FString::Printf(TEXT("[%s] %s"), *C.Problems[P].Hazard, *C.Problems[P].Short);
	}
	else if (GM->NumRevealed() > 0)
	{
		Sub = (B == C.ShelterBuilding) ? TEXT("Tsunami ve sel için güvenli toplanma yeri.") : TEXT("Tarayıcı sorun bulmadı.");
	}
	FString Act;
	if (GM->Phase == EDBPhase::Play && bKnown && !GM->Solved[P])
	{
		Act = (GM->SelTool >= 0 || GM->SelBag >= 0) ? TEXT("Tıkla: uygula") : TEXT("Tıkla: notu oku");
	}
	const float MaxW = 360.f * S;
	const TArray<FString> Lines = Wrap(Sub, 17.f * S, MaxW);
	float TW = Measure(C.Buildings[B].Name, 21.f * S).X;
	for (const FString& L : Lines)
	{
		TW = FMath::Max(TW, Measure(L, 17.f * S).X);
	}
	TW = FMath::Max(TW, Measure(Act, 16.f * S).X);
	const float TH = 34.f * S + Lines.Num() * 21.f * S + (Act.IsEmpty() ? 0.f : 24.f * S);
	FVector2D Pos = Mouse + FVector2D(22.f, 18.f) * S;
	Pos.X = FMath::Min(Pos.X, W - TW - 40.f * S);
	Pos.Y = FMath::Min(Pos.Y, H - TH - 10.f * S);
	const FBox2D R = Rect(Pos.X, Pos.Y, TW + 28.f * S, TH);
	Card(R, Paper, DarkK, 5.f);
	Txt(C.Buildings[B].Name, R.Min.X + 14.f * S, R.Min.Y + 8.f * S, 21.f * S, Ink);
	float Y = R.Min.Y + 36.f * S;
	for (const FString& L : Lines)
	{
		Txt(L, R.Min.X + 14.f * S, Y, 17.f * S, bKnown && !GM->Solved[P] ? Red : DarkK);
		Y += 21.f * S;
	}
	if (!Act.IsEmpty())
	{
		Txt(Act, R.Min.X + 14.f * S, Y + 2.f * S, 16.f * S, Teal);
	}
}

void ADBHUD::DrawSelectionCursor()
{
	if (GM->Phase != EDBPhase::Play)
	{
		return;
	}
	const FVector2D C = Mouse + FVector2D(30.f, -26.f) * S;
	if (GM->SelTool >= 0)
	{
		ToolIcon(GM->SelTool, C, 16.f * S);
	}
	else if (GM->Bags.IsValidIndex(GM->SelBag))
	{
		KitBadge(GM->Bags[GM->SelBag].Kit, Rect(C.X - 44.f * S, C.Y - 12.f * S, 88.f * S, 24.f * S), 14.f * S);
	}
}

// ---------------------------------------------------------------- sonuç

void ADBHUD::DrawResult()
{
	const FDBContent& C = FDBContent::Get();
	Box(0.f, 0.f, W, H, U(30, 20, 10, 120));
	Blockers.Add(Rect(0.f, 0.f, W, H));
	const float CW = 1180.f * S;
	const FBox2D R = Rect(W * 0.5f - CW * 0.5f, 40.f * S, CW, H - 80.f * S);
	Card(R, Paper, DarkK, 10.f);

	const bool bWon = GM->bWon;
	Txt(bWon ? TEXT("Kasaba afetlere hazır!") : TEXT("Kasaba afetlere hazır değildi"), R.GetCenter().X, R.Min.Y + 26.f * S, 44.f * S,
		bWon ? Green : Red, 1);
	Txt(FString::Printf(TEXT("Hazır: %d / %d   ·   Yanlış deneme: %d   ·   Kalan bütçe: %d TL"), GM->NumSolved(), GM->Solved.Num(),
		GM->Mistakes, GM->Money), R.GetCenter().X, R.Min.Y + 84.f * S, 24.f * S, Ink, 1);

	float Y = R.Min.Y + 132.f * S;
	const float TextX = R.Min.X + 84.f * S;
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		const FDBProblemDef& Pr = C.Problems[p];
		const bool bS = GM->Solved[p];
		const FVector2D Ic(R.Min.X + 50.f * S, Y + 12.f * S);
		Disc(Ic, 14.f * S, bS ? U(226, 244, 220) : U(250, 222, 212));
		if (bS)
		{
			TickMark(Ic, 20.f * S, Green);
		}
		else
		{
			Cross(Ic, 18.f * S, Red);
		}
		Txt(FString::Printf(TEXT("%s  ·  %s"), *C.Buildings[Pr.Building].Name, *Pr.Hazard), TextX, Y, 23.f * S, bS ? Ink : Red);
		Y = Para(Pr.Lesson, TextX, Y + 29.f * S, 18.f * S, CW - 130.f * S, DarkK, 1.2f, -1, R.Max.Y - 100.f * S) + 9.f * S;
	}

	const float BY = R.Max.Y - 86.f * S;
	Button(TEXT("Again"), -1, Rect(R.GetCenter().X - 290.f * S, BY, 270.f * S, 62.f * S), TEXT("Tekrar Oyna"), 26.f * S, Coral, Ink, Paper);
	Button(TEXT("Menu"), -1, Rect(R.GetCenter().X + 20.f * S, BY, 270.f * S, 62.f * S), TEXT("Menü"), 26.f * S, CardC, DarkK, Ink);
}

// ---------------------------------------------------------------- PNG (ekip görselleri dosyadan; yoksa çizilmiş yer tutucu)

UTexture2D* ADBHUD::LoadPng(const FString& Rel)
{
	if (const TObjectPtr<UTexture2D>* Found = PngCache.Find(Rel))
	{
		return Found->Get();
	}
	// Varlık testi: -DBVarlik="<klasör>" verilirse görseller önce oradan okunur (Content'e girmemiş, onay bekleyen dosyalar)
	FString Override;
	FString Path = FPaths::ProjectContentDir() / Rel;
	const TCHAR* Source = TEXT("content");
	if (FParse::Value(FCommandLine::Get(), TEXT("DBVarlik="), Override) && FPaths::FileExists(Override / Rel))
	{
		Path = Override / Rel;
		Source = TEXT("aday");
	}
	UTexture2D* T = FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
	PngCache.Add(Rel, T);
	if (T)
	{
		PngStatus.Add(Rel, FString::Printf(TEXT("OK %dx%d %s"), T->GetSizeX(), T->GetSizeY(), Source));
	}
	else
	{
		PngStatus.Add(Rel, FPaths::FileExists(Path) ? FString(TEXT("HATA okunamadi")) : FString(TEXT("YOK")));
	}
	return T;
}

FString ADBHUD::PngReport() const
{
	FString Out;
	TArray<FString> Keys;
	PngStatus.GetKeys(Keys);
	Keys.Sort();
	for (const FString& K : Keys)
	{
		Out += K + TEXT("\t") + PngStatus[K] + TEXT("\n");
	}
	return Out;
}

void ADBHUD::DrawTex(UTexture2D* T, const FBox2D& R, float RotDeg, const FVector2D& Pivot, const FLinearColor& Tint, bool bFlipX)
{
	if (!T || !T->GetResource())
	{
		return;
	}
	FCanvasTileItem It(R.Min, T->GetResource(), R.GetSize(), Gn(Tint));
	It.BlendMode = SE_BLEND_Translucent;
	It.Rotation = FRotator(0.f, RotDeg, 0.f);
	It.PivotPoint = Pivot;
	if (bFlipX)
	{
		It.UV0 = FVector2D(1.f, 0.f);
		It.UV1 = FVector2D(0.f, 1.f);
	}
	Canvas->DrawItem(It);
}

// ---------------------------------------------------------------- rehber karakter (eliyle gösterir)

bool ADBHUD::GuideTarget(int32 Who, FVector2D& Out) const
{
	const FDBContent& C = FDBContent::Get();
	auto Center = [&Out](const FBox2D& R) { if (!R.bIsValid) return false; Out = R.GetCenter(); return true; };
	if (const FDBLine* L = GM->CurrentLine())
	{
		if ((int32)L->Who != Who)
		{
			return false;
		}
		if (GM->IsTalking(C.IntroA, 2)) return Center(TgScanner);
		if (GM->IsTalking(C.AfterScan, 0))
		{
			const int32 T0 = GM->CurrentTask();
			return T0 >= 0 && PlayerOwner->ProjectWorldLocationToScreen(GM->World->MarkerAnchor(C.Problems[T0].Building), Out);
		}
		if (GM->IsTalking(C.BagIntroB, 1)) return Center(TgOrder);
		if (GM->IsTalking(C.BagIntroB, 2)) return Center(TgKitStation);
		if (GM->IsTalking(C.BagIntroB, 3)) return Center(TgStations);
		if (GM->IsTalking(C.BagIntroB, 4)) return Center(TgDeliver);
		if (GM->IsTalking(C.BagIntroB, 5)) return Center(TgClose);
		if (GM->IsTalking(C.PanelIntroC, 1)) return Center(TgTools);
		if (GM->IsTalking(C.PanelIntroC, 2)) return Center(TgMoney);
		if (GM->IsTalking(C.PanelIntroC, 3)) return Center(TgTools);
		if (GM->IsTalking(C.PanelIntroC, 4)) return Center(TgDrawer);
		return false;
	}
	if (Who != 0)
	{
		return false;
	}
	if (GM->Phase == EDBPhase::WaitScan)
	{
		return Center(TgScanner);
	}
	// Oyunda Mert sıradaki görevin binasını gösterir
	const int32 T = GM->CurrentTask();
	if (GM->Phase == EDBPhase::Play && T >= 0 && !GM->bPanelOpen)
	{
		return PlayerOwner->ProjectWorldLocationToScreen(GM->World->MarkerAnchor(C.Problems[T].Building), Out);
	}
	return false;
}

void ADBHUD::DrawGuide(int32 Who, const FVector2D& Feet, float Height)
{
	const FDBContent& Ct = FDBContent::Get();
	const float T = GM->Now;
	const float K = Height / 160.f;                         // 160 = tasarım boyu
	const FVector2D Base = Feet + FVector2D(0.f, -FMath::Abs(FMath::Sin(T * 3.f + Who)) * 3.f * K);
	FVector2D Target;
	const bool bPoint = GuideTarget(Who, Target);
	const FVector2D Shoulder = Base + FVector2D(bPoint && Target.X < Base.X ? -20.f : 20.f, -98.f) * K;
	float Ang;                                               // derece, 0 = sağ, 90 = aşağı
	float Reach = 54.f * K;
	if (bPoint)
	{
		const FVector2D D = Target - Shoulder;
		Ang = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)) + FMath::Sin(T * 7.f) * 4.f;
		Reach *= 1.f + 0.07f * FMath::Sin(T * 7.f);
	}
	else if (GM->CurrentLine() && (int32)GM->CurrentLine()->Who == Who)
	{
		Ang = -90.f + FMath::Sin(T * 6.f) * 28.f;           // konuşurken el sallar
	}
	else
	{
		Ang = 80.f;                                          // kol aşağıda
	}

	// Ekip görseli iki poz (Mert'in çizimi gibi, 17 Ekim): duruş ve bir şeyi gösterirken kol kaldırılmış. İkisi aynı
	// çerçevede, aynı hizada (Content/Karakterler/BENIOKU.md). Hedef soldaysa aynalanır. El noktası çerçevenin
	// %90 sağı, %45 aşağısı: Mert'in çiziminden ölçüldü, şablonda yeşil daire.
	UTexture2D* IdleTex = LoadPng(FString::Printf(TEXT("Karakterler/%s_durus.png"), Letters[Who]));
	UTexture2D* ShowTex = LoadPng(FString::Printf(TEXT("Karakterler/%s_goster.png"), Letters[Who]));
	UTexture2D* PoseTex = (bPoint && ShowTex) ? ShowTex : (IdleTex ? IdleTex : ShowTex);
	const bool bFlip = bPoint && Target.X < Base.X;
	FVector2D HandPt = FVector2D::ZeroVector;
	Box(Rect(Feet.X - 30.f * K, Feet.Y - 4.f * K, 60.f * K, 7.f * K), U(60, 40, 25, 70));
	if (PoseTex)
	{
		const float Hh = Height * 1.15f;   // karakter çerçevenin ~%87'si: çizilen boy ≈ Height
		const float Ww = Hh * PoseTex->GetSizeX() / FMath::Max(1, PoseTex->GetSizeY());
		const FBox2D R = Rect(Base.X - Ww * 0.5f, Base.Y - Hh, Ww, Hh);
		DrawTex(PoseTex, R, 0.f, FVector2D(0.5f, 0.5f), FLinearColor::White, bFlip);
		HandPt = FVector2D(R.Min.X + Ww * (bFlip ? 0.10f : 0.90f), R.Min.Y + Hh * 0.45f);
	}
	else
	{
		// Yer tutucu: çizilmiş figür
		const FLinearColor Shirt = Ct.SpeakerColor[Who];
		const FLinearColor Skin = U(240, 200, 160);
		const FLinearColor Hair = Who == 0 ? U(70, 50, 36) : (Who == 1 ? U(40, 34, 30) : U(120, 80, 50));
		Box(Rect(Base.X - 16.f * K, Base.Y - 40.f * K, 12.f * K, 40.f * K), U(70, 80, 110));
		Box(Rect(Base.X + 4.f * K, Base.Y - 40.f * K, 12.f * K, 40.f * K), U(70, 80, 110));
		Box(Rect(Base.X - 26.f * K, Base.Y - 104.f * K, 52.f * K, 68.f * K), Shirt);
		Disc(Base + FVector2D(-18.f, -100.f) * K, 10.f * K, Shirt);
		Disc(Base + FVector2D(18.f, -100.f) * K, 10.f * K, Shirt);
		// Öbür kol aşağıda
		const FVector2D Other = Base + FVector2D(bPoint && Target.X < Base.X ? 22.f : -22.f, -96.f) * K;
		Line(Other, Other + FVector2D(0.f, 44.f) * K, 10.f * K, Shirt);
		Disc(Other + FVector2D(0.f, 46.f) * K, 6.f * K, Skin);
		// Baş
		const FVector2D Head = Base + FVector2D(0.f, -132.f) * K;
		Disc(Head + FVector2D(0.f, -5.f) * K, 27.f * K, Hair);
		Disc(Head + FVector2D(0.f, 3.f) * K, 24.f * K, Skin);
		const float LookX = bPoint ? FMath::Clamp((Target.X - Base.X) * 0.02f, -4.f, 4.f) : 0.f;
		Disc(Head + FVector2D(-8.f + LookX, 0.f) * K, 3.f * K, Ink, 10);
		Disc(Head + FVector2D(8.f + LookX, 0.f) * K, 3.f * K, Ink, 10);
		Line(Head + FVector2D(-7.f, 12.f) * K, Head + FVector2D(0.f, 15.f) * K, 2.f * K, Ink);
		Line(Head + FVector2D(0.f, 15.f) * K, Head + FVector2D(7.f, 12.f) * K, 2.f * K, Ink);
		// Gösteren kol: omuzdan hedefe, elde uzanan işaret parmağı
		const float R = FMath::DegreesToRadians(Ang);
		const FVector2D Dir(FMath::Cos(R), FMath::Sin(R));
		const FVector2D Hand = Shoulder + Dir * Reach;
		Line(Shoulder, Hand, 11.f * K, Shirt);
		Disc(Hand, 7.f * K, Skin);
		if (bPoint)
		{
			Line(Hand, Hand + Dir * 14.f * K, 4.f * K, Skin);
		}
	}
	// Oyunda (konuşma yokken) elden hedefe kayan noktalı iz: neyi gösterdiği açık olsun
	if (bPoint && !GM->CurrentLine())
	{
		const FVector2D From = PoseTex ? HandPt + (Target - HandPt).GetSafeNormal() * 10.f * K
		                               : Shoulder + (Target - Shoulder).GetSafeNormal() * (Reach + 20.f * K);
		const float Len = FVector2D::Distance(From, Target) - 34.f * S;
		const FVector2D Dir = (Target - From).GetSafeNormal();
		for (float D = FMath::Fmod(T * 60.f * S, 26.f * S); D < Len; D += 26.f * S)
		{
			Disc(From + Dir * D, 4.f * S, U(255, 210, 60), 10);
		}
	}
	const FString& Name = Ct.SpeakerName[Who];
	const float TW = Measure(Name, 17.f * S).X + 18.f * S;
	const FBox2D Tag = Rect(Feet.X - TW * 0.5f, Feet.Y + 4.f * S, TW, 26.f * S);
	Card(Tag, Paper, Ink, 3.f);
	Txt(Name, Feet.X, Tag.Min.Y + 4.f * S, 17.f * S, Ink, 1);
}

// ---------------------------------------------------------------- sıralı görev listesi (sol üst)

void ADBHUD::DrawTasks()
{
	const FDBContent& C = FDBContent::Get();
	if (GM->NumRevealed() == 0)
	{
		TgTasks = FBox2D(ForceInit);
		return;
	}
	const float X = 22.f * S;
	const float Wd = 430.f * S;
	const int32 Cur = GM->CurrentTask();
	// Önce yüksekliği hesapla
	float Hgt = 56.f * S;
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (!GM->Revealed[p]) continue;
		Hgt += (p == Cur) ? (34.f + 22.f * Wrap(C.Problems[p].Hint, 17.f * S, Wd - 60.f * S).Num() + 34.f) * S : 26.f * S;
	}
	const FBox2D R = Rect(X, 18.f * S, Wd, Hgt + 8.f * S);
	TgTasks = R;
	if (GM->IsTalking(C.AfterScan, 0) || GM->IsTalking(C.BagIntroB, 1))
	{
		Glow(R);
	}
	Box(R.ShiftBy(FVector2D(5.f, 7.f) * S), ShadowC);
	Box(R, NoteY);
	Box(Rect(R.GetCenter().X - 50.f * S, R.Min.Y - 10.f * S, 100.f * S, 24.f * S), U(255, 255, 255, 150));
	Blockers.Add(R);
	Txt(TEXT("GÖREVLER"), X + 18.f * S, R.Min.Y + 14.f * S, 24.f * S, Ink);
	Txt(FString::Printf(TEXT("%d / %d"), GM->NumSolved(), GM->Solved.Num()), R.Max.X - 18.f * S, R.Min.Y + 16.f * S, 20.f * S, DarkK, 2);
	float Y = R.Min.Y + 52.f * S;
	int32 Num = 0;
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (!GM->Revealed[p]) continue;
		++Num;
		const FDBProblemDef& Pr = C.Problems[p];
		const FString& Name = C.Buildings[Pr.Building].Name;
		if (GM->Solved[p])
		{
			TickMark(FVector2D(X + 28.f * S, Y + 10.f * S), 18.f * S, Green);
			Txt(Name, X + 48.f * S, Y, 18.f * S, U(150, 136, 116));
			Line(FVector2D(X + 48.f * S, Y + 11.f * S), FVector2D(X + 48.f * S + Measure(Name, 18.f * S).X, Y + 11.f * S), 1.5f * S, U(150, 136, 116));
			Y += 26.f * S;
		}
		else if (p == Cur)
		{
			// Sıradaki görev: kart, ipucu ve ne tür çözüm gerektiği (cevabı söylemeden)
			const TArray<FString> Lines = Wrap(Pr.Hint, 17.f * S, Wd - 60.f * S);
			const FBox2D Cd = Rect(X + 10.f * S, Y - 4.f * S, Wd - 20.f * S, (30.f + 22.f * Lines.Num() + 34.f) * S);
			Card(Cd, Paper, Coral, 3.f);
			Txt(FString::Printf(TEXT("%d. %s"), Num, *Name), X + 24.f * S, Y + 2.f * S, 21.f * S, Ink);
			float LY = Y + 30.f * S;
			for (const FString& L : Lines)
			{
				Txt(L, X + 24.f * S, LY, 17.f * S, DarkK);
				LY += 22.f * S;
			}
			const bool bKit = Pr.Kit >= 0;
			const FString Chip = bKit ? TEXT("Bir çanta lazım") : TEXT("Bir şehir önlemi lazım");
			const float CW = Measure(Chip, 15.f * S).X + 20.f * S;
			Box(Rect(X + 24.f * S, LY + 4.f * S, CW, 22.f * S), bKit ? Coral : Teal);
			Txt(Chip, X + 34.f * S, LY + 7.f * S, 15.f * S, Paper);
			Y += Cd.GetSize().Y + 6.f * S;
		}
		else
		{
			Txt(FString::Printf(TEXT("%d. %s"), Num, *Name), X + 24.f * S, Y, 18.f * S, U(110, 96, 80));
			Y += 26.f * S;
		}
	}
}

// ---------------------------------------------------------------- giriş çizgi romanı

void ADBHUD::DrawMiniFriend(const FVector2D& Feet, float K, const FLinearColor& Shirt, float Bob)
{
	const FVector2D B = Feet - FVector2D(0.f, Bob);
	Box(Rect(B.X - 9.f * K, B.Y - 24.f * K, 7.f * K, 24.f * K), U(70, 80, 110));
	Box(Rect(B.X + 2.f * K, B.Y - 24.f * K, 7.f * K, 24.f * K), U(70, 80, 110));
	Box(Rect(B.X - 15.f * K, B.Y - 62.f * K, 30.f * K, 40.f * K), Shirt);
	Disc(B + FVector2D(0.f, -78.f) * K, 16.f * K, U(70, 50, 36));
	Disc(B + FVector2D(0.f, -74.f) * K, 14.f * K, U(240, 200, 160));
}

void ADBHUD::DrawComic()
{
	const FDBContent& C = FDBContent::Get();
	const int32 Ix = FMath::Clamp(GM->ComicIndex, 0, C.Comic.Num() - 1);
	const float T = GM->PhaseTime;
	Box(0.f, 0.f, W, H, U(34, 26, 20, 235));
	Blockers.Add(Rect(0.f, 0.f, W, H));
	const FBox2D P = Rect(W * 0.5f - 640.f * S, 60.f * S, 1280.f * S, 720.f * S);
	Box(P.ExpandBy(10.f * S), Paper);
	Frame(P.ExpandBy(10.f * S), Ink, 4.f * S);

	if (UTexture2D* Img = LoadPng(FString::Printf(TEXT("Cizgiroman/sahne%d.png"), Ix + 1)))
	{
		DrawTex(Img, P, 0.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		// Yer tutucu sahneler (ekip çizimi gelene kadar)
		const FLinearColor Friends[5] = { C.SpeakerColor[0], C.SpeakerColor[1], C.SpeakerColor[2], U(172, 152, 204), U(120, 180, 110) };
		const float K = 1.6f * S;
		const float Ground = P.Min.Y + P.GetSize().Y * 0.78f;
		Box(Rect(P.Min.X, P.Min.Y, P.GetSize().X, Ground - P.Min.Y), Ix == 3 ? U(236, 214, 180) : U(186, 220, 238));
		Box(Rect(P.Min.X, Ground, P.GetSize().X, P.Max.Y - Ground), Ix == 3 ? U(170, 120, 80) : U(214, 196, 160));
		if (Ix == 0)
		{
			for (int32 h = 0; h < 6; ++h)
			{
				const float Hx = P.Min.X + 60.f * S + h * 210.f * S;
				const float Hh = (110.f + (h % 3) * 40.f) * S;
				Box(Rect(Hx, Ground - Hh, 150.f * S, Hh), h % 2 ? U(232, 128, 104) : U(244, 232, 208));
				Tri(FVector2D(Hx - 10.f * S, Ground - Hh), FVector2D(Hx + 160.f * S, Ground - Hh), FVector2D(Hx + 75.f * S, Ground - Hh - 60.f * S), U(186, 92, 70));
			}
			for (int32 f = 0; f < 5; ++f)
			{
				const float Fx = P.Min.X + 200.f * S + f * 90.f * S + T * 60.f * S;
				DrawMiniFriend(FVector2D(Fx, Ground + 40.f * S), K, Friends[f], FMath::Abs(FMath::Sin(T * 8.f + f)) * 6.f * S);
			}
		}
		else if (Ix == 1)
		{
			const FBox2D Shop = Rect(P.Min.X + 520.f * S, P.Min.Y + 90.f * S, 640.f * S, 440.f * S);
			Box(Shop, U(150, 108, 72));
			Box(Shop.ExpandBy(-24.f * S), U(200, 228, 240));
			const FBox2D Game = Rect(Shop.GetCenter().X - 150.f * S, Shop.Max.Y - 200.f * S, 300.f * S, 150.f * S);
			Card(Game, U(196, 154, 108), Ink, 6.f);
			Txt(TEXT("DisasterBoard"), Game.GetCenter().X, Game.Min.Y + 52.f * S, 34.f * S, Paper, 1);
			for (int32 f = 0; f < 5; ++f)
			{
				DrawMiniFriend(FVector2D(P.Min.X + 140.f * S + f * 70.f * S, Ground + 30.f * S), K, Friends[f], FMath::Abs(FMath::Sin(T * 2.f + f)) * 3.f * S);
			}
		}
		else if (Ix == 2)
		{
			const FBox2D Board = Rect(P.GetCenter().X - 260.f * S, P.Min.Y + 170.f * S, 520.f * S, 360.f * S);
			Box(Board.ExpandBy(20.f * S), U(196, 154, 108));
			Box(Board, U(236, 218, 182));
			for (int32 g = 1; g < 8; ++g)
			{
				Line(FVector2D(Board.Min.X + g * 65.f * S, Board.Min.Y), FVector2D(Board.Min.X + g * 65.f * S, Board.Max.Y), 2.f * S, U(196, 154, 108));
				if (g < 6) Line(FVector2D(Board.Min.X, Board.Min.Y + g * 60.f * S), FVector2D(Board.Max.X, Board.Min.Y + g * 60.f * S), 2.f * S, U(196, 154, 108));
			}
			const FLinearColor Hs[] = { U(232, 128, 104), U(132, 176, 212), U(236, 188, 86), U(96, 170, 164) };
			for (int32 h = 0; h < 4; ++h)
			{
				const float Pop = FMath::Clamp(T * 2.f - h * 0.4f, 0.f, 1.f);
				const FBox2D Hb = Rect(Board.Min.X + (50.f + h * 120.f) * S, Board.Min.Y + (90.f + (h % 2) * 120.f) * S - 60.f * S * Pop, 60.f * S, 60.f * S * Pop + 1.f);
				Box(Hb, Hs[h]);
			}
			const FBox2D Note = Rect(P.Min.X + 60.f * S, P.Min.Y + 60.f * S, 300.f * S, 120.f * S);
			Card(Note, NoteY, Ink, 4.f);
			Para(TEXT("Afet gelmeden kasabayı hazırla."), Note.Min.X + 18.f * S, Note.Min.Y + 20.f * S, 26.f * S, 270.f * S, Ink);
		}
		else
		{
			const FBox2D Table = Rect(P.GetCenter().X - 330.f * S, Ground - 150.f * S, 660.f * S, 90.f * S);
			const float Angles[5] = { -150.f, -110.f, -70.f, -30.f, 180.f };
			for (int32 f = 0; f < 4; ++f)
			{
				const float A = FMath::DegreesToRadians(Angles[f]);
				DrawMiniFriend(FVector2D(Table.GetCenter().X + FMath::Cos(A) * 300.f * S, Table.Min.Y + 20.f * S), K, Friends[f], FMath::Abs(FMath::Sin(T * 3.f + f)) * 4.f * S);
			}
			Box(Table, U(158, 110, 72));
			Box(Rect(Table.GetCenter().X - 200.f * S, Table.Min.Y - 24.f * S, 400.f * S, 30.f * S), U(236, 218, 182));
			// Oyuncu (5. kişi) ön planda, arkası dönük
			Disc(FVector2D(P.GetCenter().X, P.Max.Y - 40.f * S), 70.f * S, Friends[4]);
			Disc(FVector2D(P.GetCenter().X, P.Max.Y - 150.f * S), 52.f * S, U(70, 50, 36));
		}
	}

	// Altyazı kutusu (çizgi roman anlatıcı kutusu)
	const FString& Cap = C.Comic[Ix];
	const TArray<FString> Lines = Wrap(Cap, 30.f * S, 900.f * S);
	const FBox2D CapR = Rect(P.Min.X + 30.f * S, P.Max.Y - 40.f * S - Lines.Num() * 38.f * S, 960.f * S, Lines.Num() * 38.f * S + 24.f * S);
	Card(CapR, NoteY, Ink, 6.f);
	float Y = CapR.Min.Y + 12.f * S;
	const int32 Shown = FMath::FloorToInt(T * 40.f);
	int32 Left = Shown;
	for (const FString& L : Lines)
	{
		if (Left <= 0) break;
		Txt(Left >= L.Len() ? L : L.Left(Left), CapR.Min.X + 20.f * S, Y, 30.f * S, Ink);
		Left -= L.Len() + 1;
		Y += 38.f * S;
	}

	// İlerleme: sahne noktaları ve süre çubuğu
	for (int32 i = 0; i < C.Comic.Num(); ++i)
	{
		Disc(FVector2D(W * 0.5f + (i - (C.Comic.Num() - 1) * 0.5f) * 28.f * S, P.Max.Y + 44.f * S), 7.f * S, i <= Ix ? Paper : U(120, 100, 80));
	}
	const float Pr = FMath::Clamp(T / GM->ComicSeconds, 0.f, 1.f);
	Box(Rect(P.Min.X, P.Max.Y + 12.f * S, P.GetSize().X * Pr, 5.f * S), U(236, 188, 86));
	Txt(TEXT("tıkla: sonraki sahne"), P.Min.X, P.Max.Y + 34.f * S, 18.f * S, U(220, 200, 170));
	Button(TEXT("Skip"), -1, Rect(P.Max.X - 150.f * S, P.Max.Y + 26.f * S, 150.f * S, 46.f * S), TEXT("Geç"), 22.f * S, CardC, Ink, Ink);
}

// ---------------------------------------------------------------- kasabalı konuşması, ızgara ipuçları

void ADBHUD::DotRing(const FVector2D& C, float R, const FLinearColor& Col)
{
	const int32 N = 14;
	for (int32 i = 0; i < N; ++i)
	{
		const float A = (i + GM->Now * 1.5f) * 2.f * PI / N;
		Disc(C + FVector2D(FMath::Cos(A), FMath::Sin(A) * 0.55f) * R, 3.2f * S, Col, 10);
	}
}

void ADBHUD::DrawCellHints()
{
	const FDBContent& C = FDBContent::Get();
	if (GM->Phase != EDBPhase::Play || GM->SelTool < 0 || GM->SelTool != C.FloodTool)
	{
		return;
	}
	// Rögar seçiliyken su birikintileri halkalanır, imlecin altındaki karo gösterilir
	const float Pulse = 1.f + 0.12f * FMath::Sin(GM->Now * 6.f);
	for (const FIntPoint& F : C.FloodCells)
	{
		FVector2D P;
		if (!GM->World->HasManhole(F) && PlayerOwner->ProjectWorldLocationToScreen(GM->World->CellWorld(F), P))
		{
			DotRing(P, 26.f * S * Pulse, U(70, 150, 230));
		}
	}
	const ADBPlayerController* PC = Cast<ADBPlayerController>(PlayerOwner);
	FVector2D Hp;
	if (PC && PC->bHoveredCell && GM->World->IsRoad(PC->HoveredCell) && PlayerOwner->ProjectWorldLocationToScreen(GM->World->CellWorld(PC->HoveredCell), Hp))
	{
		const bool bGood = C.FloodCells.Contains(PC->HoveredCell) && !GM->World->HasManhole(PC->HoveredCell);
		DotRing(Hp, 20.f * S, bGood ? Green : U(250, 243, 228));
	}
}

void ADBHUD::DrawNpc()
{
	const FDBContent& C = FDBContent::Get();
	const int32 B = GM->NpcBuilding;
	if (!C.Residents.IsValidIndex(B))
	{
		return;
	}
	const float Age = GM->Now - GM->NpcTime;
	if (GM->NpcKind == 2 && Age > 5.f)
	{
		GM->CloseNpc();   // teşekkür kısa sürer
		return;
	}
	FVector2D A;
	if (!PlayerOwner->ProjectWorldLocationToScreen(GM->World->ResidentWorld(B), A))
	{
		return;
	}
	const FString Text = GM->NpcText();
	const bool bBag = GM->NpcWantsBag();
	const float BW = 430.f * S;
	const TArray<FString> Lines = Wrap(Text, 20.f * S, BW - 40.f * S);
	const float BH = 50.f * S + Lines.Num() * 26.f * S + 58.f * S;
	FVector2D Pos(A.X - 70.f * S, A.Y - 34.f * S - BH);
	Pos.X = FMath::Clamp(Pos.X, 20.f * S, W - BW - 20.f * S);
	Pos.Y = FMath::Max(Pos.Y, 84.f * S);
	const FBox2D R = Rect(Pos.X, Pos.Y, BW, BH);
	const FVector2D T0(FMath::Clamp(A.X, R.Min.X + 30.f * S, R.Max.X - 70.f * S), R.Max.Y - 2.f);
	Card(R, Paper, Ink, 6.f);
	Tri(T0, T0 + FVector2D(40.f * S, 0.f), A, Paper);
	Line(T0, A, 2.5f * S, Ink);
	Line(T0 + FVector2D(40.f * S, 0.f), A, 2.5f * S, Ink);
	Blockers.Add(R);
	Disc(FVector2D(R.Min.X + 26.f * S, R.Min.Y + 26.f * S), 9.f * S, GM->NpcKind == 2 ? Green : (GM->NpcKind == 1 ? Coral : Teal));
	Txt(C.Residents[B].Name, R.Min.X + 42.f * S, R.Min.Y + 13.f * S, 22.f * S, Ink);
	const int32 Shown = FMath::FloorToInt(Age * 50.f);
	int32 Left = Shown;
	float Y = R.Min.Y + 46.f * S;
	for (const FString& L : Lines)
	{
		if (Left <= 0) break;
		Txt(Left >= L.Len() ? L : L.Left(Left), R.Min.X + 20.f * S, Y, 20.f * S, Ink);
		Left -= L.Len() + 1;
		Y += 26.f * S;
	}
	const float BY = R.Max.Y - 50.f * S;
	if (bBag)
	{
		Button(TEXT("NpcBag"), -1, Rect(R.Min.X + 18.f * S, BY, 230.f * S, 38.f * S), TEXT("Çantasını hazırla"), 19.f * S, Coral, Ink, Paper);
	}
	Button(TEXT("NpcClose"), -1, Rect(R.Max.X - 128.f * S, BY, 110.f * S, 38.f * S), TEXT("Tamam"), 19.f * S, CardC, DarkK, Ink);
}

// ---------------------------------------------------------------- tahta döndürme oku, mini oyunlar

void ADBHUD::TurnArrow(const FVector2D& C, float R, int32 Dir, const FLinearColor& Col)
{
	// Üstten dönen yay + ok ucu: sağ ok saat yönünde, sol ok tersine
	const float A0 = FMath::DegreesToRadians(Dir > 0 ? 160.f : 380.f);
	const float A1 = FMath::DegreesToRadians(Dir > 0 ? 380.f : 160.f);
	const int32 N = 16;
	FVector2D Prev = C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R;
	for (int32 i = 1; i <= N; ++i)
	{
		const float A = FMath::Lerp(A0, A1, (float)i / N);
		const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
		Line(Prev, P, 5.f * S, Col);
		Prev = P;
	}
	const FVector2D Nrm(FMath::Cos(A1), FMath::Sin(A1));
	const FVector2D Tan = FVector2D(-Nrm.Y, Nrm.X) * (A1 > A0 ? 1.f : -1.f);
	const float Hd = 11.f * S;
	Tri(Prev + Tan * Hd, Prev + Nrm * Hd * 0.8f, Prev - Nrm * Hd * 0.8f, Col);
}

void ADBHUD::Hex(const FVector2D& C, float R, float AngleRad, const FLinearColor& Col)
{
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = AngleRad + i * PI / 3.f;
		const float B = AngleRad + (i + 1) * PI / 3.f;
		Tri(C, C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R, C + FVector2D(FMath::Cos(B), FMath::Sin(B)) * R, Col);
	}
}

void ADBHUD::DrawMini()
{
	const bool bNut = GM->Mini == EDBMini::Nut;
	Box(0.f, 0.f, W, H, U(40, 30, 22, 120));
	Blockers.Add(Rect(0.f, 0.f, W, H));
	const FBox2D R = Rect(W * 0.5f - 400.f * S, H * 0.5f - 280.f * S, 800.f * S, 540.f * S);
	const float CX = R.GetCenter().X;
	Card(R, Paper, Ink, 8.f);
	Txt(bNut ? TEXT("Yangın musluğunu tamir et") : TEXT("Rögar kapağını yerine oturt"), CX, R.Min.Y + 20.f * S, 32.f * S, Ink, 1);
	const FString How = bNut
		? TEXT("Basılı tut: anahtar somunu sıkar ve gittikçe hızlanır. Yeşil çizgiye gelince bırak!")
		: TEXT("Basılı tut: kapak sağa kayar, bırakınca sola. Kapağı yeşil alanda 4 saniye tut!");
	float Y = R.Min.Y + 68.f * S;
	for (const FString& L : Wrap(How, 19.f * S, 700.f * S))
	{
		Txt(L, CX, Y, 19.f * S, DarkK, 1);
		Y += 25.f * S;
	}
	if (bNut)
	{
		DrawNutGame(R);
	}
	else
	{
		DrawManholeGame(R);
	}
	if (!GM->MiniMsg.IsEmpty() && GM->Now - GM->MiniMsgTime < 2.5f)
	{
		Txt(GM->MiniMsg, CX, R.Max.Y - 104.f * S, 23.f * S, GM->bMiniMsgBad ? Red : Green, 1);
	}
	Txt(TEXT("Fare sol tuşu ya da Boşluk  ·  Esc / sağ tık: vazgeç"), R.Min.X + 24.f * S, R.Max.Y - 44.f * S, 15.f * S, DarkK);
	if (GM->MiniWinAt < 0.f)
	{
		Button(TEXT("MiniCancel"), -1, Rect(R.Max.X - 170.f * S, R.Max.Y - 62.f * S, 150.f * S, 46.f * S), TEXT("Vazgeç"), 20.f * S, CardC, DarkK, Ink);
	}
	else
	{
		const float A = FMath::Clamp((GM->Now - GM->MiniWinAt) / 0.25f, 0.f, 1.f);
		TickMark(FVector2D(CX + 300.f * S, R.Min.Y + 150.f * S), 70.f * S * A + 10.f * S, Green);
	}
	// Basılı tutarken düğme gibi çöker: oyuncu basılı tuttuğunu görsün
	const FBox2D Pad = Rect(R.Min.X + 24.f * S, R.Min.Y + 130.f * S, 132.f * S, 46.f * S);
	const bool bHold = GM->bMiniHold || GM->bMiniAutoTest;
	Card(bHold ? Pad.ShiftBy(FVector2D(0.f, 3.f * S)) : Pad, bHold ? Coral : CardC, DarkK, bHold ? 1.f : 4.f);
	Txt(bHold ? (bNut ? TEXT("SIKIYOR") : TEXT("BASILI")) : TEXT("Basılı tut"), Pad.GetCenter().X, Pad.Min.Y + (bHold ? 14.f : 11.f) * S, 18.f * S, bHold ? Paper : Ink, 1);
}

void ADBHUD::DrawNutGame(const FBox2D& R)
{
	// Anahtar yarım daire boyunca soldan sağa döner; yay üstünde yeşil çizgi, sonunda kırmızı (fazla sıkma)
	const FVector2D G(R.GetCenter().X, R.Min.Y + 372.f * S);
	const float Rg = 175.f * S;
	auto Dir = [](float Pos) { const float A = PI + Pos * PI; return FVector2D(FMath::Cos(A), FMath::Sin(A)); };
	const int32 N = 48;
	for (int32 i = 0; i < N; ++i)
	{
		const float P0 = (float)i / N;
		const float P1 = (float)(i + 1) / N;
		const float Pm = (P0 + P1) * 0.5f;
		FLinearColor Col = U(214, 204, 190);
		if (Pm > 0.93f) Col = U(214, 76, 66);
		else if (FMath::Abs(Pm - GM->MiniZone) <= GM->MiniZoneHalf) Col = U(90, 186, 104);
		Line(G + Dir(P0) * Rg, G + Dir(P1) * Rg, 16.f * S, Col);
	}
	for (int32 i = 0; i <= 10; ++i)
	{
		const FVector2D D = Dir(i / 10.f);
		Line(G + D * (Rg + 12.f * S), G + D * (Rg + 22.f * S), 2.f * S, DarkK);
	}
	// Yeşil çizgi: hedef (yayın dışında, ok ucuyla; yayın üstünde çapraz duruyordu, "X" gibi okunuyordu)
	const FVector2D Dz = Dir(GM->MiniZone);
	const FVector2D Nz(-Dz.Y, Dz.X);
	Line(G + Dz * (Rg + 10.f * S), G + Dz * (Rg + 44.f * S), 6.f * S, U(40, 150, 70));
	Tri(G + Dz * (Rg + 10.f * S), G + Dz * (Rg + 24.f * S) + Nz * 9.f * S, G + Dz * (Rg + 24.f * S) - Nz * 9.f * S, U(40, 150, 70));
	// İngiliz anahtarı: sap ibre gibi döner, ucunda kırmızı tutamak; ağzı somunu kavrar
	const FVector2D Dh = Dir(GM->MiniPos);
	const FLinearColor Steel = U(120, 128, 150);
	Line(G, G + Dh * (Rg - 26.f * S), 24.f * S, U(70, 74, 90));
	Line(G, G + Dh * (Rg - 28.f * S), 18.f * S, Steel);
	Disc(G + Dh * (Rg - 40.f * S), 15.f * S, Coral);
	Disc(G, 66.f * S, U(70, 74, 90));
	Disc(G, 60.f * S, Steel);
	const float Spin = GM->MiniPos * 2.f * PI;
	Hex(G, 44.f * S, Spin, U(60, 62, 70));
	Hex(G, 40.f * S, Spin, U(176, 178, 188));
	Disc(G, 16.f * S, U(96, 98, 108));
	Txt(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GM->MiniPos * 100.f)), G.X, G.Y + 72.f * S, 16.f * S, DarkK, 1);
}

void ADBHUD::DrawManholeGame(const FBox2D& R)
{
	// Yol şeridi: yeşil alan gezinir, kapak basılı tutunca sağa kayar
	const FBox2D Tr = Rect(R.Min.X + 80.f * S, R.Min.Y + 220.f * S, 640.f * S, 96.f * S);
	const float TW = Tr.GetSize().X;
	Box(Tr.ShiftBy(FVector2D(3.f, 5.f) * S), ShadowC);
	Box(Tr, U(98, 100, 110));
	for (int32 i = 0; i < 8; ++i)
	{
		Box(Rect(Tr.Min.X + 20.f * S + i * 80.f * S, Tr.GetCenter().Y - 2.f * S, 40.f * S, 4.f * S), U(236, 230, 210));
	}
	const float Z0 = Tr.Min.X + (GM->MiniZone - GM->MiniZoneHalf) * TW;
	const float Z1 = Tr.Min.X + (GM->MiniZone + GM->MiniZoneHalf) * TW;
	const bool bIn = FMath::Abs(GM->MiniPos - GM->MiniZone) <= GM->MiniZoneHalf;
	const FBox2D Zone = FBox2D(FVector2D(Z0, Tr.Min.Y - 8.f * S), FVector2D(Z1, Tr.Max.Y + 8.f * S));
	Box(Zone, bIn ? U(90, 200, 110, 200) : U(90, 186, 104, 140));
	Frame(Zone, U(40, 150, 70), 3.f * S);
	// Kapak
	const FVector2D Cv(Tr.Min.X + GM->MiniPos * TW, Tr.GetCenter().Y);
	Disc(Cv + FVector2D(3.f, 5.f) * S, 40.f * S, ShadowC);
	Disc(Cv, 40.f * S, U(56, 58, 64));
	Disc(Cv, 33.f * S, U(96, 98, 106));
	for (int32 k = -1; k <= 1; ++k)
	{
		Box(Rect(Cv.X + k * 13.f * S - 3.f * S, Cv.Y - 22.f * S, 6.f * S, 44.f * S), U(56, 58, 64));
	}
	// İlerleme: yeşilde geçen süre
	const float Total = GM->ManholeHoldSeconds;
	const float F = FMath::Clamp(GM->MiniProgress / Total, 0.f, 1.f);
	const FBox2D Bar = Rect(Tr.Min.X, Tr.Max.Y + 56.f * S, TW, 28.f * S);
	Box(Bar, CardC);
	Box(Rect(Bar.Min.X, Bar.Min.Y, TW * F, Bar.GetSize().Y), Green);
	Frame(Bar, DarkK, 2.f * S);
	FString Sec = FString::Printf(TEXT("Yeşilde: %.1f / %.0f sn"), GM->MiniProgress, Total).Replace(TEXT("."), TEXT(","));
	Txt(Sec, Bar.GetCenter().X, Bar.Max.Y + 6.f * S, 18.f * S, Ink, 1);
}

// ---------------------------------------------------------------- geri bildirim: fare izi, tıklama, rüzgar

void ADBHUD::AddClickFx(const FVector2D& P)
{
	ClickFxs.Add({ P, FxNow });
}

void ADBHUD::DrawCursorFx()
{
	const float Now = FxNow;
	// Fare izi: son 0,3 sn'deki konumlar, sona doğru incelip söner
	if (Trail.Num() == 0 || FVector2D::Distance(Trail.Last().P, Mouse) > 3.f)
	{
		Trail.Add({ Mouse, Now });
	}
	Trail.RemoveAll([Now](const FTrail& T) { return Now - T.T > 0.3f; });
	for (int32 i = 1; i < Trail.Num(); ++i)
	{
		const float K = 1.f - (Now - Trail[i].T) / 0.3f;
		const float Wd = (1.5f + 4.5f * K) * S;
		Line(Trail[i - 1].P, Trail[i].P, Wd + 2.f * S, U(58, 42, 30, uint8(70 * K)));
		Line(Trail[i - 1].P, Trail[i].P, Wd, U(255, 214, 90, uint8(200 * K)));
	}
	// Tıklama: büyüyen halka + dışa saçılan kıvılcımlar (0,4 sn)
	ClickFxs.RemoveAll([Now](const FClickFx& C) { return Now - C.T > 0.4f; });
	for (const FClickFx& C : ClickFxs)
	{
		const float A = (Now - C.T) / 0.4f;
		const float E = 1.f - FMath::Square(1.f - A);   // hızlı başlar, yavaşlar
		const float R = (6.f + 30.f * E) * S;
		const int32 N = 28;
		for (int32 k = 0; k < N; ++k)
		{
			const float A0 = k * 2.f * PI / N;
			const float A1 = (k + 1) * 2.f * PI / N;
			const FVector2D P0 = C.P + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R;
			const FVector2D P1 = C.P + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R;
			Line(P0, P1, 4.f * S, U(58, 42, 30, uint8(110 * (1.f - A))));
			Line(P0, P1, 2.5f * S, U(255, 214, 90, uint8(240 * (1.f - A))));
		}
		for (int32 k = 0; k < 7; ++k)
		{
			const float Ang = k * 2.f * PI / 7.f + C.T * 3.f;
			const FVector2D D(FMath::Cos(Ang), FMath::Sin(Ang));
			Disc(C.P + D * (R + 8.f * S * E), (3.5f - 2.5f * A) * S, U(226, 118, 94, uint8(230 * (1.f - A))), 8);
		}
	}
}

void ADBHUD::DrawWind()
{
	// Kamera takvime yaklaşırken (Ayarlar / Emeği Geçenler) soldan sağa geçen ince rüzgar çizgileri ve kâğıt kırıntıları
	const float Age = GM->Now - GM->WindTime;
	const float Life = 1.7f;
	if (Age < 0.f || Age > Life)
	{
		return;
	}
	const float Fade = FMath::Sin(PI * Age / Life);
	FRandomStream R(1234);
	for (int32 i = 0; i < 14; ++i)
	{
		const float Y0 = H * R.FRandRange(0.08f, 0.92f);
		const float Len = R.FRandRange(160.f, 320.f) * S;
		const float Speed = R.FRandRange(0.9f, 1.4f) * (W + Len) / Life;
		const float Delay = R.FRandRange(0.f, 0.5f);
		const float Head = -Len + Speed * FMath::Max(0.f, Age - Delay);
		const float Amp = R.FRandRange(4.f, 12.f) * S;
		const float Ph = R.FRandRange(0.f, 6.f);
		const int32 Seg = 10;
		FVector2D Prev;
		for (int32 k = 0; k <= Seg; ++k)
		{
			const float X = Head - Len * k / Seg;
			const FVector2D P(X, Y0 + FMath::Sin(X * 0.012f / S + Ph + Age * 3.f) * Amp);
			if (k > 0)
			{
				const float Tail = 1.f - (float)k / Seg;
				Line(Prev, P, (1.f + 2.f * Tail) * S, U(255, 255, 255, uint8(150 * Fade * Tail)));
			}
			Prev = P;
		}
	}
	// Kâğıt kırıntıları: dönerek savrulur
	for (int32 i = 0; i < 9; ++i)
	{
		const float Y0 = H * R.FRandRange(0.15f, 0.85f);
		const float Speed = R.FRandRange(0.7f, 1.1f) * (W + 100.f * S) / Life;
		const float X = -40.f * S + Speed * Age;
		const float Y = Y0 + FMath::Sin(Age * R.FRandRange(3.f, 6.f) + i) * 30.f * S - Age * 20.f * S;
		const float Rot = Age * R.FRandRange(4.f, 9.f) + i;
		const FVector2D C(X, Y);
		const FVector2D Ax(FMath::Cos(Rot) * 7.f * S, FMath::Sin(Rot) * 7.f * S);
		const FVector2D Ay(-FMath::Sin(Rot) * 4.f * S, FMath::Cos(Rot) * 4.f * S * FMath::Abs(FMath::Cos(Age * 7.f + i)));
		const FLinearColor Pc = U(250, 243, 228, uint8(230 * Fade));
		Tri(C - Ax - Ay, C + Ax - Ay, C + Ax + Ay, Pc);
		Tri(C - Ax - Ay, C + Ax + Ay, C - Ax + Ay, Pc);
	}
}
