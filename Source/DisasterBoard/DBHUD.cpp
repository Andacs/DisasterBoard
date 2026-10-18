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
#include "Fonts/CompositeFont.h"

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

// El çizimi çerçeveler (Andaç, 17 Ekim ~22.30: "aşırı yapay duruyor"): Content/Arayuz/ui_*.png 9 dilime bölünüp
// her boyuta esnetilir, dolgu rengine boyanır. Dosya yoksa eski düz kutu çizilir.
namespace
{
	constexpr float UiSrcBorder = 72.f;   // dokuda köşe bölgesi (8 px boşluk + yuvarlak köşe + 14 px kontur)
	const FLinearColor PanelBase = U(246, 238, 220);
	const FLinearColor DugmeBase = U(242, 242, 242);
	FLinearColor TintFrom(const FLinearColor& Fill, const FLinearColor& Base)
	{
		return FLinearColor(FMath::Min(1.f, Fill.R / Base.R), FMath::Min(1.f, Fill.G / Base.G), FMath::Min(1.f, Fill.B / Base.B), Fill.A);
	}
}

bool ADBHUD::Nine(UTexture2D* T, const FBox2D& R, float DstB, const FLinearColor& Tint)
{
	if (!T || !T->GetResource() || R.GetSize().X <= 1.f || R.GetSize().Y <= 1.f)
	{
		return false;
	}
	const float D = FMath::Min(DstB, 0.5f * FMath::Min(R.GetSize().X, R.GetSize().Y));
	const float Bu = UiSrcBorder / T->GetSizeX(), Bv = UiSrcBorder / T->GetSizeY();
	const float Xs[4] = { R.Min.X, R.Min.X + D, R.Max.X - D, R.Max.X };
	const float Ys[4] = { R.Min.Y, R.Min.Y + D, R.Max.Y - D, R.Max.Y };
	const float Us[4] = { 0.f, Bu, 1.f - Bu, 1.f };
	const float Vs[4] = { 0.f, Bv, 1.f - Bv, 1.f };
	for (int32 j = 0; j < 3; ++j)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector2D Sz(Xs[i + 1] - Xs[i], Ys[j + 1] - Ys[j]);
			if (Sz.X <= 0.01f || Sz.Y <= 0.01f)
			{
				continue;
			}
			FCanvasTileItem It(FVector2D(Xs[i], Ys[j]), T->GetResource(), Sz, FVector2D(Us[i], Vs[j]), FVector2D(Us[i + 1], Vs[j + 1]), Gn(Tint));
			It.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(It);
		}
	}
	return true;
}

void ADBHUD::Card(const FBox2D& R, const FLinearColor& Fill, const FLinearColor& Edge, float Shadow)
{
	const bool bNote = Fill.Equals(NoteY, 0.02f);
	if (UTexture2D* T = LoadPng(bNote ? TEXT("Arayuz/ui_not.png") : TEXT("Arayuz/ui_panel.png")))
	{
		const float D = FMath::Clamp(FMath::Min(R.GetSize().X, R.GetSize().Y) * 0.3f, 6.f * S, 16.f * S);
		Nine(T, R.ShiftBy(FVector2D(Shadow * 0.6f, Shadow) * S), D, FLinearColor(0.f, 0.f, 0.f, 0.22f * Fill.A));
		Nine(T, R, D, bNote ? FLinearColor(1.f, 1.f, 1.f, Fill.A) : TintFrom(Fill, PanelBase));
		return;
	}
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
	const float Sc = Px / FontBaseH * FontScaleFix;
	float XL = 0.f, YL = 0.f;
	Canvas->TextSize(PickFont(Px), T, XL, YL, Sc, Sc);
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
	FCanvasTextItem It(FVector2D(X, Y - Px * FontBaselineFix), FText::FromString(T), PickFont(Px), Gn(C));
	It.Scale = FVector2D(Px / FontBaseH * FontScaleFix);
	It.BlendMode = SE_BLEND_Translucent;
	if (ColorGain != 1.f && It.Font == Font.Get())   // kalın (Baloo 2 Bold) zaten dolgun: kontur tombullaştırıyordu
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
	FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -3.f * S)) : R;
	// Basılınca kısa çökme + geri yaylanma (revizyon 7: daha güçlü tıklama geri bildirimi)
	const float PA = FxNow - PressTime;
	if (Id == PressId && Index == PressIndex && PA >= 0.f && PA < 0.28f)
	{
		const float K = 1.f - 0.1f * FMath::Sin(PA / 0.28f * PI) * FMath::Exp(-PA * 4.f) + 0.03f * FMath::Sin(PA / 0.28f * 2.f * PI);
		const FVector2D C0 = RR.GetCenter(), Hs = RR.GetExtent() * K;
		RR = FBox2D(C0 - Hs, C0 + Hs);
	}
	FLinearColor F = Fill;
	if (bHov)
	{
		F = FLinearColor::LerpUsingHSV(Fill, FLinearColor::White, 0.18f);
		Frame(RR.ExpandBy(4.f * S), U(255, 214, 90, 150), 3.f * S);   // üzerine gelince sıcak çerçeve
	}
	if (!bEnabled)
	{
		F.A *= 0.45f;
	}
	if (UTexture2D* BT = LoadPng(TEXT("Arayuz/ui_dugme.png")))
	{
		const float D = FMath::Min(20.f * S, RR.GetSize().Y * 0.42f);
		const float Sh = bHov ? 6.f : 3.5f;
		Nine(BT, RR.ShiftBy(FVector2D(Sh * 0.5f, Sh) * S), D, FLinearColor(0.f, 0.f, 0.f, 0.22f * F.A));
		Nine(BT, RR, D, TintFrom(F, DugmeBase));
		RR = RR.ShiftBy(FVector2D(0.f, -2.f * S));   // alt kalınlık şeridi: yazı görsel ortada dursun
	}
	else
	{
		Card(RR, F, bEnabled ? Edge : WithA(Edge, 0.5f), bHov ? 7.f : 4.f);
	}
	if (bSelected)
	{
		Frame(RR.ExpandBy(4.f * S), Teal, 4.f * S);
	}
	bBoldText = true;
	Txt(Label, RR.GetCenter().X, RR.GetCenter().Y - Px * 0.52f, Px, bEnabled ? TextC : WithA(TextC, 0.5f), 1);
	bBoldText = false;
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
	const float PW = 800.f * S;   // Andaç (17 Ekim ~21.30): ikonlar ve yazılar büyüdü
	const float X = W - 24.f * S - PW + (1.f - PanelSlide) * (PW + 30.f * S);
	return Rect(X, 20.f * S, PW, H - 216.f * S);
}

FBox2D ADBHUD::DrawerHandleRect() const
{
	const FBox2D P = PanelRect();
	return Rect(P.Min.X - 58.f * S, P.GetCenter().Y - 100.f * S, 60.f * S, 200.f * S);
}

FBox2D ADBHUD::BottomBarRect() const { return Rect(W * 0.5f - 325.f * S, H - 192.f * S, 650.f * S, 174.f * S); }
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
	if (GM->bMenuConfirm)
	{
		for (int32 i = Hits.Num() - 1; i >= 0; --i)
		{
			if ((Hits[i].Id == TEXT("MenuYes") || Hits[i].Id == TEXT("MenuNo") || Hits[i].Id == TEXT("MenuCal")) && Hits[i].R.IsInside(P))
			{
				GM->PlaySfx(EDBSfx::Click, 0.9f);
				if (Hits[i].Id == TEXT("MenuYes")) GM->Restart(false);
				else if (Hits[i].Id == TEXT("MenuCal")) GM->PauseToCalendar();
				else GM->bMenuConfirm = false;
				return true;
			}
		}
		return true;
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
		// Etkileşimli satır: yazı bittiyse yalnızca gösterilen tuş geçer (oyuncu işlemi kendisi yapar)
		const int32 Do = GM->DoLineTarget();
		if (Do == 4)
		{
			for (int32 Dir : { -1, 1 })
			{
				const FBox2D& T = Dir < 0 ? TgTurnL : TgTurnR;
				if (T.bIsValid && T.ExpandBy(10.f * S).IsInside(P))
				{
					GM->PlaySfx(EDBSfx::Click, 0.9f);
					AddClickFx(P);
					GM->TurnBoard(Dir);
					return true;
				}
			}
			if (GM->IsLineComplete())
			{
				DoMissTime = FxNow;
				GM->PlaySfx(EDBSfx::Bad, 0.35f, 0.f, 1.4f);
				return true;
			}
		}
		if (Do != 0 && Do != 4 && GM->IsLineComplete())
		{
			const FBox2D T = DoTargetRect();
			if (!(T.bIsValid && T.ExpandBy(10.f * S).IsInside(P)))
			{
				DoMissTime = FxNow;
				GM->PlaySfx(EDBSfx::Bad, 0.35f, 0.f, 1.4f);
				return true;
			}
			GM->PlaySfx(EDBSfx::Click, 0.9f);
			AddClickFx(P);
			GM->AdvanceTalk();
			if (Do == 1 && GM->Phase == EDBPhase::WaitScan)
			{
				GM->StartScan();   // "tarayıcıya bas" → bastı: tarama hemen başlar
			}
			return true;
		}
		GM->PlaySfx(EDBSfx::Click, 0.55f, 0.f, 1.15f);
		GM->AdvanceTalk();
		return true;
	}

	auto Dispatch = [this](FName Id, int32 Ix)
	{
		GM->PlaySfx(EDBSfx::Click, 0.9f, 0.f, FMath::FRandRange(0.94f, 1.06f));
		PressId = Id;
		PressIndex = Ix;
		PressTime = FxNow;
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
		else if (Id == TEXT("Station"))
		{
			const int32 NB = GM->BagPersonal.Num(), KB = GM->BagKit;
			GM->StationClick(Ix / 10, Ix % 10);
			if (Ix / 10 == 0 ? GM->BagKit != KB : GM->BagPersonal.Num() != NB)
			{
				Flies.Add({ Ix / 10 == 0 ? GM->BagKit : GM->BagPersonal.Last(), Ix / 10 == 0, Mouse, FxNow });
			}
		}
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
		else if (Id == TEXT("Vfx")) GM->CycleVfx();
		else if (Id == TEXT("Menu")) GM->Restart(false);
		else if (Id == TEXT("Turn")) GM->TurnBoard(Ix);
		else if (Id == TEXT("Marker")) GM->ClickBuilding(Ix);
		else if (Id == TEXT("Deliver")) GM->bBeltFast = true;
		else if (Id == TEXT("MenuAsk")) GM->AskMenu();
		else if (Id == TEXT("MenuYes")) GM->Restart(false);
		else if (Id == TEXT("MenuNo")) GM->bMenuConfirm = false;
		else if (Id == TEXT("MenuCal")) GM->PauseToCalendar();
		else if (Id == TEXT("TasksToggle")) bTasksOpen = !bTasksOpen;
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
		Font = MakeRuntimeFont(TEXT("Arayuz.ttf"));
		FontBold = MakeRuntimeFont(TEXT("Arayuz_Kalin.ttf"));
		float XL = 0.f, YL = 0.f;
		Canvas->TextSize(Font, TEXT("Ağ"), XL, YL);
		FontBaseH = FMath::Max(1.f, YL);
		if (Font != GEngine->GetLargeFont())
		{
			// Baloo 2 (upem 1000): satır 1,602 em, üst 1,078, büyük harf 0,602. Roboto'da satır 1,172, üst 0,928,
			// büyük harf 0,711. Büyük harf Roboto'nunkinin %96'sı olsun (em = 0,968 Px), taban çizgisi aynı yerde.
			FontScaleFix = 1.602f * 0.968f;
			FontBaselineFix = 1.078f * 0.968f - 0.928f / 1.172f;
		}
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
		DrawMeters();
		if (GM->Phase == EDBPhase::Play)
		{
			DrawTasks();
		}
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
		else if (GM->BagsDoneHint())
		{
			// Bütün çantalar teslim ama masa açık kaldı: Asil gelir, kulpu gösterir, nasıl kapatılacağını söyler
			DrawGuide(1, FVector2D(PanelRect().Min.X - 110.f * S, H - 228.f * S), 180.f * S);
			DrawBagsDoneHint();
		}
	}
	DrawTalk();
	DrawDoHint();
	if (GM->Mini != EDBMini::None)
	{
		DrawMini();
	}
	if (GM->Phase != EDBPhase::Result)
	{
		DrawMenuButton();
	}
	DrawEventFx();
	if (GM->Phase != EDBPhase::Result)
	{
		DrawPhotoToast();
	}
	if (GM->bDebugPortraits)
	{
		DrawPortraitGallery();
	}
	if (GM->bMenuConfirm)
	{
		DrawMenuConfirm();
	}
	if (GM->Phase == EDBPhase::Result)
	{
		DrawResult();
	}
	DrawAchievementPopup();
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
	S = 2.2f * Size.Y / 744.f;   // sayfa dokusu büyüdükçe çizim de büyür (tasarım 744 yüksekliğe göre)
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
	Txt(TEXT("EKİM 2026"), CX, BandH * 0.5f - 1.5f * U1, 8.f * U1, Paper, 1);   // takvimin spiral teli üst %7'yi kapatıyor
	if (UTexture2D* Logo = LoadPng(TEXT("Arayuz/logo.png")))
	{
		DrawTex(Logo, Rect(CX - 30.f * U1, BandH + 1.5f * U1, 60.f * U1, 15.f * U1), 0.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Txt(TEXT("Tetkik"), CX, BandH + 4.f * U1, 12.f * U1, Ink, 1);   // oyunun adı (Andaç, 18 Ekim); proje adı DisasterBoard kalır
	}
	// Andaç (17 Ekim ~19.30): "Afetlere hazır mısın?" alt başlığı kalktı; duraklatılmış oyunda "Oyuna devam et"
	const TCHAR* Labels[] = { GM->bPausedGame ? TEXT("Oyuna devam et") : TEXT("Oyna"), TEXT("Ayarlar"), TEXT("Emeği Geçenler"), TEXT("Çıkış") };
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
	Txt(bSettings ? TEXT("AYARLAR") : TEXT("EMEĞİ GEÇENLER"), CX, BandH * 0.5f - 1.5f * U1, 8.f * U1, Paper, 1);
	float Y = BandH + 3.f * U1;
	if (bSettings)
	{
		Button(TEXT("Fullscreen"), -1, Rect(W * 0.15f, Y, W * 0.7f, 12.f * U1),
			FString::Printf(TEXT("Ekran: %s"), *GM->FullscreenLabel()), 6.f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 14.5f * U1;
		Button(TEXT("Quality"), -1, Rect(W * 0.15f, Y, W * 0.7f, 12.f * U1),
			FString::Printf(TEXT("Grafik: %s"), *GM->QualityLabel()), 6.f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 14.5f * U1;
		Button(TEXT("Vfx"), -1, Rect(W * 0.15f, Y, W * 0.7f, 12.f * U1),
			FString::Printf(TEXT("Efektler: %s"), *GM->VfxLabel()), 6.f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 14.5f * U1;
		Button(TEXT("Music"), -1, Rect(W * 0.15f, Y, W * 0.7f, 12.f * U1),
			FString::Printf(TEXT("Müzik: %s"), *GM->MusicLabel()), 6.f * U1, CardC, DarkK, Ink, bEnabled);
		Y += 14.f * U1;
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
		float R = 24.f * S * FMath::Max(0.2f, Pop) * (bHov ? 1.18f : 1.f) * (bCur ? 1.25f : 1.f);
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
			DrawTex(Mark, Rect(Sp.X - R * 1.25f, Sp.Y - R * 1.25f, R * 2.5f, R * 2.5f), 0.f, FVector2D(0.5f, 0.5f));
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
		if (GM->Phase == EDBPhase::Play)
		{
			Hit(TEXT("Marker"), B, FBox2D(Sp - FVector2D(R * 1.3f, R * 1.2f), Sp + FVector2D(R * 1.3f, R * 1.6f)));
		}
		if (!bSolved)
		{
			// İstek açıkça: hangi çanta / hangi önlem (ekip: "istekler net belirtilecek")
			const FDBProblemDef& Pr = C.Problems[p];
			const bool bKit = Pr.Kit >= 0;
			const FString Want = bKit ? C.Kits[Pr.Kit].Name : C.Tools[Pr.Tool].Name;
			const FLinearColor WantC = bKit ? C.Kits[Pr.Kit].Color : C.Tools[Pr.Tool].Color;
			const float Px = 19.f * S;
			const float LW = Measure(Want, Px).X + 20.f * S;
			const FBox2D L = Rect(Sp.X + R * 1.3f + 6.f * S, Sp.Y - 14.f * S, LW, 28.f * S);
			Box(L.ShiftBy(FVector2D(2.f, 3.f) * S), ShadowC);
			Box(L, WantC);
			Frame(L, Ink, 1.5f * S);
			Txt(Want, L.Min.X + 10.f * S, L.Min.Y + 3.f * S, Px, Paper);
			if (GM->Phase == EDBPhase::Play)
			{
				Hit(TEXT("Marker"), B, L);
			}
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
	if (ShownMoney < 0.f || LastMoney == INT_MIN)
	{
		ShownMoney = GM->Money;
		LastMoney = GM->Money;
	}
	if (GM->Money != LastMoney)
	{
		MoneyFxs.Add({ GM->Money - LastMoney, FxNow });   // her değişim kendi uçan yazısı
		LastMoney = GM->Money;
	}
	ShownMoney = FMath::FInterpConstantTo(ShownMoney, (float)GM->Money, GetWorld()->GetDeltaSeconds(), 90.f);
	const bool bUp = ShownMoney < GM->Money - 0.5f, bDown = ShownMoney > GM->Money + 0.5f;
	Txt(FString::Printf(TEXT("%d TL"), FMath::RoundToInt(ShownMoney)), R.Max.X - 18.f * S, R.Min.Y + 9.f * S, 34.f * S, bUp ? Green : (bDown ? Red : Ink), 2);
	MoneyFxs.RemoveAll([this](const FMoneyFx& M) { return FxNow - M.T > 1.4f; });
	for (int32 k = 0; k < MoneyFxs.Num(); ++k)
	{
		const FMoneyFx& M = MoneyFxs[k];
		const float A = (FxNow - M.T) / 1.4f;
		const float Y = R.Max.Y + 6.f * S + A * 46.f * S + k * 30.f * S;   // aynı anda gelenler alt alta
		Txt(FString::Printf(TEXT("%s%d TL"), M.Delta > 0 ? TEXT("+") : TEXT(""), M.Delta), R.Max.X - 18.f * S, Y, 30.f * S,
			WithA(M.Delta > 0 ? Green : Red, 1.f - A * A), 2);
	}
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
		// Radar tarayıcı (Andaç: daha havalı): anten + yanıp sönen ışık, yuvarlak radar ekranı (dönen tarama, bip'ler)
		const bool bScanning = GM->Phase == EDBPhase::Scanning;
		const FVector2D Ant(RR.Max.X - 16.f * S, RR.Min.Y - 42.f * S);
		Line(FVector2D(RR.Max.X - 22.f * S, RR.Min.Y + 6.f * S), Ant, 4.f * S, Ink);
		const bool bBlink = FMath::Fmod(GM->Now * (bScanning ? 6.f : 1.5f), 1.f) < 0.5f;
		if (bBlink)
		{
			Disc(Ant, 12.f * S, U(255, 90, 70, 70));
		}
		Disc(Ant, 6.f * S, bBlink ? U(255, 96, 72) : U(140, 50, 40));
		RoundCard(RR, U(58, 74, 88), Ink, 18.f * S, 5.f);
		RoundBox(Rect(RR.Min.X + 8.f * S, RR.Min.Y + 6.f * S, RR.GetSize().X - 16.f * S, 8.f * S), 4.f * S, U(255, 255, 255, 40));
		DrawRadar(FVector2D(RR.GetCenter().X, RR.Min.Y + 46.f * S), 33.f * S, bScanning);
		const FLinearColor Knobs[] = { Coral, U(246, 196, 64), U(120, 220, 170) };
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector2D Kc(RR.Min.X + 20.f * S + i * 22.f * S, RR.Min.Y + 98.f * S);
			Disc(Kc, 8.f * S, Ink);
			Disc(Kc, 6.f * S, (bScanning && FMath::Fmod(GM->Now * 4.f + i * 0.33f, 1.f) < 0.5f) ? U(255, 255, 220) : Knobs[i]);
		}
		Txt(TEXT("TARAYICI"), RR.GetCenter().X, RR.Max.Y - 32.f * S, 15.f * S, U(170, 255, 210), 1);
	}
	// Taranırken: cihazdan ekrana yayılan radar halkaları + "TARANIYOR" ilerlemesi
	if (GM->Phase == EDBPhase::Scanning)
	{
		const FVector2D Cc(RR.GetCenter().X, RR.Min.Y + 46.f * S);
		for (int32 k = 0; k < 3; ++k)
		{
			const float Ph = FMath::Fmod(GM->PhaseTime * 0.9f + k / 3.f, 1.f);
			const float Rr = 40.f * S + Ph * 900.f * S;
			const int32 Seg = 72;
			for (int32 s = 0; s < Seg; ++s)
			{
				const float A0 = 2.f * PI * s / Seg, A1 = 2.f * PI * (s + 1) / Seg;
				Line(Cc + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * Rr, Cc + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * Rr, (3.f - 2.f * Ph) * S,
					U(120, 255, 190, uint8(150 * (1.f - Ph))));
			}
		}
		const float Pct = FMath::Clamp(GM->PhaseTime / FMath::Max(0.1f, GM->ScanSeconds), 0.f, 1.f);
		const FBox2D Lb = Rect(RR.GetCenter().X - 110.f * S, RR.Min.Y - 104.f * S, 220.f * S, 50.f * S);
		RoundCard(Lb, U(20, 40, 36, 220), U(120, 255, 190), 12.f * S, 3.f);
		Txt(FString::Printf(TEXT("TARANIYOR %%%d"), FMath::RoundToInt(Pct * 100.f)), Lb.GetCenter().X, Lb.Min.Y + 5.f * S, 20.f * S, U(170, 255, 210), 1);
		const FBox2D Bar = Rect(Lb.Min.X + 14.f * S, Lb.Max.Y - 14.f * S, Lb.GetSize().X - 28.f * S, 6.f * S);
		RoundBox(Bar, 3.f * S, U(60, 90, 80));
		RoundBox(Rect(Bar.Min.X, Bar.Min.Y, Bar.GetSize().X * Pct, Bar.GetSize().Y), 3.f * S, U(120, 255, 190));
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
	const float Y = BR.Min.Y + 36.f * S;
	Txt(TEXT("Şehir önlemleri"), X, BR.Min.Y + 7.f * S, 21.f * S, DarkK);
	TgTools = Rect(X, Y, 5.f * 112.f * S + 4.f * 10.f * S, 128.f * S);
	if (GM->IsTalking(C.PanelIntroC, 1))
	{
		Glow(TgTools);
	}
	for (int32 t = 0; t < C.Tools.Num() && t < 5; ++t)
	{
		const FDBToolDef& T = C.Tools[t];
		const FBox2D R = Rect(X + t * 122.f * S, Y, 112.f * S, 128.f * S);
		const bool bSel = GM->SelTool == t;
		const bool bHov = bPlay && Hover(R);
		const FBox2D RR = (bSel || bHov) ? R.ShiftBy(FVector2D(0.f, -5.f * S)) : R;
		Card(RR, bSel ? U(255, 248, 220) : Paper, bSel ? Teal : DarkK, 4.f);
		if (bSel)
		{
			Frame(RR.ExpandBy(3.f * S), Teal, 4.f * S);
		}
		// Revizyon 15: ikonlar büyük ve okunur
		// Seçili önlem süzülür, üzerine gelinen büyüyüp kıpırdar
		const float Bob = bSel ? FMath::Sin(FxNow * 5.f) * 4.f * S : 0.f;
		const float Grow = bHov ? 1.12f + 0.04f * FMath::Sin(FxNow * 12.f) : (bSel ? 1.06f : 1.f);
		ToolIcon(t, FVector2D(RR.GetCenter().X, RR.Min.Y + 34.f * S + Bob), 31.f * S * Grow);
		float LY = RR.Min.Y + 68.f * S;
		for (const FString& L : Wrap(T.Name, 17.f * S, 106.f * S))
		{
			Txt(L, RR.GetCenter().X, LY, 17.f * S, Ink, 1);
			LY += 17.f * S;
		}
		const bool bAfford = GM->Money >= T.Cost;
		Txt(FString::Printf(TEXT("%d TL"), T.Cost), RR.GetCenter().X, RR.Max.Y - 23.f * S, 18.f * S, bAfford ? U(150, 110, 30) : Red, 1);
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
	const bool bCanTurn = GM->Mini == EDBMini::None && (bPlay || GM->Phase == EDBPhase::BagPrep || GM->Phase == EDBPhase::WaitScan || GM->DoLineTarget() == 4);
	for (int32 Dir : { -1, 1 })
	{
		const float Sz = 76.f * S;
		const FBox2D R = Rect(Dir < 0 ? BR.Min.X - Sz - 14.f * S : BR.Max.X + 14.f * S, BR.GetCenter().Y - Sz * 0.5f, Sz, Sz);
		const bool bHov = bCanTurn && Hover(R);
		const FBox2D RR = bHov ? R.ShiftBy(FVector2D(0.f, -4.f * S)) : R;
		(Dir < 0 ? TgTurnL : TgTurnR) = R;
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
	float LetterX = RR.GetCenter().X + 5.f * S;
	if (UTexture2D* K = LoadPng(TEXT("Arayuz/ui_kulp.png")))
	{
		// Dokunun oranı 1:4; kulp çubuğu ortada: kartın sol yarısına oturur, harfler sağda
		const float KH = RR.GetSize().Y - 16.f * S, KW = KH * 0.25f;
		DrawTex(K, Rect(RR.Min.X + 17.f * S - KW * 0.5f, RR.Min.Y + 8.f * S, KW, KH), 0.f, FVector2D(0.5f, 0.5f));
		LetterX = RR.Max.X - 15.f * S;
	}
	else
	{
		Box(Rect(RR.Min.X + 10.f * S, RR.Min.Y + 14.f * S, 6.f * S, RR.GetSize().Y - 28.f * S), U(120, 84, 56));
	}
	const TCHAR* Word = TEXT("HAZIRLIK");
	const float LetterPx = 17.f * S;
	float LY = RR.GetCenter().Y - 8.f * LetterPx * 0.55f;
	for (int32 i = 0; Word[i]; ++i)
	{
		Txt(FString::Chr(Word[i]), LetterX, LY, LetterPx, Ink, 1);
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
	// Revizyon 8: gereksiz çarpı kaldırıldı; masa soldaki kulpla (ya da Esc / sağ tık) kapanır
	TgClose = DrawerHandleRect();
	if (GM->IsTalking(C.BagIntroB, 5))
	{
		Glow(TgClose);
	}

	// Bant: sağda, yukarıdan aşağı (Purble Place'teki gibi ama dikey ve durmaz)
	const float BeltL = P.Max.X - 154.f * S;
	const float BeltW = 110.f * S;
	const float BeltTop = Y0 + 244.f * S;
	const float BeltBot = P.Max.Y - 124.f * S;
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
	const FBox2D Card0 = Rect(X0, Y0 + 62.f * S, 540.f * S, 166.f * S);
	TgOrder = Card0;
	if (GM->IsTalking(C.BagIntroB, 1))
	{
		Glow(Card0);
	}
	Card(Card0, NoteY, Ink, 6.f);
	const TArray<int32> Pending = GM->PendingOrders();
	if (Ord >= 0)
	{
		const FDBProblemDef& Pr = C.Problems[Ord];
		const int32 Num = Pending.IndexOfByKey(Ord) + 1;
		Txt(FString::Printf(TEXT("SİPARİŞ %d / %d"), FMath::Max(1, Num), Pending.Num()), Card0.Min.X + 16.f * S, Card0.Min.Y + 8.f * S, 17.f * S, DarkK);
		Txt(C.Buildings[Pr.Building].Name, Card0.Min.X + 16.f * S, Card0.Min.Y + 28.f * S, 30.f * S, Ink);
		// 1. temel kit
		Txt(TEXT("1. Temel kit:"), Card0.Min.X + 16.f * S, Card0.Min.Y + 76.f * S, 20.f * S, Ink);
		const FDBKitDef& K = C.Kits[Pr.Kit];
		const float KW = Measure(K.Name, 19.f * S).X + 20.f * S;
		RoundBox(Rect(Card0.Min.X + 146.f * S, Card0.Min.Y + 72.f * S, KW, 30.f * S), 8.f * S, K.Color);
		Txt(K.Name, Card0.Min.X + 156.f * S, Card0.Min.Y + 76.f * S, 19.f * S, Paper);
		KitIcon(Pr.Kit, FVector2D(Card0.Min.X + 170.f * S + KW, Card0.Min.Y + 87.f * S), 19.f * S);
		// 2. kişiye özel
		Txt(TEXT("2. Kişiye özel:"), Card0.Min.X + 16.f * S, Card0.Min.Y + 122.f * S, 20.f * S, Ink);
		float IX = Card0.Min.X + 160.f * S;
		for (int32 It : Pr.Personal)
		{
			ItemIcon(It, FVector2D(IX + 18.f * S, Card0.Min.Y + 134.f * S), 18.f * S);
			Txt(C.Items[It].Name, IX + 42.f * S, Card0.Min.Y + 123.f * S, 19.f * S, Ink);
			IX += Measure(C.Items[It].Name, 19.f * S).X + 62.f * S;
		}
		// Çantanın resmi: olması gereken hâli
		DrawBagIcon(FVector2D(Card0.Max.X - 62.f * S, Card0.Min.Y + 66.f * S), 0.8f * S, Pr.Kit, Pr.Personal);
		Button(TEXT("OrderPrev"), -1, Rect(Card0.Max.X + 10.f * S, Card0.Min.Y + 20.f * S, 50.f * S, 50.f * S), TEXT("<"), 26.f * S, CardC, DarkK, Ink, bEdit && Pending.Num() > 1);
		Button(TEXT("OrderNext"), -1, Rect(Card0.Max.X + 10.f * S, Card0.Min.Y + 84.f * S, 50.f * S, 50.f * S), TEXT(">"), 26.f * S, CardC, DarkK, Ink, bEdit && Pending.Num() > 1);
	}
	else
	{
		Txt(TEXT("Bütün çanta siparişleri tamam!"), Card0.Min.X + 16.f * S, Card0.Min.Y + 64.f * S, 26.f * S, Green);
	}

	// İstasyonlar: 0 temel kit, 1..3 kişiye özel (bant önlerinden geçerken tıkla)
	TgStations = FBox2D(ForceInit);
	for (int32 K = 0; K < ADBGameMode::NumStations && K <= C.Stations.Num(); ++K)
	{
		const float Yc = BeltTop + ADBGameMode::StationCenter(K) * BeltLen;
		const FBox2D Row = Rect(X0, Yc - 55.f * S, 594.f * S, 110.f * S);
		const bool bActive = GM->IsStationActive(K);
		if (K == 0) TgKitStation = Row; else TgStations += Row;
		Box(Row, bActive ? U(255, 246, 214) : U(214, 208, 220));
		Frame(Row, bActive ? U(236, 170, 40) : DarkK, (bActive ? 4.f : 2.f) * S);
		// Boru: istasyondan banda
		Box(Rect(Row.Max.X, Yc - 6.f * S, BeltL - Row.Max.X, 12.f * S), bActive ? U(236, 170, 40) : U(150, 150, 160));
		Txt(K == 0 ? TEXT("TEMEL KİT") : C.Stations[K - 1].Name, Row.Min.X + 10.f * S, Row.Min.Y + 4.f * S, 16.f * S, DarkK);
		for (int32 B = 0; B < 3; ++B)
		{
			const FBox2D Bt = Rect(Row.Min.X + 10.f * S + B * 194.f * S, Row.Min.Y + 26.f * S, 184.f * S, 76.f * S);
			const bool bHov = bActive && Hover(Bt);
			const FBox2D BR = bHov ? Bt.ShiftBy(FVector2D(0.f, -3.f * S)) : Bt;
			if (K == 0)
			{
				const FDBKitDef& Kd = C.Kits[B];
				const bool bSel = GM->bBagOnBelt && GM->BagKit == B;
				Card(BR, Kd.Color, bSel ? Ink : DarkK, bHov ? 6.f : 3.f);
				if (bSel) Frame(BR.ExpandBy(3.f * S), Ink, 3.f * S);
				const bool bKitPng = KitIcon(B, FVector2D(BR.Min.X + 34.f * S, BR.GetCenter().Y - (bHov ? 3.f * S * FMath::Abs(FMath::Sin(FxNow * 9.f)) : 0.f)), 28.f * S * (bHov ? 1.12f : 1.f));
				const float TX = BR.Min.X + (bKitPng ? 70.f : 12.f) * S;
				Txt(Kd.Short, TX, BR.Min.Y + 6.f * S, 23.f * S, Paper);
				for (int32 i = 0; i < Kd.Defaults.Num(); ++i)
				{
					ItemIcon(Kd.Defaults[i], FVector2D(TX + 15.f * S + i * 34.f * S, BR.Min.Y + 54.f * S), 14.f * S);
				}
			}
			else
			{
				const int32 It = C.Stations[K - 1].Items[B];
				const bool bIn = GM->bBagOnBelt && GM->BagPersonal.Contains(It);
				Card(BR, bIn ? U(226, 240, 214) : Paper, bIn ? Green : DarkK, bHov ? 6.f : 3.f);
				ItemIcon(It, FVector2D(BR.Min.X + 38.f * S, BR.GetCenter().Y - (bHov ? 3.f * S * FMath::Abs(FMath::Sin(FxNow * 9.f)) : 0.f)), 31.f * S * (bHov ? 1.12f : 1.f));
				const float NPx = 20.f * S;
				const TArray<FString> NL = Wrap(C.Items[It].Name, NPx, 100.f * S);
				float NY = BR.GetCenter().Y - NL.Num() * NPx * 0.62f;
				for (const FString& L : NL)
				{
					Txt(L, BR.Min.X + 78.f * S, NY, NPx, Ink);
					NY += NPx * 1.15f;
				}
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

	// Teslim (Andaç, 17 Ekim ~19.45): TESLİM düğmesi yerine bandın dibinde açık çanta. Paket iner, çantaya düşer,
	// çanta kapanır, teslim edilir (uçup kasabaya gider). Çantaya basılı tutmak bandı hızlandırır.
	const FVector2D BagC(BeltCX, BeltBot + 70.f * S);
	const FBox2D Del = Rect(BagC.X - 64.f * S, BagC.Y - 66.f * S, 128.f * S, 118.f * S);
	TgDeliver = Del;
	if (GM->IsTalking(C.BagIntroB, 4)) Glow(Del);
	const bool bFast = GM->bBeltFast;
	const float Pk = GM->bPacking ? GM->Now - GM->PackStartAt : -1.f;
	const float Age = GM->Now - GM->DeliverTime;
	const FDBContent& Ct = C;
	auto KitCol = [&Ct](int32 K) { return Ct.Kits.IsValidIndex(K) ? Ct.Kits[K].Color : U(196, 154, 108); };
	if (Pk >= 0.f)
	{
		// 0–0,38 paket düşer, 0,38–0,72 kapak kapanır, sonra fermuar
		const float Fall = FMath::Clamp(Pk / 0.38f, 0.f, 1.f);
		const float Close = FMath::Clamp((Pk - 0.38f) / 0.34f, 0.f, 1.f);
		const float Squash = (Pk > 0.38f && Pk < 0.55f) ? FMath::Sin((Pk - 0.38f) / 0.17f * PI) * 0.08f : 0.f;
		DrawDeliveryBag(BagC + FVector2D(0.f, Squash * 20.f * S), S * (1.f + Squash), KitCol(GM->PackKit), FMath::InterpEaseInOut(0.f, 1.f, Close, 2.f), Close > 0.9f, GM->PackKit);
		if (Fall < 1.f)
		{
			const FVector2D From(BeltCX, BeltBot);
			const FVector2D To(BagC.X, BagC.Y - 40.f * S);
			DrawTray(FMath::Lerp(From, To, Fall * Fall), S * FMath::Lerp(1.f, 0.55f, Fall), GM->PackKit, GM->PackItems);
		}
		if (Close >= 1.f)
		{
			Sparkles(Del.ExpandBy(10.f * S), 8, Pk - 0.72f, 1.f, 7);
		}
	}
	else if (Age >= 0.f && Age < 1.0f)
	{
		if (GM->bDeliverOk)
		{
			// Teslim: çanta kalkar, kasabaya doğru (sola, yukarı) uçar, küçülür
			const float U2 = FMath::Clamp(Age / 0.9f, 0.f, 1.f);
			const FVector2D Pc = BagC + FVector2D(-420.f * S * U2 * U2, -260.f * S * FMath::Sin(U2 * PI * 0.6f));
			DrawDeliveryBag(Pc, S * FMath::Lerp(1.f, 0.35f, U2), KitCol(GM->PackKit), 1.f, true, GM->PackKit);
			Txt(TEXT("Teslim edildi!"), BagC.X, Del.Max.Y - 30.f * S - 20.f * S * U2, 22.f * S, WithA(Green, 1.f - U2 * 0.5f), 1);
		}
		else
		{
			// Yanlış: çanta sallanır, kapak açılır, kırmızı çarpı
			const float Sh = FMath::Sin(Age * 50.f) * 9.f * S * (1.f - Age);
			DrawDeliveryBag(BagC + FVector2D(Sh, 0.f), S, KitCol(GM->PackKit), FMath::Clamp(1.f - Age * 2.f, 0.f, 1.f), false, GM->PackKit);
			const FVector2D Cc = BagC - FVector2D(0.f, 92.f * S);
			Disc(Cc, 30.f * S, U(250, 222, 212));
			Cross(Cc, 34.f * S, Red);
		}
	}
	else
	{
		// Boş, açık çanta bekler (yeni gelince yaylanarak belirir); paketin kiti seçilince çanta o renge döner
		const float Pop = (Age >= 1.0f && Age < 1.35f) ? FMath::Sin((Age - 1.0f) / 0.35f * PI) * 0.12f : 0.f;
		const float Bob = FMath::Sin(GM->Now * 3.f) * 2.f * S;
		DrawDeliveryBag(BagC + FVector2D(0.f, Bob), S * (1.f + Pop), KitCol(GM->bBagOnBelt ? GM->BagKit : -1), bFast ? 0.15f : 0.f, false, -1);
	}
	if (bFast)
	{
		const FBox2D Fb = Rect(BagC.X - 52.f * S, BeltBot - 40.f * S, 104.f * S, 30.f * S);
		RoundCard(Fb, U(255, 214, 90), Ink, 10.f * S, 2.f);
		Txt(TEXT("HIZLI >>"), Fb.GetCenter().X, Fb.Min.Y + 4.f * S, 18.f * S, Ink, 1);
	}
	if (GM->bBagOnBelt && bEdit && !GM->bPacking)
	{
		Hit(TEXT("Deliver"), -1, Del);
	}
	Button(TEXT("BeltReset"), -1, Rect(X0, P.Max.Y - 64.f * S, 160.f * S, 44.f * S), TEXT("Baştan"), 19.f * S, CardC, DarkK, Ink, bEdit && GM->bBagOnBelt && !GM->bPacking);
	Txt(TEXT("Bant durmaz: paket önünden geçerken tıkla."), X0 + 176.f * S, P.Max.Y - 72.f * S, 18.f * S, DarkK);
	Txt(TEXT("Alttaki çantaya basılı tut: bant hızlanır."), X0 + 176.f * S, P.Max.Y - 46.f * S, 18.f * S, DarkK);

	// Paket bantta (çantaya düşmeden önce)
	if (GM->bBagOnBelt && !GM->bPacking)
	{
		const FVector2D TrayC(BeltCX, BeltTop + GM->BagT * BeltLen);
		const float Pop = FMath::Clamp((FxNow - TrayPopT) / 0.28f, 0.f, 1.f);
		DrawTray(TrayC, S * (1.f + 0.18f * FMath::Sin(Pop * PI)), GM->BagKit, GM->BagPersonal);
		// Uçan eşyalar: düğmeden pakete yay çizerek, küçülerek; varınca paket yaylanır
		for (int32 i = Flies.Num() - 1; i >= 0; --i)
		{
			const FFly& F = Flies[i];
			const float Uf = (FxNow - F.T) / 0.38f;
			if (Uf >= 1.f)
			{
				TrayPopT = FxNow;
				Flies.RemoveAt(i);
				continue;
			}
			const float E = 1.f - FMath::Pow(1.f - Uf, 2.f);
			const FVector2D To = TrayC - FVector2D(0.f, 18.f * S);
			const FVector2D Pp = FMath::Lerp(F.From, To, E) - FVector2D(0.f, 110.f * S * FMath::Sin(Uf * PI));
			const float Rr = FMath::Lerp(26.f, 13.f, E) * S;
			for (int32 k = 1; k <= 3; ++k)
			{
				const float Ub = FMath::Max(0.f, Uf - k * 0.06f);
				const FVector2D Tp = FMath::Lerp(F.From, To, 1.f - FMath::Pow(1.f - Ub, 2.f)) - FVector2D(0.f, 110.f * S * FMath::Sin(Ub * PI));
				Disc(Tp, (5.f - k) * S, U(255, 214, 90, uint8(170 - 45 * k)));
			}
			if (F.bKit)
			{
				if (!KitIcon(F.Id, Pp, Rr))
				{
					Disc(Pp, Rr, C.Kits.IsValidIndex(F.Id) ? C.Kits[F.Id].Color : Coral);
				}
			}
			else
			{
				ItemIcon(F.Id, Pp, Rr);
			}
		}
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
	RoundCard(BR, Paper, Ink, 22.f * S, 8.f);
	Tri(TailA, TailB, TailTip, Paper);
	Line(TailA, TailTip, 2.5f * S, Ink);
	Line(TailB, TailTip, 2.5f * S, Ink);
	Disc(FVector2D(BR.Min.X + 32.f * S, BR.Min.Y + 30.f * S), 9.f * S, C.SpeakerColor[Who]);
	Txt(C.SpeakerName[Who] + TEXT("  ·  ") + C.SpeakerRole[Who], BR.Min.X + 50.f * S, BR.Min.Y + 17.f * S, 22.f * S, DarkK);
	Para(L->Text, BR.Min.X + 26.f * S, BR.Min.Y + 54.f * S, 26.f * S, BR.GetSize().X - 52.f * S, Ink, 1.25f, GM->VisibleChars());
	if (GM->IsLineComplete())
	{
		const float A = 0.5f + 0.5f * FMath::Sin(GM->Now * 5.f);
		const bool bDo = GM->DoLineTarget() != 0;
		Txt(bDo ? TEXT("gösterilen tuşa bas") : TEXT("devam için tıkla"), BR.Max.X - 22.f * S, BR.Max.Y - 30.f * S, bDo ? 18.f * S : 16.f * S,
			bDo ? WithA(Coral, 0.5f + 0.5f * A) : WithA(DarkK, 0.4f + 0.6f * A), 2);
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
		if (GM->IsTalking(C.PanelIntroC, 5)) return Center(TgTurnR);
		return false;
	}
	if (Who == 1 && GM->BagsDoneHint())
	{
		return Center(TgDrawer);
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
	const float Wd = 380.f * S;
	const int32 Cur = GM->CurrentTask();
	// Başlık şeridi her zaman; liste açılır-kapanır (revizyon 6: sade, sıralı, aktif görev belirgin)
	if (!bTasksOpen)
	{
		const FBox2D Hd = Rect(X, 78.f * S, 230.f * S, 40.f * S);
		RoundCard(Hd, NoteY, DarkK, 10.f * S, 4.f);
		Txt(FString::Printf(TEXT("GÖREVLER  %d / %d  +"), GM->NumSolved(), GM->Solved.Num()), X + 14.f * S, Hd.Min.Y + 8.f * S, 20.f * S, Ink);
		Hit(TEXT("TasksToggle"), -1, Hd);
		Blockers.Add(Hd);
		TgTasks = Hd;
		return;
	}
	// Önce yüksekliği hesapla
	float Hgt = 56.f * S;
	for (int32 p = 0; p < C.Problems.Num(); ++p)
	{
		if (!GM->Revealed[p]) continue;
		Hgt += (p == Cur) ? (34.f + 22.f * Wrap(C.Problems[p].Hint, 17.f * S, Wd - 60.f * S).Num() + 34.f) * S : 26.f * S;
	}
	const FBox2D R = Rect(X, 78.f * S, Wd, Hgt + 8.f * S);
	TgTasks = R;
	if (GM->IsTalking(C.AfterScan, 0) || GM->IsTalking(C.BagIntroB, 1))
	{
		Glow(R);
	}
	RoundCard(R, NoteY, DarkK, 12.f * S, 5.f);
	Blockers.Add(R);
	Txt(TEXT("GÖREVLER"), X + 18.f * S, R.Min.Y + 14.f * S, 24.f * S, Ink);
	Txt(FString::Printf(TEXT("%d / %d   -"), GM->NumSolved(), GM->Solved.Num()), R.Max.X - 18.f * S, R.Min.Y + 16.f * S, 20.f * S, DarkK, 2);
	Hit(TEXT("TasksToggle"), -1, Rect(R.Min.X, R.Min.Y, R.GetSize().X, 46.f * S));
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
		// Revizyon 3: hafif kamera hareketi: yavaş yakınlaşma, sahneden sahneye yönü değişen kayma (doku penceresiyle)
		const float U = FMath::Clamp(T / FMath::Max(1.f, GM->ComicSeconds), 0.f, 1.f);
		const float Ease = U * U * (3.f - 2.f * U);
		const float Win = 1.f / (1.f + 0.09f * Ease);
		const FVector2D Dir = (Ix % 2 == 0) ? FVector2D(1.f, 0.4f) : FVector2D(-1.f, -0.3f);
		const FVector2D Cn = FVector2D(0.5f, 0.5f) + Dir * (1.f - Win) * 0.5f * Ease;
		DrawTexUV(Img, P, Cn - FVector2D(Win * 0.5f), Cn + FVector2D(Win * 0.5f));
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
			Txt(TEXT("Tetkik"), Game.GetCenter().X, Game.Min.Y + 52.f * S, 34.f * S, Paper, 1);
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

	// Atmosfer: süzülen toz / ışıltı; sahne başında kâğıttan açılma
	Sparkles(P, 26, GM->Now, 0.55f, 77u + Ix);
	if (T < 0.45f)
	{
		Box(P, WithA(Paper, 1.f - T / 0.45f));
	}

	// Altyazı kutusu (çizgi roman anlatıcı kutusu): yuvarlak, sahne başında yaylanarak gelir
	const FString& Cap = C.Comic[Ix];
	const TArray<FString> Lines = Wrap(Cap, 30.f * S, 900.f * S);
	FBox2D CapR = Rect(P.Min.X + 30.f * S, P.Max.Y - 40.f * S - Lines.Num() * 38.f * S, 960.f * S, Lines.Num() * 38.f * S + 24.f * S);
	{
		const float Pa = FMath::Clamp((T - 0.2f) / 0.35f, 0.f, 1.f);
		const float K = Pa >= 1.f ? 1.f : 0.85f + 0.15f * Pa + 0.08f * FMath::Sin(Pa * PI);
		const FVector2D C0 = CapR.GetCenter(), Hs = CapR.GetExtent() * K;
		CapR = FBox2D(C0 - Hs, C0 + Hs);
	}
	RoundCard(CapR, NoteY, Ink, 16.f * S, 6.f);
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
	// Revizyon 14: konuşmada kasabalının portresi (çizimi varsa Content/Karakterler/NPC_<Ad>.png, yoksa yer tutucu)
	const float BW = 540.f * S;
	const float PortW = 110.f * S;
	const TArray<FString> Lines = Wrap(Text, 20.f * S, BW - 40.f * S - PortW);
	const float BH = FMath::Max(50.f * S + Lines.Num() * 26.f * S + 58.f * S, 160.f * S);
	FVector2D Pos(A.X - 70.f * S, A.Y - 34.f * S - BH);
	Pos.X = FMath::Clamp(Pos.X, 20.f * S, W - BW - 20.f * S);
	Pos.Y = FMath::Max(Pos.Y, 84.f * S);
	const FBox2D R = Rect(Pos.X, Pos.Y, BW, BH);
	const FVector2D T0(FMath::Clamp(A.X, R.Min.X + 30.f * S, R.Max.X - 70.f * S), R.Max.Y - 2.f);
	RoundCard(R, Paper, Ink, 20.f * S, 6.f);
	Tri(T0, T0 + FVector2D(40.f * S, 0.f), A, Paper);
	Line(T0, A, 2.5f * S, Ink);
	Line(T0 + FVector2D(40.f * S, 0.f), A, 2.5f * S, Ink);
	Blockers.Add(R);
	DrawPortrait(B, FVector2D(R.Min.X + 62.f * S, R.Min.Y + 70.f * S), 46.f * S);
	Disc(FVector2D(R.Min.X + PortW + 26.f * S, R.Min.Y + 26.f * S), 9.f * S, GM->NpcKind == 2 ? Green : (GM->NpcKind == 1 ? Coral : Teal));
	Txt(C.Residents[B].Name, R.Min.X + PortW + 42.f * S, R.Min.Y + 13.f * S, 22.f * S, Ink);
	const int32 Shown = FMath::FloorToInt(Age * 50.f);
	int32 Left = Shown;
	float Y = R.Min.Y + 46.f * S;
	for (const FString& L : Lines)
	{
		if (Left <= 0) break;
		Txt(Left >= L.Len() ? L : L.Left(Left), R.Min.X + PortW + 20.f * S, Y, 20.f * S, Ink);
		Left -= L.Len() + 1;
		Y += 26.f * S;
	}
	const float BY = R.Max.Y - 50.f * S;
	if (bBag)
	{
		Button(TEXT("NpcBag"), -1, Rect(R.Min.X + PortW + 18.f * S, BY, 230.f * S, 38.f * S), TEXT("Çantasını hazırla"), 19.f * S, Coral, Ink, Paper);
	}
	Button(TEXT("NpcClose"), -1, Rect(R.Max.X - 128.f * S, BY, 110.f * S, 38.f * S), TEXT("Tamam"), 19.f * S, CardC, DarkK, Ink);
}

// ---------------------------------------------------------------- tahta döndürme oku, mini oyunlar

void ADBHUD::TurnArrow(const FVector2D& C, float R, int32 Dir, const FLinearColor& Col)
{
	if (UTexture2D* T = LoadPng(Dir > 0 ? TEXT("Arayuz/ui_ok_sag.png") : TEXT("Arayuz/ui_ok_sol.png")))
	{
		// Etkin değilken soluk (Col gri gelir)
		const bool bOff = Col.Equals(U(170, 160, 150), 0.02f);
		DrawTex(T, Rect(C.X - R * 1.5f, C.Y - R * 1.6f, R * 3.f, R * 3.f), 0.f, FVector2D(0.5f, 0.5f), bOff ? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
		return;
	}
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
	FBox2D R = Rect(W * 0.5f - 400.f * S, H * 0.5f - 280.f * S, 800.f * S, 540.f * S);
	// Revizyon 9: hata anında pencere sallanır, kenarı kırmızı yanar
	const float BadAge = GM->Now - GM->MiniMsgTime;
	const bool bShake = GM->bMiniMsgBad && BadAge >= 0.f && BadAge < 0.45f;
	if (bShake)
	{
		R = R.ShiftBy(FVector2D(FMath::Sin(BadAge * 70.f) * 14.f * S * (1.f - BadAge / 0.45f), 0.f));
	}
	const float CX = R.GetCenter().X;
	RoundCard(R, Paper, bShake ? Red : Ink, 24.f * S, 8.f);
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
		// Kazanınca: büyükten oturan damga + işin yerinden fışkıran su
		const float Age = GM->Now - GM->MiniWinAt;
		const float A = FMath::Clamp(Age / 0.25f, 0.f, 1.f);
		TickMark(FVector2D(CX + 300.f * S, R.Min.Y + 150.f * S), 70.f * S * A + 10.f * S, Green);
		Fountain(FVector2D(CX, R.Min.Y + (bNut ? 372.f : 268.f) * S), Age, 46, U(110, 170, 222), 11u);
		const float Sc = 1.f + 1.2f * FMath::Exp(-Age * 9.f);
		const FString Stamp = bNut ? TEXT("TAMİR!") : TEXT("YERİNE OTURDU!");
		const float Px = 64.f * S * Sc;
		const float SW = Measure(Stamp, Px).X + 50.f * S * Sc;
		const FBox2D SR = Rect(CX - SW * 0.5f, R.Min.Y + 150.f * S - Px * 0.2f, SW, Px * 1.25f);
		RoundCard(SR, WithA(U(236, 246, 228), FMath::Min(1.f, Age * 6.f)), Green, 18.f * S, 6.f);
		Txt(Stamp, CX, SR.Min.Y + Px * 0.12f, Px, WithA(Green, FMath::Min(1.f, Age * 6.f)), 1);
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
	if (bIn)
	{
		Frame(Zone.ExpandBy((4.f + 3.f * FMath::Sin(FxNow * 12.f)) * S), U(255, 230, 120, 200), 3.f * S);   // yeşilde: parlıyor
	}
	Box(Zone, bIn ? U(90, 200, 110, 200) : U(90, 186, 104, 140));
	Frame(Zone, U(40, 150, 70), 3.f * S);
	// Kapak
	const FVector2D Cv(Tr.Min.X + GM->MiniPos * TW, Tr.GetCenter().Y);
	if (bIn)
	{
		Sparkles(FBox2D(Cv - FVector2D(60.f * S), Cv + FVector2D(60.f * S)), 10, FxNow * 2.f, 0.9f, 5u);
	}
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
	if (GM->VfxLevel == 0)
	{
		Trail.Reset();
		ClickFxs.RemoveAll([Now](const FClickFx& C) { return Now - C.T > 0.4f; });
	}
	// Fare izi: son 0,3 sn'deki konumlar, sona doğru incelip söner
	if (GM->VfxLevel > 0 && (Trail.Num() == 0 || FVector2D::Distance(Trail.Last().P, Mouse) > 3.f))
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
	if (Age < 0.f || Age > Life || GM->VfxLevel == 0)
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

// ---------------------------------------------------------------- keskin font, menü düğmesi

UFont* ADBHUD::MakeRuntimeFont(const TCHAR* File)
{
	// Motorun varsayılan fontu küçük bir bitmap: takvimde 3-4 kat büyütülünce bulanıklaşıyordu. Runtime font harfleri
	// istenen boyutta yeniden çizer. Dosyalar: Content/Fonts/ (pakete NonUFS girer; JamKit 04 §5).
	// Tema fontu Baloo 2 (OFL): Arayuz.ttf = Medium (metin), Arayuz_Kalin.ttf = Bold (başlık, düğme)
	const FString Path = FPaths::ProjectContentDir() / TEXT("Fonts") / File;
	if (!FPaths::FileExists(Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("DB_FONT yok: %s, motor fontu kullaniliyor"), *Path);
		return FCString::Strcmp(File, TEXT("Arayuz.ttf")) == 0 ? GEngine->GetLargeFont() : nullptr;
	}
	UFont* F = NewObject<UFont>(this);
	F->FontCacheType = EFontCacheType::Runtime;
	F->LegacyFontSize = 32;
	FTypefaceEntry& E = F->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
	E.Name = TEXT("Regular");
	E.Font = FFontData(Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
	UE_LOG(LogTemp, Log, TEXT("DB_FONT %s"), *Path);
	return F;
}

void ADBHUD::DrawMenuButton()
{
	// Oyun sırasında takvim menüsüne dönüş (onaylı: kasabadaki ilerleme sıfırlanır)
	if (GM->Phase == EDBPhase::ToBoard)
	{
		return;
	}
	// Andaç (17 Ekim ~19.30): Menü → doğrudan takvim (ilerleme korunur, takvimde "Oyuna devam et"). Esc duraklatma penceresini açar.
	const FBox2D R = Rect(20.f * S, 20.f * S, 120.f * S, 44.f * S);
	Blockers.Add(R);
	Button(TEXT("MenuCal"), -1, R, TEXT("Menü"), 19.f * S, CardC, DarkK, Ink, !GM->bMenuConfirm && GM->CanPause());
}

void ADBHUD::DrawMenuConfirm()
{
	// Duraklatma (revizyon 7): Oyuna devam / Takvime dön (ilerleme korunur) / Ana menü (baştan)
	Box(0.f, 0.f, W, H, U(40, 30, 22, 130));
	Blockers.Add(Rect(0.f, 0.f, W, H));
	const FBox2D R = Rect(W * 0.5f - 280.f * S, H * 0.5f - 250.f * S, 560.f * S, 520.f * S);
	RoundCard(R, Paper, Ink, 26.f * S, 8.f);
	Txt(TEXT("Duraklatıldı"), R.GetCenter().X, R.Min.Y + 26.f * S, 34.f * S, Ink, 1);
	const float BX = R.Min.X + 60.f * S, BW2 = R.GetSize().X - 120.f * S;
	Button(TEXT("MenuNo"), -1, Rect(BX, R.Min.Y + 96.f * S, BW2, 64.f * S), TEXT("Oyuna devam"), 26.f * S, Coral, Ink, Paper);
	Button(TEXT("MenuCal"), -1, Rect(BX, R.Min.Y + 184.f * S, BW2, 64.f * S), TEXT("Takvime dön"), 26.f * S, CardC, DarkK, Ink);
	Txt(TEXT("ilerleme korunur, takvimden \"Oyuna devam et\""), R.GetCenter().X, R.Min.Y + 252.f * S, 16.f * S, DarkK, 1);
	Button(TEXT("MenuYes"), -1, Rect(BX, R.Min.Y + 290.f * S, BW2, 64.f * S), TEXT("Ana menü"), 26.f * S, CardC, DarkK, Ink);
	Txt(TEXT("kasaba baştan başlar"), R.GetCenter().X, R.Min.Y + 358.f * S, 16.f * S, Red, 1);
	// Başarımlar: açılanlar altın, kapalılar gri
	const int32 NA = ADBGameMode::Achievements().Num();
	Txt(FString::Printf(TEXT("Başarımlar %d / %d"), GM->NumAchieved(), NA), R.GetCenter().X, R.Min.Y + 398.f * S, 18.f * S, Ink, 1);
	for (int32 i = 0; i < NA; ++i)
	{
		const FVector2D Bc(R.GetCenter().X + (i - (NA - 1) * 0.5f) * 56.f * S, R.Min.Y + 460.f * S);
		DrawAchBadge(i, Bc, 22.f * S, GM->AchUnlocked.IsValidIndex(i) && GM->AchUnlocked[i]);
		if (FVector2D::Distance(Mouse, Bc) < 24.f * S)
		{
			const ADBGameMode::FAchievement& A = ADBGameMode::Achievements()[i];
			const FString Tip = FString(A.Name) + TEXT(": ") + A.Desc;
			const float TW = Measure(Tip, 16.f * S).X + 24.f * S;
			const FBox2D TR = Rect(FMath::Clamp(Bc.X - TW * 0.5f, 10.f * S, W - TW - 10.f * S), R.Max.Y + 8.f * S, TW, 32.f * S);
			RoundCard(TR, Paper, Ink, 10.f * S, 3.f);
			Txt(Tip, TR.Min.X + 12.f * S, TR.Min.Y + 6.f * S, 16.f * S, Ink);
		}
	}
}

// ---------------------------------------------------------------- revizyon: yuvarlak kart, yüzdeler, efektler

void ADBHUD::RoundBox(const FBox2D& R, float Radius, const FLinearColor& C)
{
	const float Rd = FMath::Min(Radius, FMath::Min(R.GetSize().X, R.GetSize().Y) * 0.5f);
	Box(Rect(R.Min.X + Rd, R.Min.Y, R.GetSize().X - 2.f * Rd, R.GetSize().Y), C);
	Box(Rect(R.Min.X, R.Min.Y + Rd, Rd, R.GetSize().Y - 2.f * Rd), C);
	Box(Rect(R.Max.X - Rd, R.Min.Y + Rd, Rd, R.GetSize().Y - 2.f * Rd), C);
	// Köşeler: çeyrek daireler (tam disk, kenar kutularıyla çakışan yarısı aynı renkte)
	const int32 N = 20;
	const FVector2D Cs[4] = { FVector2D(R.Min.X + Rd, R.Min.Y + Rd), FVector2D(R.Max.X - Rd, R.Min.Y + Rd),
		FVector2D(R.Max.X - Rd, R.Max.Y - Rd), FVector2D(R.Min.X + Rd, R.Max.Y - Rd) };
	const float A0[4] = { PI, 1.5f * PI, 0.f, 0.5f * PI };
	for (int32 c = 0; c < 4; ++c)
	{
		for (int32 k = 0; k < N; ++k)
		{
			const float A = A0[c] + 0.5f * PI * k / N, B = A0[c] + 0.5f * PI * (k + 1) / N;
			Tri(Cs[c], Cs[c] + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rd, Cs[c] + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Rd, C);
		}
	}
}

void ADBHUD::RoundCard(const FBox2D& R, const FLinearColor& Fill, const FLinearColor& Edge, float Radius, float Shadow)
{
	const bool bNote = Fill.Equals(NoteY, 0.02f);
	if (UTexture2D* T = LoadPng(bNote ? TEXT("Arayuz/ui_not.png") : TEXT("Arayuz/ui_balon.png")))
	{
		const float D = FMath::Clamp(Radius * 1.35f, 8.f * S, 34.f * S);
		Nine(T, R.ShiftBy(FVector2D(Shadow * 0.6f, Shadow) * S), D, FLinearColor(0.f, 0.f, 0.f, 0.22f * Fill.A));
		Nine(T, R, D, bNote ? FLinearColor(1.f, 1.f, 1.f, Fill.A) : TintFrom(Fill, PanelBase));
		return;
	}
	// Revizyon 7: konuşma balonları köşeli değil, çizimlerle uyumlu yumuşak
	RoundBox(R.ShiftBy(FVector2D(Shadow * 0.6f, Shadow) * S), Radius, WithA(ShadowC, Fill.A));
	RoundBox(R.ExpandBy(2.5f * S), Radius + 2.5f * S, Edge);
	RoundBox(R, Radius, Fill);
}

void ADBHUD::DrawMeters()
{
	// Revizyon 11-12: Kasaba Hazırlık % ve Şehir Farkındalık % (bütçenin sağında), dolarken nabız atar
	if (!GM->bPanelOpen && !GM->bPlayStarted)
	{
		return;
	}
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float H1 = GM->HazirlikYuzde(), F1 = GM->FarkindalikYuzde();
	const bool bRise1 = ShownHazirlik < H1 - 0.5f, bRise2 = ShownFarkindalik < F1 - 0.5f;
	ShownHazirlik = FMath::FInterpConstantTo(ShownHazirlik, H1, Dt, 40.f);
	ShownFarkindalik = FMath::FInterpConstantTo(ShownFarkindalik, F1, Dt, 40.f);
	const FBox2D M = MoneyRect();
	const FBox2D R = Rect(M.Max.X + 16.f * S, M.Min.Y, 300.f * S, M.GetSize().Y + 22.f * S);
	RoundCard(R, Paper, DarkK, 12.f * S, 5.f);
	Blockers.Add(R);
	auto Bar = [&](float Y, const TCHAR* Name, float V, const FLinearColor& Col, bool bRise)
	{
		Txt(Name, R.Min.X + 14.f * S, Y, 17.f * S, DarkK);
		Txt(FString::Printf(TEXT("%%%d"), FMath::RoundToInt(V)), R.Max.X - 14.f * S, Y - 2.f * S, 20.f * S, bRise ? Col : Ink, 2);
		const FBox2D B = Rect(R.Min.X + 14.f * S, Y + 22.f * S, R.GetSize().X - 28.f * S, 9.f * S);
		RoundBox(B, 4.5f * S, U(214, 204, 190));
		if (V > 0.5f)
		{
			RoundBox(Rect(B.Min.X, B.Min.Y, B.GetSize().X * V / 100.f, B.GetSize().Y), 4.5f * S, Col);
		}
		if (bRise)
		{
			Frame(B.ExpandBy(3.f * S), WithA(Col, 0.5f + 0.5f * FMath::Sin(FxNow * 18.f)), 2.f * S);
		}
	};
	Bar(R.Min.Y + 8.f * S, TEXT("Kasaba Hazırlık"), ShownHazirlik, Green, bRise1);
	Bar(R.Min.Y + 44.f * S, TEXT("Şehir Farkındalık"), ShownFarkindalik, Teal, bRise2);
}

void ADBHUD::DrawEventFx()
{
	const float Dt = GetWorld()->GetDeltaSeconds();
	// Başarı: ekran kenarında yeşil parlama + işaretin yerinden konfeti. Hata: kırmızı parlama.
	auto Edge = [&](float Age, const FLinearColor& Col)
	{
		if (Age < 0.f || Age > 0.7f)
		{
			return;
		}
		const float A = FMath::Sin(PI * FMath::Clamp(Age / 0.7f, 0.f, 1.f)) * 0.55f;
		for (int32 k = 0; k < 6; ++k)
		{
			const float T = (6 - k) * 9.f * S;
			const FLinearColor C = WithA(Col, A * (k + 1) / 6.f * 0.35f);
			Box(0.f, 0.f, W, T, C);
			Box(0.f, H - T, W, T, C);
			Box(0.f, T, T, H - 2.f * T, C);
			Box(W - T, T, T, H - 2.f * T, C);
		}
	};
	Edge(GM->Now - GM->FxGoodTime, U(120, 220, 110));
	Edge(GM->Now - GM->FxBadTime, U(230, 80, 60));
	if (GM->FxGoodTime != LastFxGood)
	{
		LastFxGood = GM->FxGoodTime;
		FVector2D P(W * 0.5f, H * 0.4f);
		if (GM->FxBuilding >= 0)
		{
			PlayerOwner->ProjectWorldLocationToScreen(GM->World->MarkerAnchor(GM->FxBuilding), P);
		}
		const FLinearColor Pal[] = { Coral, Teal, Green, U(255, 214, 90), U(172, 152, 204), U(132, 176, 212) };
		const int32 NConf = GM->VfxLevel == 2 ? 40 : (GM->VfxLevel == 1 ? 15 : 0);
		for (int32 i = 0; i < NConf; ++i)
		{
			const float Ang = FMath::FRandRange(-PI, 0.f);
			const float Sp = FMath::FRandRange(220.f, 520.f) * S;
			Confetti.Add({ P, FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * Sp, Pal[i % 6], FMath::FRandRange(0.f, 6.f) });
		}
	}
	for (FConfetti& C : Confetti)
	{
		C.V.Y += 900.f * S * Dt;
		C.V *= 1.f - 1.2f * Dt;
		C.P += C.V * Dt;
		C.Rot += Dt * 9.f;
		const FVector2D Ax(FMath::Cos(C.Rot) * 6.f * S, FMath::Sin(C.Rot) * 6.f * S);
		const FVector2D Ay(-FMath::Sin(C.Rot) * 3.5f * S, FMath::Cos(C.Rot) * 3.5f * S);
		Tri(C.P - Ax - Ay, C.P + Ax - Ay, C.P + Ax + Ay, C.C);
		Tri(C.P - Ax - Ay, C.P + Ax + Ay, C.P - Ax + Ay, C.C);
	}
	Confetti.RemoveAll([this](const FConfetti& C) { return C.P.Y > H + 20.f; });
}

// ---------------------------------------------------------------- etkileşimli tutorial: "Bu tuşa bas"

FBox2D ADBHUD::DoTargetRect() const
{
	switch (GM->DoLineTarget())
	{
	case 1: return TgScanner;
	case 2: return TgTools;
	case 3: return TgDrawer;
	case 4: return TgTurnR;
	default: return FBox2D(ForceInit);
	}
}

void ADBHUD::DrawDoHint()
{
	if (GM->DoLineTarget() == 0 || !GM->IsLineComplete())
	{
		return;
	}
	const FBox2D T = DoTargetRect();
	if (!T.bIsValid)
	{
		return;
	}
	// Hedef: dışa yayılan halkalar (nabız) + hafif büyüyüp küçülen sarı çerçeve
	const float Pulse = 0.5f + 0.5f * FMath::Sin(FxNow * 6.f);
	Frame(T.ExpandBy((6.f + 4.f * Pulse) * S), U(255, 214, 90, uint8(160 + 90 * Pulse)), 5.f * S);
	if (GM->DoLineTarget() == 4 && TgTurnL.bIsValid)
	{
		Frame(TgTurnL.ExpandBy((6.f + 4.f * Pulse) * S), U(255, 214, 90, uint8(160 + 90 * Pulse)), 5.f * S);
	}
	for (int32 k = 0; k < 2; ++k)
	{
		const float Ph = FMath::Fmod(FxNow * 0.9f + k * 0.5f, 1.f);
		Frame(T.ExpandBy((10.f + 40.f * Ph) * S), U(255, 214, 90, uint8(200 * (1.f - Ph))), 3.f * S);
	}
	// Etiket: hedefin üstünde, yanlış yere tıklanınca sallanır
	const float Miss = FxNow - DoMissTime;
	const float Shake = Miss < 0.45f ? FMath::Sin(Miss * 60.f) * 10.f * S * (1.f - Miss / 0.45f) : 0.f;
	const FString Lbl = GM->DoLineTarget() == 4 ? TEXT("Oklara ya da Q / E tuşuna bas") : TEXT("Bu tuşa bas");
	const float LW = Measure(Lbl, 24.f * S).X + 36.f * S;
	FVector2D C(T.GetCenter().X + Shake, T.Min.Y - 58.f * S - 6.f * S * Pulse);
	C.X = FMath::Clamp(C.X, LW * 0.5f + 10.f * S, W - LW * 0.5f - 10.f * S);
	const FBox2D L = Rect(C.X - LW * 0.5f, C.Y - 22.f * S, LW, 44.f * S);
	RoundCard(L, Miss < 0.45f ? U(250, 222, 212) : U(255, 214, 90), Ink, 14.f * S, 4.f);
	Txt(Lbl, L.GetCenter().X, L.Min.Y + 7.f * S, 24.f * S, Ink, 1);
	const FVector2D Tip(FMath::Clamp(T.GetCenter().X, L.Min.X + 20.f * S, L.Max.X - 20.f * S), T.Min.Y - 10.f * S);
	Tri(FVector2D(Tip.X - 12.f * S, L.Max.Y - 1.f), FVector2D(Tip.X + 12.f * S, L.Max.Y - 1.f), FVector2D(Tip.X, Tip.Y - 6.f * S * Pulse), Ink);
}

// ---------------------------------------------------------------- 2D efekt yardımcıları (hikâye, mini oyun)

void ADBHUD::DrawTexUV(UTexture2D* T, const FBox2D& R, const FVector2D& UV0, const FVector2D& UV1, float Alpha)
{
	if (!T || !T->GetResource())
	{
		return;
	}
	FCanvasTileItem It(R.Min, T->GetResource(), R.GetSize(), UV0, UV1, Gn(FLinearColor(1.f, 1.f, 1.f, Alpha)));
	It.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(It);
}

void ADBHUD::Sparkles(const FBox2D& Area, int32 N, float T, float Alpha, uint32 Seed)
{
	// Yavaşça yükselen, yanıp sönen küçük ışıltılar (tohumlu: her karede aynı parçacıklar sürekli hareket eder)
	if (GM->VfxLevel == 0)
	{
		return;
	}
	N = GM->VfxLevel == 1 ? N / 2 : N;
	FRandomStream Rs(Seed);
	const FVector2D Sz = Area.GetSize();
	for (int32 i = 0; i < N; ++i)
	{
		const float X0 = Rs.FRand(), Y0 = Rs.FRand(), Sp = Rs.FRandRange(0.03f, 0.09f), Ph = Rs.FRand() * 6.f;
		const float Y = FMath::Fmod(Y0 - T * Sp + 10.f, 1.f);
		const float X = X0 + 0.02f * FMath::Sin(T * 1.3f + Ph);
		const float Tw = 0.5f + 0.5f * FMath::Sin(T * 3.f + Ph);
		const FVector2D P(Area.Min.X + X * Sz.X, Area.Min.Y + Y * Sz.Y);
		const float Rr = (1.5f + 2.5f * Tw) * S;
		Disc(P, Rr, U(255, 240, 190, uint8(255 * Alpha * Tw * FMath::Sin(Y * PI))), 8);
	}
}

void ADBHUD::Fountain(const FVector2D& Origin, float Age, int32 N, const FLinearColor& Col, uint32 Seed)
{
	// Yukarı fışkırıp yay çizerek düşen damlalar (yaş 0-1,2 sn)
	if (Age < 0.f || Age > 1.2f || GM->VfxLevel == 0)
	{
		return;
	}
	FRandomStream Rs(Seed);
	for (int32 i = 0; i < N; ++i)
	{
		const float Ang = Rs.FRandRange(-PI * 0.85f, -PI * 0.15f);
		const float Sp = Rs.FRandRange(260.f, 560.f) * S;
		const float Delay = Rs.FRandRange(0.f, 0.25f);
		const float T = Age - Delay;
		if (T < 0.f)
		{
			continue;
		}
		const FVector2D P = Origin + FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * Sp * T + FVector2D(0.f, 900.f * S * T * T);
		const float A = FMath::Clamp(1.f - T / 1.f, 0.f, 1.f);
		Disc(P, (3.f + 3.f * A) * S, WithA(Col, A), 10);
	}
}

// ---------------------------------------------------------------- kasabalı portresi

namespace
{
	// Tools/sablon_listesi.py KASABALILAR ile aynı sıra (DBData C.Residents sırası)
	const TCHAR* ResidentKeys[] = { TEXT("AyseTeyze"), TEXT("SelmaOgretmen"), TEXT("HasanAmca"), TEXT("Zeynep"), TEXT("KemalBey"), TEXT("Elif"),
		TEXT("AliUsta"), TEXT("RizaAbi"), TEXT("Can"), TEXT("Deniz"), TEXT("MuratAbi"), TEXT("NerminHanim") };
}

void ADBHUD::DrawPortrait(int32 Resident, const FVector2D& C, float R)
{
	Disc(C, R + 3.f * S, Ink);
	Disc(C, R, U(244, 232, 208));
	if (Resident >= 0 && Resident < UE_ARRAY_COUNT(ResidentKeys))
	{
		if (UTexture2D* T = LoadPng(FString::Printf(TEXT("Karakterler/NPC_%s.png"), ResidentKeys[Resident])))
		{
			DrawTex(T, FBox2D(C - FVector2D(R), C + FVector2D(R)), 0.f, FVector2D(0.5f, 0.5f));
			return;
		}
	}
	// Yer tutucu: tahtadaki figürle aynı gömlek rengi, yüz ve saç (çizim gelince yerine oturur)
	const FLinearColor Shirts[] = { U(232, 128, 104), U(96, 170, 164), U(236, 188, 86), U(172, 152, 204), U(120, 180, 110), U(132, 176, 212) };
	const FLinearColor Shirt = Shirts[FMath::Max(0, Resident) % 6];
	// Gömlek: dairenin alt yayına oturan şekil (taşmaz): omuz çizgisi + alt yay, merkezden yelpaze
	{
		TArray<FVector2D> Pts;
		Pts.Add(C + FVector2D(-R * 0.66f, R * 0.52f));
		Pts.Add(C + FVector2D(R * 0.66f, R * 0.52f));
		for (int32 k = 0; k <= 16; ++k)
		{
			const float A = FMath::DegreesToRadians(FMath::Lerp(38.f, 142.f, k / 16.f));
			Pts.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R * 0.98f);
		}
		const FVector2D First = Pts[0];   // Pts.Add(Pts[0]) dizi büyürken kendi elemanını okuyup çöküyordu
		Pts.Add(First);
		const FVector2D Fc = C + FVector2D(0.f, R * 0.74f);
		for (int32 k = 0; k + 1 < Pts.Num(); ++k)
		{
			Tri(Fc, Pts[k], Pts[k + 1], Shirt);
		}
		Disc(C + FVector2D(-R * 0.5f, R * 0.6f), R * 0.17f, Shirt);
		Disc(C + FVector2D(R * 0.5f, R * 0.6f), R * 0.17f, Shirt);
	}
	Disc(C + FVector2D(0.f, R * 0.02f), R * 0.52f, (Resident & 1) ? U(70, 50, 36) : U(200, 200, 205));
	Disc(C + FVector2D(0.f, R * 0.12f), R * 0.46f, U(240, 200, 160));
	Disc(C + FVector2D(-R * 0.17f, R * 0.08f), R * 0.06f, Ink, 10);
	Disc(C + FVector2D(R * 0.17f, R * 0.08f), R * 0.06f, Ink, 10);
	Line(C + FVector2D(-R * 0.14f, R * 0.3f), C + FVector2D(0.f, R * 0.36f), 2.f * S, Ink);
	Line(C + FVector2D(0.f, R * 0.36f), C + FVector2D(R * 0.14f, R * 0.3f), 2.f * S, Ink);
}

void ADBHUD::DrawPortraitGallery()
{
	// Görsel testi: 12 kasabalı portresi tek karede (hepsi LoadPng'den geçer, rapora girer)
	const FDBContent& C = FDBContent::Get();
	Box(0.f, 0.f, W, H, U(40, 30, 22, 200));
	for (int32 i = 0; i < 12; ++i)
	{
		const FVector2D P(W * 0.5f + ((i % 6) - 2.5f) * 230.f * S, H * 0.5f + ((i / 6) - 0.5f) * 280.f * S);
		DrawPortrait(i, P, 80.f * S);
		if (C.Residents.IsValidIndex(i))
		{
			Txt(C.Residents[i].Name, P.X, P.Y + 96.f * S, 20.f * S, Paper, 1);
		}
	}
}

// ---------------------------------------------------------------- 17 Ekim ~20.00: başarımlar, teslim çantası, radar, albüm

void ADBHUD::Star(const FVector2D& C, float R, const FLinearColor& Col, float Rot)
{
	FVector2D Pts[10];
	for (int32 k = 0; k < 10; ++k)
	{
		const float A = Rot - PI * 0.5f + k * PI / 5.f;
		const float Rr = (k % 2 == 0) ? R : R * 0.45f;
		Pts[k] = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rr;
	}
	for (int32 k = 0; k < 10; ++k)
	{
		Tri(C, Pts[k], Pts[(k + 1) % 10], Col);
	}
}

void ADBHUD::DrawAchBadge(int32 Index, const FVector2D& C, float R, bool bOn)
{
	// Rozet: altın madalya + yıldız (açık), gri + kilit (kapalı)
	if (UTexture2D* T = LoadPng(bOn ? TEXT("Arayuz/ui_rozet.png") : TEXT("Arayuz/ui_rozet_kilit.png")))
	{
		const float Rr = R * 1.3f;
		DrawTex(T, Rect(C.X - Rr, C.Y - Rr, 2.f * Rr, 2.f * Rr), 0.f, FVector2D(0.5f, 0.5f), bOn ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.8f));
		return;
	}
	Disc(C + FVector2D(2.f, 3.f) * S, R + 3.f * S, ShadowC);
	Disc(C, R + 3.f * S, bOn ? U(150, 100, 30) : U(150, 140, 130));
	Disc(C, R, bOn ? U(246, 196, 64) : U(214, 206, 196));
	Disc(C, R * 0.78f, bOn ? U(255, 220, 110) : U(226, 218, 208));
	if (bOn)
	{
		Star(C, R * 0.6f, U(232, 128, 104), 0.f);
	}
	else
	{
		Txt(TEXT("?"), C.X, C.Y - R * 0.62f, R * 1.1f, U(160, 150, 140), 1);
	}
}

void ADBHUD::DrawAchievementPopup()
{
	if (GM->AchShowing < 0)
	{
		return;
	}
	const float Age = GM->Now - GM->AchShowTime;
	if (Age < 0.f || Age > ADBGameMode::AchShowSeconds)
	{
		return;
	}
	const ADBGameMode::FAchievement& A = ADBGameMode::Achievements()[GM->AchShowing];
	// Sağ üstten kayarak gelir, yaylanır; sonda sağa çekilir
	const float In = FMath::Clamp(Age / 0.3f, 0.f, 1.f);
	const float Out = FMath::Clamp((ADBGameMode::AchShowSeconds - Age) / 0.3f, 0.f, 1.f);
	const float Ease = 1.f - FMath::Pow(1.f - In, 3.f) + FMath::Sin(In * PI) * 0.06f;
	const float CW = 470.f * S, CH = 104.f * S;
	const float X = W - 20.f * S - CW + (1.f - FMath::Min(Ease, Out)) * (CW + 40.f * S);
	const FBox2D R = Rect(X, 96.f * S, CW, CH);
	RoundCard(R, U(255, 250, 236), U(200, 140, 40), 18.f * S, 6.f);
	// Parlama: kartın kenarında altın nabız
	Frame(R.ExpandBy((4.f + 3.f * FMath::Sin(Age * 10.f)) * S), U(255, 210, 60, uint8(120 * Out)), 3.f * S);
	const FVector2D Bc(R.Min.X + 56.f * S, R.GetCenter().Y);
	const float Pop = Age < 0.5f ? 1.f + 0.3f * FMath::Sin(Age / 0.5f * PI) : 1.f;
	// Rozetin arkasında dönen ışınlar
	for (int32 k = 0; k < 8; ++k)
	{
		const float Ang = Age * 1.5f + k * PI / 4.f;
		Tri(Bc, Bc + FVector2D(FMath::Cos(Ang - 0.12f), FMath::Sin(Ang - 0.12f)) * 52.f * S, Bc + FVector2D(FMath::Cos(Ang + 0.12f), FMath::Sin(Ang + 0.12f)) * 52.f * S,
			U(255, 214, 90, 90));
	}
	DrawAchBadge(GM->AchShowing, Bc, 34.f * S * Pop, true);
	Txt(TEXT("BAŞARIM AÇILDI"), R.Min.X + 104.f * S, R.Min.Y + 12.f * S, 15.f * S, Coral);
	Txt(A.Name, R.Min.X + 104.f * S, R.Min.Y + 32.f * S, 28.f * S, Ink);
	Txt(A.Desc, R.Min.X + 104.f * S, R.Min.Y + 68.f * S, 17.f * S, DarkK);
	Sparkles(R, 10, Age, Out, 41);
}

void ADBHUD::DrawBagsDoneHint()
{
	// Asil'in balonu: bütün çantalar teslim, masa nasıl kapanır
	const FBox2D P = PanelRect();
	const FBox2D BR = Rect(P.Min.X - 156.f * S - 600.f * S, H - 600.f * S, 600.f * S, 180.f * S);
	const FVector2D TailA(BR.Max.X - 120.f * S, BR.Max.Y - 2.f), TailB(BR.Max.X - 60.f * S, BR.Max.Y - 2.f), TailTip(P.Min.X - 120.f * S, H - 420.f * S);
	RoundCard(BR, Paper, Ink, 22.f * S, 8.f);
	Tri(TailA, TailB, TailTip, Paper);
	Line(TailA, TailTip, 2.5f * S, Ink);
	Line(TailB, TailTip, 2.5f * S, Ink);
	const FDBContent& C = FDBContent::Get();
	Disc(FVector2D(BR.Min.X + 32.f * S, BR.Min.Y + 30.f * S), 9.f * S, C.SpeakerColor[1]);
	Txt(C.SpeakerName[1] + TEXT("  ·  ") + C.SpeakerRole[1], BR.Min.X + 50.f * S, BR.Min.Y + 17.f * S, 22.f * S, DarkK);
	Para(TEXT("Bütün çantalar teslim, harika! Masayı kapatmak için kenardaki HAZIRLIK kulpuna bas (ya da Esc / sağ tık)."),
		BR.Min.X + 26.f * S, BR.Min.Y + 54.f * S, 24.f * S, BR.GetSize().X - 52.f * S, Ink, 1.2f);
	Button(TEXT("PanelClose"), -1, Rect(BR.Max.X - 186.f * S, BR.Max.Y - 54.f * S, 166.f * S, 42.f * S), TEXT("Masayı kapat"), 18.f * S, Coral, Ink, Paper);
	Blockers.Add(BR);
	// Kulp: nabız halkaları
	if (TgDrawer.bIsValid)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(FxNow * 6.f);
		Frame(TgDrawer.ExpandBy((6.f + 4.f * Pulse) * S), U(255, 214, 90, uint8(160 + 90 * Pulse)), 5.f * S);
		for (int32 k = 0; k < 2; ++k)
		{
			const float Ph = FMath::Fmod(FxNow * 0.9f + k * 0.5f, 1.f);
			Frame(TgDrawer.ExpandBy((10.f + 40.f * Ph) * S), U(255, 214, 90, uint8(200 * (1.f - Ph))), 3.f * S);
		}
	}
}

void ADBHUD::DrawRT(UTextureRenderTarget2D* T, const FBox2D& R)
{
	if (!T || !T->GetResource())
	{
		return;
	}
	// Sahne yakalamanın alfası anlamsız: opak çiz
	FCanvasTileItem It(R.Min, T->GetResource(), R.GetSize(), FLinearColor::White);
	It.BlendMode = SE_BLEND_Opaque;
	Canvas->DrawItem(It);
}

void ADBHUD::DrawPhoto(int32 Problem, const FBox2D& R)
{
	UTextureRenderTarget2D* T = GM->World ? GM->World->Photo(Problem) : nullptr;
	if (T)
	{
		DrawRT(T, R);
		return;
	}
	// Fotoğraf yoksa: gökyüzü, yer, bina silueti
	Box(R, U(176, 214, 236));
	Box(Rect(R.Min.X, R.Min.Y + R.GetSize().Y * 0.68f, R.GetSize().X, R.GetSize().Y * 0.32f), U(226, 204, 164));
	const FDBContent& C = FDBContent::Get();
	const FDBBuildingDef& B = C.Buildings[C.Problems[Problem].Building];
	const float BW = R.GetSize().X * 0.36f, BH = R.GetSize().Y * FMath::Clamp(B.Size.Z / 50.f, 0.25f, 0.6f);
	Box(Rect(R.GetCenter().X - BW * 0.5f, R.Min.Y + R.GetSize().Y * 0.7f - BH, BW, BH), B.Body);
	Tri(FVector2D(R.GetCenter().X - BW * 0.6f, R.Min.Y + R.GetSize().Y * 0.7f - BH), FVector2D(R.GetCenter().X + BW * 0.6f, R.Min.Y + R.GetSize().Y * 0.7f - BH),
		FVector2D(R.GetCenter().X, R.Min.Y + R.GetSize().Y * 0.7f - BH - BW * 0.4f), B.Roof);
}

void ADBHUD::DrawPhotoToast()
{
	// Oyunda: çözülen isteğin fotoğrafı çekilince sağda küçük bir polaroid belirir ("albüme eklendi")
	const float Age = GM->Now - GM->LastPhotoTime;
	if (GM->LastPhoto < 0 || Age < 0.f || Age > 2.6f || GM->bPanelOpen)
	{
		return;
	}
	const float In = FMath::Clamp(Age / 0.25f, 0.f, 1.f), Out = FMath::Clamp((2.6f - Age) / 0.3f, 0.f, 1.f);
	const float PW = 200.f * S, PH = 190.f * S;
	const float X = W - 24.f * S - PW + (1.f - FMath::Min(1.f - FMath::Pow(1.f - In, 3.f), Out)) * (PW + 40.f * S);
	const FBox2D R = Rect(X, 220.f * S, PW, PH);
	Box(R.ShiftBy(FVector2D(4.f, 6.f) * S), ShadowC);
	Box(R, U(252, 250, 244));
	const FBox2D Ph = Rect(R.Min.X + 10.f * S, R.Min.Y + 10.f * S, PW - 20.f * S, (PW - 20.f * S) * 0.75f);
	DrawPhoto(GM->LastPhoto, Ph);
	// Flaş: ilk anda beyaz, fotoğraf "banyo olur"
	const float Dev = FMath::Clamp(1.f - Age / 0.6f, 0.f, 1.f);
	if (Dev > 0.f)
	{
		Box(Ph, FLinearColor(1.f, 1.f, 1.f, Dev));
	}
	if (UTexture2D* Bant = LoadPng(TEXT("Arayuz/ui_bant.png")))
	{
		DrawTex(Bant, Rect(R.GetCenter().X - 48.f * S, R.Min.Y - 18.f * S, 96.f * S, 36.f * S), -4.f, FVector2D(0.5f, 0.5f));
	}
	else
	{
		Box(Rect(R.GetCenter().X - 30.f * S, R.Min.Y - 8.f * S, 60.f * S, 18.f * S), U(250, 232, 150, 200));   // bant
	}
	Txt(TEXT("Albüme eklendi"), R.GetCenter().X, Ph.Max.Y + 8.f * S, 17.f * S, Ink, 1);
}

void ADBHUD::DrawTray(const FVector2D& C, float Sc, int32 Kit, const TArray<int32>& Personal)
{
	// Bantta giden paket: karton kutu, kapakları açık; temel kitin eşyaları üstünden görünür, kişiye özel eşyalar önünde
	const FDBContent& Ct = FDBContent::Get();
	const FLinearColor Box1 = U(214, 170, 120), Box2 = U(190, 146, 98);
	Tri(FVector2D(C.X - 44.f * Sc, C.Y - 16.f * Sc), FVector2D(C.X - 30.f * Sc, C.Y - 16.f * Sc), FVector2D(C.X - 54.f * Sc, C.Y - 34.f * Sc), Box2);
	Tri(FVector2D(C.X + 44.f * Sc, C.Y - 16.f * Sc), FVector2D(C.X + 30.f * Sc, C.Y - 16.f * Sc), FVector2D(C.X + 54.f * Sc, C.Y - 34.f * Sc), Box2);
	if (Kit >= 0)
	{
		const TArray<int32>& D = Ct.Kits[Kit].Defaults;
		for (int32 i = 0; i < D.Num(); ++i)
		{
			ItemIcon(D[i], FVector2D(C.X + (i - (D.Num() - 1) * 0.5f) * 28.f * Sc, C.Y - 22.f * Sc), 13.f * Sc);
		}
	}
	const FBox2D Bx = Rect(C.X - 44.f * Sc, C.Y - 16.f * Sc, 88.f * Sc, 50.f * Sc);
	Box(Bx.ShiftBy(FVector2D(3.f, 4.f) * Sc), ShadowC);
	Box(Bx.ExpandBy(2.f * Sc), Ink);
	Box(Bx, Box1);
	Box(Rect(Bx.Min.X, Bx.Min.Y, Bx.GetSize().X, 6.f * Sc), Box2);
	if (Kit >= 0)
	{
		// Kit etiketi
		RoundBox(Rect(Bx.Min.X + 4.f * Sc, Bx.Max.Y - 16.f * Sc, 22.f * Sc, 12.f * Sc), 3.f * Sc, Ct.Kits[Kit].Color);
	}
	for (int32 i = 0; i < Personal.Num(); ++i)
	{
		ItemIcon(Personal[i], FVector2D(C.X + 8.f * Sc + (i - (Personal.Num() - 1) * 0.5f) * 26.f * Sc, C.Y + 10.f * Sc), 12.f * Sc);
	}
}

void ADBHUD::DrawDeliveryBag(const FVector2D& C, float Sc, const FLinearColor& Body, float Flap, bool bKitMark, int32 Kit)
{
	// Teslim çantası: sırt çantası; Flap 0 = kapak açık (yukarıda, ağzı görünür), 1 = kapalı (önüne inmiş)
	const FLinearColor Dark(Body.R * 0.72f, Body.G * 0.72f, Body.B * 0.72f, 1.f);
	const float TopY = C.Y - 38.f * Sc;
	const float Ang = Flap * PI * 0.92f;
	const float FL = 40.f * Sc;
	const float EdgeY = TopY - FL * FMath::Cos(Ang);
	const float EdgeW = 50.f * Sc + 6.f * Sc * FMath::Sin(Ang);
	auto DrawFlap = [&]()
	{
		const FVector2D H0(C.X - 46.f * Sc, TopY), H1(C.X + 46.f * Sc, TopY), E0(C.X - EdgeW, EdgeY), E1(C.X + EdgeW, EdgeY);
		Tri(H0, H1, E1, Ink);
		Tri(H0, E1, E0, Ink);
		const FVector2D In(0.f, (EdgeY < TopY ? 2.f : -2.f) * Sc);
		Tri(H0 + FVector2D(2.f * Sc, 0.f), H1 - FVector2D(2.f * Sc, 0.f), E1 + In - FVector2D(2.f * Sc, 0.f), Dark);
		Tri(H0 + FVector2D(2.f * Sc, 0.f), E1 + In - FVector2D(2.f * Sc, 0.f), E0 + In + FVector2D(2.f * Sc, 0.f), Dark);
	};
	// Askı (arkada)
	Line(FVector2D(C.X - 22.f * Sc, TopY), FVector2D(C.X - 16.f * Sc, TopY - 22.f * Sc), 6.f * Sc, Ink);
	Line(FVector2D(C.X + 22.f * Sc, TopY), FVector2D(C.X + 16.f * Sc, TopY - 22.f * Sc), 6.f * Sc, Ink);
	Line(FVector2D(C.X - 17.f * Sc, TopY - 22.f * Sc), FVector2D(C.X + 17.f * Sc, TopY - 22.f * Sc), 6.f * Sc, Ink);
	if (Flap < 0.55f)
	{
		DrawFlap();
	}
	// Gövde
	const FBox2D Bd = Rect(C.X - 48.f * Sc, TopY, 96.f * Sc, 82.f * Sc);
	RoundBox(Bd.ShiftBy(FVector2D(3.f, 5.f) * Sc), 16.f * Sc, ShadowC);
	RoundBox(Bd.ExpandBy(3.f * Sc), 18.f * Sc, Ink);
	RoundBox(Bd, 16.f * Sc, Body);
	// Ağız (kapak açıkken içi karanlık)
	if (Flap < 0.55f)
	{
		RoundBox(Rect(C.X - 40.f * Sc, TopY - 6.f * Sc, 80.f * Sc, 16.f * Sc), 8.f * Sc, U(58, 42, 30, 230));
	}
	// Ön cep + fermuar
	const FBox2D Pk = Rect(C.X - 30.f * Sc, TopY + 40.f * Sc, 60.f * Sc, 32.f * Sc);
	RoundBox(Pk.ExpandBy(2.f * Sc), 9.f * Sc, Ink);
	RoundBox(Pk, 8.f * Sc, Dark);
	Line(FVector2D(Pk.Min.X + 6.f * Sc, Pk.Min.Y + 7.f * Sc), FVector2D(Pk.Max.X - 6.f * Sc, Pk.Min.Y + 7.f * Sc), 2.f * Sc, U(250, 240, 220));
	if (bKitMark && Kit >= 0)
	{
		if (!KitIcon(Kit, Pk.GetCenter() + FVector2D(0.f, 4.f * Sc), 11.f * Sc))
		{
			Box(Rect(Pk.GetCenter().X - 9.f * Sc, Pk.GetCenter().Y + 1.f * Sc, 18.f * Sc, 6.f * Sc), U(250, 250, 250));
			Box(Rect(Pk.GetCenter().X - 3.f * Sc, Pk.GetCenter().Y - 5.f * Sc, 6.f * Sc, 18.f * Sc), U(250, 250, 250));
		}
	}
	if (Flap >= 0.55f)
	{
		DrawFlap();
		Disc(FVector2D(C.X, EdgeY - 2.f * Sc), 4.f * Sc, U(240, 200, 90));   // toka
	}
}

void ADBHUD::DrawRadar(const FVector2D& C, float R, bool bScanning)
{
	// Yuvarlak radar ekranı: halkalar, artı, dönen tarama kanadı, bulunan risklerde yanıp sönen bip'ler
	Disc(C, R + 4.f * S, Ink);
	Disc(C, R, U(16, 46, 38));
	auto Ring = [&](float Rr, const FLinearColor& Col, float T)
	{
		const int32 N = 40;
		for (int32 k = 0; k < N; ++k)
		{
			const float A0 = 2.f * PI * k / N, A1 = 2.f * PI * (k + 1) / N;
			Line(C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * Rr, C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * Rr, T, Col);
		}
	};
	Ring(R * 0.36f, U(90, 200, 150, 90), 1.2f * S);
	Ring(R * 0.7f, U(90, 200, 150, 90), 1.2f * S);
	Line(C - FVector2D(R, 0.f), C + FVector2D(R, 0.f), 1.f * S, U(90, 200, 150, 70));
	Line(C - FVector2D(0.f, R), C + FVector2D(0.f, R), 1.f * S, U(90, 200, 150, 70));
	const float Sweep = GM->Now * (bScanning ? 7.f : 1.8f);
	const int32 Fan = 16;
	for (int32 k = 0; k < Fan; ++k)
	{
		const float A0 = Sweep - 1.1f * (k + 1) / Fan, A1 = Sweep - 1.1f * k / Fan;
		const uint8 Al = (uint8)(140.f * (1.f - (float)k / Fan));
		Tri(C, C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * R, C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * R, U(120, 255, 170, Al));
	}
	Line(C, C + FVector2D(FMath::Cos(Sweep), FMath::Sin(Sweep)) * R, 2.f * S, U(200, 255, 220));
	for (int32 p = 0; p < GM->Revealed.Num(); ++p)
	{
		if (!GM->Revealed[p])
		{
			continue;
		}
		const float A = p * 2.39996f;
		const float D = (0.3f + 0.6f * FMath::Frac(p * 0.618f)) * R;
		const float Since = FMath::Fmod(FMath::Fmod(Sweep - A, 2.f * PI) + 2.f * PI, 2.f * PI);
		const float Glow2 = FMath::Exp(-Since * 1.1f);
		const bool bDone = GM->Solved.IsValidIndex(p) && GM->Solved[p];
		const FLinearColor Col = bDone ? U(120, 255, 140) : U(255, 90, 70);
		const FVector2D Bp = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * D;
		Disc(Bp, (2.5f + 2.5f * Glow2) * S, WithA(Col, 0.45f + 0.55f * Glow2));
	}
	Disc(C, 2.5f * S, U(200, 255, 220));
	// Cam parlaması
	Tri(C + FVector2D(-R * 0.8f, -R * 0.2f), C + FVector2D(-R * 0.2f, -R * 0.85f), C + FVector2D(-R * 0.55f, -R * 0.65f), U(255, 255, 255, 40));
}

void ADBHUD::DrawResult()
{
	// Andaç (17 Ekim ~19.30): sonuç ekranı baştan; yapılanlar fotoğraf albümü (polaroid) olarak görünür.
	// Fotoğraflar sırayla "banyo olur" (deklanşör sesi), üstüne gelince "neden önemli" altta yazar.
	const FDBContent& C = FDBContent::Get();
	if (ResultStart < 0.f)
	{
		ResultStart = FxNow;
		ResultPhotosShown = 0;
	}
	const float T = FxNow - ResultStart;
	Box(0.f, 0.f, W, H, U(30, 20, 10, 150));
	Blockers.Add(Rect(0.f, 0.f, W, H));

	// Albüm: kraft kâğıt, kenarda dikiş, serpiştirilmiş lifler
	const float AW = FMath::Min(1800.f * S, W - 30.f * S);
	const float Rise = FMath::Pow(1.f - FMath::Clamp(T / 0.45f, 0.f, 1.f), 3.f) * 300.f * S;
	const FBox2D R = Rect(W * 0.5f - AW * 0.5f, 26.f * S + Rise, AW, H - 52.f * S);
	RoundCard(R, U(206, 172, 128), U(120, 84, 56), 26.f * S, 10.f);
	if (UTexture2D* Kraft = LoadPng(TEXT("Arayuz/ui_kraft.png")); Kraft && Kraft->GetResource())
	{
		// Döşenen kraft dokusu (kenarları dikişsiz): ~420 px'lik karolar
		const FBox2D In = R.ExpandBy(-14.f * S);
		const float Tile = 420.f * S;
		FCanvasTileItem It(In.Min, Kraft->GetResource(), In.GetSize(), FVector2D(0.f, 0.f), In.GetSize() / Tile, Gn(FLinearColor::White));
		It.BlendMode = SE_BLEND_Opaque;
		Canvas->DrawItem(It);
	}
	else
	{
		RoundBox(R.ExpandBy(-14.f * S), 18.f * S, U(214, 182, 140));
		for (int32 k = 0; k < 140; ++k)
		{
			const float Fx = FMath::Frac(FMath::Sin(k * 12.9898f) * 43758.5453f), Fy = FMath::Frac(FMath::Sin(k * 78.233f) * 12543.1234f);
			Box(Rect(R.Min.X + 20.f * S + Fx * (AW - 40.f * S), R.Min.Y + 20.f * S + Fy * (R.GetSize().Y - 40.f * S), 6.f * S, 1.5f * S), U(180, 146, 104, 120));
		}
	}
	for (float Xd = R.Min.X + 30.f * S; Xd < R.Max.X - 30.f * S; Xd += 22.f * S)
	{
		Box(Rect(Xd, R.Min.Y + 22.f * S, 12.f * S, 2.f * S), U(250, 240, 220, 150));
		Box(Rect(Xd, R.Max.Y - 24.f * S, 12.f * S, 2.f * S), U(250, 240, 220, 150));
	}
	const float CX = R.GetCenter().X;

	// Başlık şeridi (kurdele) + alt başlık
	const bool bWon = GM->bWon;
	const FString Title = bWon ? TEXT("Kasaba afetlere hazır!") : TEXT("Kasaba henüz hazır değil");
	const float TW = Measure(Title, 52.f * S).X + 90.f * S;
	const FBox2D Rib = Rect(CX - TW * 0.5f, R.Min.Y + 38.f * S, TW, 74.f * S);
	Tri(FVector2D(Rib.Min.X - 30.f * S, Rib.Min.Y + 12.f * S), FVector2D(Rib.Min.X, Rib.Min.Y + 12.f * S), FVector2D(Rib.Min.X - 14.f * S, Rib.Max.Y - 2.f * S), U(180, 80, 62));
	Tri(FVector2D(Rib.Max.X + 30.f * S, Rib.Min.Y + 12.f * S), FVector2D(Rib.Max.X, Rib.Min.Y + 12.f * S), FVector2D(Rib.Max.X + 14.f * S, Rib.Max.Y - 2.f * S), U(180, 80, 62));
	RoundCard(Rib, bWon ? Coral : U(160, 120, 100), Ink, 12.f * S, 6.f);
	Txt(Title, CX, Rib.Min.Y + 8.f * S, 52.f * S, Paper, 1);
	Star(FVector2D(Rib.Min.X + 30.f * S, Rib.GetCenter().Y), 16.f * S, U(255, 220, 110), T);
	Star(FVector2D(Rib.Max.X - 30.f * S, Rib.GetCenter().Y), 16.f * S, U(255, 220, 110), -T);
	Txt(TEXT("Kasabanın albümü: bu oyunda yaptıkların"), CX, Rib.Max.Y + 10.f * S, 22.f * S, U(90, 62, 40), 1);

	// İstatistik çipleri
	{
		const FString Chips[] = {
			FString::Printf(TEXT("Hazırlık %%%d"), FMath::RoundToInt(GM->HazirlikYuzde())),
			FString::Printf(TEXT("Farkındalık %%%d"), FMath::RoundToInt(GM->FarkindalikYuzde())),
			FString::Printf(TEXT("Bütçe %d TL"), GM->Money),
			FString::Printf(TEXT("Hata %d"), GM->Mistakes),
			FString::Printf(TEXT("Başarım %d / %d"), GM->NumAchieved(), ADBGameMode::Achievements().Num()),
		};
		const FLinearColor Dots[] = { Green, Teal, U(236, 188, 86), Red, U(246, 196, 64) };
		float Total = 0.f;
		float Ws[5];
		for (int32 i = 0; i < 5; ++i) { Ws[i] = Measure(Chips[i], 20.f * S).X + 52.f * S; Total += Ws[i] + 14.f * S; }
		float X = CX - (Total - 14.f * S) * 0.5f;
		for (int32 i = 0; i < 5; ++i)
		{
			const float Ap = FMath::Clamp((T - 0.3f - i * 0.07f) / 0.2f, 0.f, 1.f);
			const FBox2D Ch = Rect(X, R.Min.Y + 154.f * S + (1.f - Ap) * 14.f * S, Ws[i], 38.f * S);
			RoundCard(Ch, WithA(Paper, Ap), WithA(DarkK, Ap), 19.f * S, 3.f * Ap);
			Disc(FVector2D(Ch.Min.X + 20.f * S, Ch.GetCenter().Y), 8.f * S, WithA(Dots[i], Ap));
			Txt(Chips[i], Ch.Min.X + 36.f * S, Ch.Min.Y + 6.f * S, 20.f * S, WithA(Ink, Ap));
			X += Ws[i] + 14.f * S;
		}
	}

	// Polaroidler: 4 × 2, sırayla belirir (deklanşör), fotoğraf beyazdan "banyo olur"
	const int32 N = C.Problems.Num();
	const int32 Cols = 4;
	const float PW = 280.f * S, PH = 252.f * S, Gap = 34.f * S;
	const float GX = CX - (Cols * PW + (Cols - 1) * Gap) * 0.5f;
	const float GY = R.Min.Y + 210.f * S;
	ResultHover = -1;
	for (int32 p = 0; p < N; ++p)
	{
		const float At = 0.55f + p * 0.24f;
		const float Age = T - At;
		if (Age < 0.f)
		{
			continue;
		}
		if (p >= ResultPhotosShown)
		{
			ResultPhotosShown = p + 1;
			GM->PlaySfx(EDBSfx::Shutter, 0.6f, ((p % Cols) - 1.5f) * 0.3f, FMath::FRandRange(0.95f, 1.05f));
		}
		const int32 Cx = p % Cols, Cy = p / Cols;
		const float Jit = FMath::Sin(p * 2.7f) * 8.f * S;
		FBox2D Pr = Rect(GX + Cx * (PW + Gap), GY + Cy * (PH + 16.f * S) + Jit, PW, PH);
		// İniş: büyükten küçülerek yerine oturur
		const float In = FMath::Clamp(Age / 0.3f, 0.f, 1.f);
		const float Sc = 1.f + 0.25f * FMath::Pow(1.f - In, 2.f) - FMath::Sin(In * PI) * 0.03f;
		Pr = FBox2D(Pr.GetCenter() - Pr.GetExtent() * Sc, Pr.GetCenter() + Pr.GetExtent() * Sc);
		const bool bHov = Hover(Pr) && In >= 1.f;
		if (bHov)
		{
			ResultHover = p;
			Pr = Pr.ShiftBy(FVector2D(0.f, -6.f * S));
		}
		Box(Pr.ShiftBy(FVector2D(5.f, 8.f) * S), U(60, 40, 25, 110));
		Box(Pr, U(252, 250, 244));
		if (bHov) Frame(Pr.ExpandBy(3.f * S), U(255, 210, 60), 3.f * S);
		const FBox2D Ph = Rect(Pr.Min.X + 12.f * Sc * S, Pr.Min.Y + 12.f * Sc * S, Pr.GetSize().X - 24.f * Sc * S, (Pr.GetSize().X - 24.f * Sc * S) * 0.7f);
		DrawPhoto(p, Ph);
		const float Dev = FMath::Clamp(1.f - Age / 0.9f, 0.f, 1.f);
		if (Dev > 0.f)
		{
			Box(Ph, FLinearColor(1.f, 0.98f, 0.92f, Dev));
		}
		// Bant parçası (üstte, biraz eğik görünsün diye iki üçgenle)
		const FVector2D Tc(Pr.GetCenter().X + FMath::Sin(p * 1.3f) * 30.f * S, Pr.Min.Y);
		if (UTexture2D* Bant = LoadPng(TEXT("Arayuz/ui_bant.png")))
		{
			DrawTex(Bant, Rect(Tc.X - 58.f * S, Tc.Y - 22.f * S, 116.f * S, 44.f * S), FMath::Sin(p * 2.1f) * 7.f, FVector2D(0.5f, 0.5f), FLinearColor(1.f, 1.f, 1.f, 0.92f));
		}
		else
		{
			Tri(Tc + FVector2D(-40.f, -10.f) * S, Tc + FVector2D(40.f, -4.f) * S, Tc + FVector2D(38.f, 14.f) * S, U(250, 232, 150, 190));
			Tri(Tc + FVector2D(-40.f, -10.f) * S, Tc + FVector2D(38.f, 14.f) * S, Tc + FVector2D(-42.f, 8.f) * S, U(250, 232, 150, 190));
		}
		// Afet etiketi + tik
		const FDBProblemDef& Pd = C.Problems[p];
		const float HW = Measure(Pd.Hazard, 15.f * S).X + 18.f * S;
		RoundBox(Rect(Ph.Min.X + 8.f * S, Ph.Min.Y + 8.f * S, HW, 24.f * S), 12.f * S, U(58, 42, 30, 200));
		Txt(Pd.Hazard, Ph.Min.X + 17.f * S, Ph.Min.Y + 11.f * S, 15.f * S, Paper);
		const bool bS = GM->Solved.IsValidIndex(p) && GM->Solved[p];
		const FVector2D Tk(Ph.Max.X - 22.f * S, Ph.Max.Y - 22.f * S);
		const float TkPop = FMath::Clamp((Age - 0.6f) / 0.25f, 0.f, 1.f);
		if (TkPop > 0.f)
		{
			const float TkS = TkPop + FMath::Sin(TkPop * PI) * 0.4f;
			Disc(Tk, 20.f * S * TkS, Paper);
			Disc(Tk, 17.f * S * TkS, bS ? Green : Red);
			if (bS) TickMark(Tk, 20.f * S * TkS, Paper); else Cross(Tk, 18.f * S * TkS, Paper);
		}
		// Altyazı: bina + yapılan iş (el yazısı gibi koyu kahve)
		Txt(C.Buildings[Pd.Building].Name, Pr.GetCenter().X, Ph.Max.Y + 6.f * S, 22.f * S, Ink, 1);
		FString What;
		if (Pd.Kit >= 0) What = C.Kits[Pd.Kit].Name + TEXT(" teslim");
		else if (C.Tools.IsValidIndex(Pd.Tool)) What = C.Tools[Pd.Tool].Done;
		Txt(What, Pr.GetCenter().X, Ph.Max.Y + 34.f * S, 16.f * S, DarkK, 1);
	}

	// Alt şerit: üstüne gelinen fotoğrafın dersi
	const float LY = GY + 2.f * PH + 16.f * S + 12.f * S;
	const FBox2D Ls = Rect(CX - 760.f * S, LY, 1520.f * S, 58.f * S);
	if (T > 0.55f + N * 0.24f)
	{
		RoundCard(Ls, NoteY, DarkK, 12.f * S, 4.f);
		if (ResultHover >= 0)
		{
			const FDBProblemDef& Pd = C.Problems[ResultHover];
			Txt(TEXT("Neden önemli?"), Ls.Min.X + 18.f * S, Ls.Min.Y + 6.f * S, 16.f * S, Coral);
			Para(Pd.Lesson, Ls.Min.X + 18.f * S, Ls.Min.Y + 26.f * S, 17.f * S, Ls.GetSize().X - 36.f * S, Ink, 1.1f, -1, Ls.Max.Y);
		}
		else
		{
			Txt(TEXT("Bir fotoğrafın üstüne gel: o önlemin neden önemli olduğunu oku."), Ls.GetCenter().X, Ls.Min.Y + 18.f * S, 20.f * S, DarkK, 1);
		}
	}

	// Başarımlar: rozet sırası
	const int32 NA = ADBGameMode::Achievements().Num();
	const float BY0 = Ls.Max.Y + 10.f * S;
	Txt(FString::Printf(TEXT("Başarımlar  %d / %d"), GM->NumAchieved(), NA), R.Min.X + 60.f * S, BY0 + 26.f * S, 24.f * S, Ink);
	for (int32 i = 0; i < NA; ++i)
	{
		const float Ap = FMath::Clamp((T - 1.0f - N * 0.24f - i * 0.08f) / 0.25f, 0.f, 1.f);
		if (Ap <= 0.f)
		{
			continue;
		}
		const FVector2D Bc(R.Min.X + 330.f * S + i * 172.f * S, BY0 + 34.f * S);
		const bool bOn = GM->AchUnlocked.IsValidIndex(i) && GM->AchUnlocked[i];
		DrawAchBadge(i, Bc, 24.f * S * (Ap + FMath::Sin(Ap * PI) * 0.3f), bOn);
		Txt(ADBGameMode::Achievements()[i].Name, Bc.X, Bc.Y + 30.f * S, 15.f * S, bOn ? Ink : U(150, 130, 110), 1);
		if (FVector2D::Distance(Mouse, Bc) < 26.f * S)
		{
			const FString Tip = ADBGameMode::Achievements()[i].Desc;
			const float TW2 = Measure(Tip, 16.f * S).X + 24.f * S;
			const FBox2D TR = Rect(Bc.X - TW2 * 0.5f, Bc.Y - 66.f * S, TW2, 32.f * S);
			RoundCard(TR, Paper, Ink, 10.f * S, 3.f);
			Txt(Tip, TR.Min.X + 12.f * S, TR.Min.Y + 6.f * S, 16.f * S, Ink);
		}
	}

	// Düğmeler
	const float BY = R.Max.Y - 92.f * S;
	Button(TEXT("Again"), -1, Rect(CX - 290.f * S, BY, 270.f * S, 62.f * S), TEXT("Tekrar Oyna"), 26.f * S, Coral, Ink, Paper);
	Button(TEXT("Menu"), -1, Rect(CX + 20.f * S, BY, 270.f * S, 62.f * S), TEXT("Menü"), 26.f * S, CardC, DarkK, Ink);
}
