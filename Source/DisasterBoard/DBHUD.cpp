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
	Disc(C, R + 2.f * S, Ink);
	Disc(C, R, D.Color);
	Txt(D.Icon, C.X, C.Y - R * 0.5f, R, Ink, 1);
}

void ADBHUD::ToolIcon(int32 Tool, const FVector2D& C, float R)
{
	const FDBToolDef& D = FDBContent::Get().Tools[Tool];
	const FBox2D B(C - FVector2D(R), C + FVector2D(R));
	Box(B.ExpandBy(2.f * S), Ink);
	Box(B, D.Color);
	Txt(D.Icon, C.X, C.Y - R * 0.45f, R * 0.9f, Paper, 1);
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
	const float PW = 700.f * S;
	const float X = W - 24.f * S - PW + (1.f - PanelSlide) * (PW + 30.f * S);
	return Rect(X, H * 0.40f - 345.f * S, PW, 690.f * S);
}

FBox2D ADBHUD::DrawerHandleRect() const
{
	const FBox2D P = PanelRect();
	return Rect(P.Min.X - 46.f * S, P.GetCenter().Y - 100.f * S, 48.f * S, 200.f * S);
}

FBox2D ADBHUD::BottomBarRect() const { return Rect(W * 0.5f - 380.f * S, H - 182.f * S, 760.f * S, 164.f * S); }
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
	if (GM->Phase == EDBPhase::Talk)
	{
		GM->AdvanceTalk();
		return true;
	}

	auto Dispatch = [this](FName Id, int32 Ix)
	{
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
		else if (Id == TEXT("Item")) GM->AddItem(Ix);
		else if (Id == TEXT("Draft")) GM->RemoveDraftAt(Ix);
		else if (Id == TEXT("Pack")) GM->PackBag();
		else if (Id == TEXT("Clear")) GM->ClearDraft();
		else if (Id == TEXT("Tool")) GM->SelectTool(Ix);
		else if (Id == TEXT("Bag")) GM->SelectBag(Ix);
		else if (Id == TEXT("Again")) GM->Restart(true);
		else if (Id == TEXT("Menu")) GM->Restart(false);
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

	switch (GM->Phase)
	{
	case EDBPhase::Menu:
	case EDBPhase::MenuPage:
		DrawMenuPages();
		if (DebugDot.X >= 0.f)
		{
			Disc(DebugDot, 7.f * S, Red);
		}
		return;
	case EDBPhase::ToBoard:
		return;
	default:
		break;
	}

	if (GM->Phase != EDBPhase::Result)
	{
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
	if (GM->Phase != EDBPhase::Talk && GM->Phase != EDBPhase::Result)
	{
		DrawTooltip();
		DrawSelectionCursor();
	}
	if (GM->Phase != EDBPhase::Result)
	{
		DrawToast();
	}
	DrawTalk();
	if (GM->Phase == EDBPhase::Result)
	{
		DrawResult();
	}
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
	Txt(TEXT("DisasterBoard"), CX, BandH + 4.f * U1, 12.f * U1, Ink, 1);
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
	float Y = BandH + 5.f * U1;
	if (bSettings)
	{
		Button(TEXT("Fullscreen"), -1, Rect(W * 0.15f, Y, W * 0.7f, 14.f * U1),
			FString::Printf(TEXT("Ekran: %s"), *GM->FullscreenLabel()), 6.5f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 18.f * U1;
		Button(TEXT("Quality"), -1, Rect(W * 0.15f, Y, W * 0.7f, 14.f * U1),
			FString::Printf(TEXT("Grafik: %s"), *GM->QualityLabel()), 6.5f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 19.f * U1;
		Txt(TEXT("Esc / sağ tık: geri   ·   F5: baştan oyna"), CX, Y, 4.6f * U1, DarkK, 1);
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
		float R = 21.f * S * FMath::Max(0.2f, Pop) * (bHov ? 1.18f : 1.f);
		if (!bSolved)
		{
			R *= 1.f + 0.06f * FMath::Sin(GM->Now * 6.f + p);
		}
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
}

void ADBHUD::DrawMoney()
{
	if (!GM->bPanelOpen && !GM->bPlayStarted)
	{
		return;
	}
	const FBox2D R = MoneyRect();
	if (GM->IsTalking(FDBContent::Get().PanelIntroC, 2))
	{
		Glow(R);
	}
	Card(R, Paper, DarkK);
	Blockers.Add(R);
	const FVector2D Coin(R.Min.X + 32.f * S, R.GetCenter().Y);
	Disc(Coin, 17.f * S, U(214, 160, 56));
	Disc(Coin, 12.f * S, U(240, 196, 86));
	Txt(TEXT("TL"), Coin.X, Coin.Y - 7.f * S, 13.f * S, U(120, 84, 30), 1);
	Txt(TEXT("Bütçe"), R.Min.X + 60.f * S, R.Min.Y + 15.f * S, 22.f * S, DarkK);
	Txt(FString::Printf(TEXT("%d TL"), GM->Money), R.Max.X - 18.f * S, R.Min.Y + 9.f * S, 34.f * S, Ink, 2);
}

void ADBHUD::DrawScannerAndA()
{
	const FDBContent& C = FDBContent::Get();
	Blockers.Add(Rect(0.f, H - 220.f * S, 290.f * S, 220.f * S));
	Portrait(0, FVector2D(94.f * S, H - 122.f * S), 64.f * S);

	const FBox2D R = ScannerRect();
	const bool bCan = GM->Phase == EDBPhase::WaitScan;
	if (bCan || GM->IsTalking(C.IntroA, 2))
	{
		Glow(R);
	}
	const bool bHov = bCan && Hover(R);
	const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -4.f * S)) : R;
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
	if (GM->IsTalking(C.PanelIntroC, 1))
	{
		Glow(Rect(X, Y, 4.f * 112.f * S + 3.f * 10.f * S, 112.f * S));
	}
	for (int32 t = 0; t < C.Tools.Num() && t < 4; ++t)
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

	// Sırt çantası: hazırlanan çantalar
	const float BX = X + 4.f * 122.f * S + 14.f * S;
	Txt(TEXT("Sırt çantası"), BX, BR.Min.Y + 10.f * S, 19.f * S, DarkK);
	const FBox2D Pack = Rect(BX, Y - 2.f * S, 200.f * S, 116.f * S);
	if (GM->IsTalking(C.PanelIntroC, 3))
	{
		Glow(Pack);
	}
	Frame(Rect(BX + 128.f * S, Y - 20.f * S, 52.f * S, 22.f * S), Ink, 5.f * S);
	Card(Pack, U(206, 116, 88), Ink, 5.f);
	for (int32 b = 0; b < GM->MaxBags && b < 4; ++b)
	{
		const FBox2D R = Rect(BX + 12.f * S + (b % 2) * 92.f * S, Y + 6.f * S + (b / 2) * 54.f * S, 84.f * S, 48.f * S);
		if (!GM->Bags.IsValidIndex(b))
		{
			Box(R, U(255, 255, 255, 50));
			Frame(R, U(110, 60, 40, 140), 2.f * S);
			continue;
		}
		const FDBBag& Bag = GM->Bags[b];
		if (Bag.bUsed)
		{
			Box(R, U(44, 34, 30));
			continue;
		}
		const bool bSel = GM->SelBag == b;
		const bool bHov = bPlay && Hover(R);
		const FBox2D RR = (bSel || bHov) ? R.ShiftBy(FVector2D(0.f, -3.f * S)) : R;
		Box(RR, bSel ? U(255, 248, 220) : Paper);
		KitBadge(Bag.Kit, Rect(RR.Min.X, RR.Min.Y, RR.GetSize().X, 20.f * S), 13.f * S);
		for (int32 k = 0; k < Bag.Items.Num(); ++k)
		{
			ItemIcon(Bag.Items[k], FVector2D(RR.Min.X + 13.f * S + k * 19.5f * S, RR.Min.Y + 34.f * S), 8.f * S);
		}
		Frame(RR, bSel ? Teal : Ink, bSel ? 4.f * S : 2.f * S);
		if (bPlay)
		{
			Hit(TEXT("Bag"), b, RR);
		}
	}
}

void ADBHUD::DrawDrawerHandle()
{
	const FBox2D R = DrawerHandleRect();
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

void ADBHUD::DrawPanel()
{
	const FDBContent& C = FDBContent::Get();
	const FBox2D P = PanelRect();
	Card(P, U(244, 230, 204), DarkK, 10.f);
	Blockers.Add(P);
	const bool bEdit = GM->CanEditBags() && PanelSlide > 0.97f;
	const float X0 = P.Min.X + 24.f * S;
	const float Y0 = P.Min.Y;

	Txt(TEXT("Hazırlık Masası"), X0, Y0 + 16.f * S, 34.f * S, Ink);
	const FBox2D XR = Rect(P.Max.X - 68.f * S, Y0 + 14.f * S, 50.f * S, 50.f * S);
	if (GM->IsTalking(C.BagIntroB, 5))
	{
		Glow(XR);
	}
	Button(TEXT("PanelClose"), -1, XR, TEXT("X"), 26.f * S, Coral, Ink, Paper, bEdit);
	Txt(TEXT("Önce çantanın türünü seç, sonra eşyalara tıkla."), X0, Y0 + 62.f * S, 17.f * S, DarkK);

	// Çanta türü sekmeleri
	const float TabY = Y0 + 92.f * S;
	if (GM->IsTalking(C.BagIntroB, 2))
	{
		Glow(Rect(X0, TabY, 3.f * 208.f * S + 2.f * 14.f * S, 44.f * S));
	}
	for (int32 k = 0; k < C.Kits.Num() && k < 3; ++k)
	{
		const FBox2D R = Rect(X0 + k * 222.f * S, TabY, 208.f * S, 44.f * S);
		const bool bSel = GM->DraftKit == k;
		Button(TEXT("Kit"), k, R, C.Kits[k].Short, 19.f * S, bSel ? C.Kits[k].Color : CardC, bSel ? Ink : DarkK, bSel ? Paper : Ink, bEdit);
	}

	// Eşyalar 4×3
	const float GY = Y0 + 148.f * S;
	if (GM->IsTalking(C.BagIntroB, 3))
	{
		Glow(Rect(X0, GY, 4.f * 150.f * S + 3.f * 12.f * S, 3.f * 92.f * S + 2.f * 8.f * S));
	}
	for (int32 i = 0; i < C.Items.Num() && i < 12; ++i)
	{
		const FBox2D R = Rect(X0 + (i % 4) * 162.f * S, GY + (i / 4) * 100.f * S, 150.f * S, 92.f * S);
		const bool bIn = GM->Draft.Contains(i);
		const bool bHov = bEdit && Hover(R);
		const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -4.f * S)) : R;
		Card(RR, bIn ? U(226, 240, 214) : Paper, bIn ? Green : DarkK, bHov ? 7.f : 4.f);
		ItemIcon(i, FVector2D(RR.GetCenter().X, RR.Min.Y + 34.f * S), 22.f * S);
		Txt(C.Items[i].Name, RR.GetCenter().X, RR.Min.Y + 62.f * S, 16.f * S, Ink, 1);
		if (bIn)
		{
			TickMark(FVector2D(RR.Max.X - 18.f * S, RR.Min.Y + 18.f * S), 20.f * S, Green);
		}
		if (bEdit)
		{
			Hit(TEXT("Item"), i, RR);
		}
	}

	// Hazırlanan çanta
	const float BY = GY + 3.f * 100.f * S + 4.f * S;
	const FDBKitDef& Kit = C.Kits[FMath::Clamp(GM->DraftKit, 0, C.Kits.Num() - 1)];
	Txt(FString::Printf(TEXT("%s (%d/%d)"), *Kit.Name, GM->Draft.Num(), GM->BagCapacity), X0, BY, 20.f * S, Ink);
	for (int32 k = 0; k < GM->BagCapacity && k < 4; ++k)
	{
		const FBox2D R = Rect(X0 + k * 96.f * S, BY + 30.f * S, 86.f * S, 86.f * S);
		if (GM->Draft.IsValidIndex(k))
		{
			const bool bHov = bEdit && Hover(R);
			Card(R, Paper, bHov ? Coral : DarkK, 3.f);
			ItemIcon(GM->Draft[k], FVector2D(R.GetCenter().X, R.Min.Y + 32.f * S), 20.f * S);
			Txt(C.Items[GM->Draft[k]].Name, R.GetCenter().X, R.Min.Y + 60.f * S, 13.f * S, Ink, 1);
			if (bHov)
			{
				Cross(FVector2D(R.Max.X - 14.f * S, R.Min.Y + 14.f * S), 16.f * S, Coral);
			}
			if (bEdit)
			{
				Hit(TEXT("Draft"), k, R);
			}
		}
		else
		{
			Box(R, WithA(Kit.Color, 0.18f));
			Frame(R, WithA(Kit.Color, 0.7f), 2.f * S);
		}
	}
	const FBox2D PackR = Rect(X0 + 4.f * 96.f * S + 16.f * S, BY + 30.f * S, 252.f * S, 50.f * S);
	if (GM->IsTalking(C.BagIntroB, 4))
	{
		Glow(PackR);
	}
	Button(TEXT("Pack"), -1, PackR, TEXT("Çantayı Hazırla"), 22.f * S, Green, Ink, Paper, bEdit && GM->Draft.Num() > 0);
	Button(TEXT("Clear"), -1, Rect(PackR.Min.X, BY + 86.f * S, 252.f * S, 30.f * S), TEXT("Boşalt"), 16.f * S,
		CardC, DarkK, Ink, bEdit && GM->Draft.Num() > 0);

	// Alt: hazır çantalar (sol) ve not kağıdı (sağ, panelin altına sarkar)
	const float AY = BY + 30.f * S + 86.f * S + 16.f * S;
	const float NoteW = 460.f * S;
	Txt(TEXT("Hazır çantalar"), X0, AY, 18.f * S, Ink);
	float LY = AY + 26.f * S;
	const float ListW = P.Max.X - 24.f * S - NoteW - 16.f * S - X0;
	if (GM->Bags.Num() == 0)
	{
		Txt(TEXT("Henüz yok."), X0, LY, 15.f * S, DarkK);
	}
	for (int32 b = 0; b < GM->Bags.Num(); ++b)
	{
		const FDBBag& Bag = GM->Bags[b];
		const FString L = FString::Printf(TEXT("%d. %s%s"), b + 1, *C.Kits[Bag.Kit].Short, Bag.bUsed ? TEXT(" (gönderildi)") : TEXT(""));
		Txt(L, X0, LY, 15.f * S, Bag.bUsed ? U(140, 128, 110) : Ink);
		LY += 19.f * S;
	}

	const FBox2D NR = Rect(P.Max.X - 24.f * S - NoteW, AY - 6.f * S, NoteW, 350.f * S);
	Blockers.Add(NR);
	if (GM->IsTalking(C.BagIntroB, 1))
	{
		Glow(NR);
	}
	Box(NR.ShiftBy(FVector2D(5.f, 7.f) * S), ShadowC);
	Box(NR, NoteY);
	Box(Rect(NR.GetCenter().X - 50.f * S, NR.Min.Y - 12.f * S, 100.f * S, 26.f * S), U(255, 255, 255, 150));
	Txt(TEXT("NOT"), NR.Min.X + 16.f * S, NR.Min.Y + 12.f * S, 22.f * S, Ink);
	Disc(FVector2D(NR.Max.X - 196.f * S, NR.Min.Y + 25.f * S), 5.f * S, Coral);
	Txt(TEXT("çanta"), NR.Max.X - 186.f * S, NR.Min.Y + 16.f * S, 14.f * S, DarkK);
	Disc(FVector2D(NR.Max.X - 128.f * S, NR.Min.Y + 25.f * S), 5.f * S, Teal);
	Txt(TEXT("şehir önlemi"), NR.Max.X - 118.f * S, NR.Min.Y + 16.f * S, 14.f * S, DarkK);
	float NY = NR.Min.Y + 46.f * S;
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (!GM->Revealed[p])
		{
			continue;
		}
		const FDBProblemDef& Pr = C.Problems[p];
		const bool bS = GM->Solved[p];
		const FVector2D Dot(NR.Min.X + 18.f * S, NY + 8.f * S);
		if (bS)
		{
			TickMark(Dot, 16.f * S, Green);
			Txt(C.Buildings[Pr.Building].Name + TEXT(": hazır"), NR.Min.X + 32.f * S, NY, 14.f * S, U(140, 128, 110));
			NY += 19.f * S;
			continue;
		}
		Disc(Dot, 5.f * S, Pr.Tool >= 0 ? Teal : Coral);
		const FString L = FString::Printf(TEXT("[%s] %s: %s"), *Pr.Hazard, *C.Buildings[Pr.Building].Name, *Pr.Short);
		NY = Para(L, NR.Min.X + 32.f * S, NY, 14.f * S, NR.GetSize().X - 46.f * S, Ink, 1.16f, -1, NR.Max.Y - 6.f * S) + 5.f * S;
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
		BR = Rect(40.f * S, H - 500.f * S, 660.f * S, 200.f * S);
		TailA = FVector2D(110.f * S, BR.Max.Y - 2.f);
		TailB = FVector2D(170.f * S, BR.Max.Y - 2.f);
		TailTip = FVector2D(104.f * S, H - 196.f * S);
	}
	else if (Who == 1)
	{
		const FBox2D P = PanelRect();
		const FVector2D PC(P.Min.X - 96.f * S, H - 330.f * S);
		BR = Rect(PC.X - 80.f * S - 640.f * S, H - 590.f * S, 660.f * S, 200.f * S);
		TailA = FVector2D(BR.Max.X - 120.f * S, BR.Max.Y - 2.f);
		TailB = FVector2D(BR.Max.X - 60.f * S, BR.Max.Y - 2.f);
		TailTip = PC + FVector2D(-40.f * S, -50.f * S);
		Portrait(1, PC, 74.f * S);
	}
	else
	{
		const FVector2D PC(W * 0.5f - 340.f * S, H - 276.f * S);
		BR = Rect(W * 0.5f - 250.f * S, H - 430.f * S, 660.f * S, 190.f * S);
		TailA = FVector2D(BR.Min.X + 2.f, BR.Max.Y - 90.f * S);
		TailB = FVector2D(BR.Min.X + 2.f, BR.Max.Y - 40.f * S);
		TailTip = PC + FVector2D(60.f * S, -20.f * S);
		Portrait(2, PC, 70.f * S);
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
	const float Y0 = 92.f * S;
	const FBox2D R = Rect(W * 0.5f - (TW + 48.f * S) * 0.5f, Y0, TW + 48.f * S, Lines.Num() * Px * 1.25f + 26.f * S);
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
