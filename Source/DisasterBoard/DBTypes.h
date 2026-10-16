#pragma once

#include "CoreMinimal.h"

// Oyunun bütün içeriği (eşyalar, şehir araçları, binalar, sorunlar, konuşmalar, jenerik) DBData.cpp'de.
// Metin ya da denge değiştirmek için yalnızca o dosya düzenlenir.

enum class EDBPhase : uint8
{
	Menu,      // duvardaki takvimde ana menü
	MenuPage,  // takvimde Ayarlar / Emeği Geçenler sayfası
	ToBoard,   // kamera takvimden tahtaya dönüyor
	Talk,      // A / B / C konuşuyor (tıkla: devam)
	WaitScan,  // tarayıcıya basılmasını bekliyor
	Scanning,  // tarama çizgisi tahtayı geçiyor
	BagPrep,   // ilk çanta hazırlığı (sağ panel açık, süre işlemiyor)
	Play,      // araç / çanta seç, binaya uygula; deprem geri sayımı
	Quake,     // deprem
	Result     // sonuç kartı
};

enum class EDBMenuPage : uint8 { None, Settings, Credits };

enum class EDBSpeaker : uint8 { A, B, C };

struct FDBItemDef
{
	FString Name;
	FString Icon;          // kartta büyük yazılan kısaltma (emoji yok: fontta olmayan glif oyunu dondurur)
	FLinearColor Color;
};

struct FDBToolDef
{
	FString Name;
	FString Icon;
	FLinearColor Color;
};

struct FDBBuildingDef
{
	FString Name;
	FIntPoint Cell;        // 0..GridN-1; X ekseni = sütun, Y ekseni = satır
	FVector Size;          // cm
	FLinearColor Body;
	FLinearColor Roof;
	bool bPark = false;    // düz yeşil alan + ağaç (gövde yok)
};

struct FDBProblemDef
{
	int32 Building = -1;
	FString Short;         // not kağıdında tek satır
	FString Note;          // binaya seçimsiz tıklayınca
	int32 Tool = -1;       // >= 0: bu şehir aracı gerekir
	TArray<int32> Items;   // Tool < 0: çantada bu eşyaların hepsi olmalı
	FString Lesson;        // sonuç kartında: doğrusu ne
};

struct FDBLine
{
	EDBSpeaker Who;
	FString Text;
};

struct FDBContent
{
	TArray<FDBItemDef> Items;
	TArray<FDBToolDef> Tools;
	TArray<FDBBuildingDef> Buildings;
	TArray<FDBProblemDef> Problems;

	TArray<FDBLine> IntroA;       // oyun başı: A tarayıcıyı gösterir
	TArray<FDBLine> AfterScan;    // tarama bitti: A
	TArray<FDBLine> BagIntroB;    // sağ panel: B
	TArray<FDBLine> PanelIntroC;  // sağ panel kapandı: C alt paneli anlatır

	FString SpeakerName[3];
	FString SpeakerRole[3];
	FLinearColor SpeakerColor[3];

	TArray<FString> Credits;

	static const FDBContent& Get();
};

// Karton paleti (sRGB'den çevrilir; MID "Color" parametresi doğrusal renk ister)
namespace DBColor
{
	inline FLinearColor RGB(uint8 R, uint8 G, uint8 B) { return FLinearColor(FColor(R, G, B)); }
	inline FColor UI(uint8 R, uint8 G, uint8 B, uint8 A = 255) { return FColor(R, G, B, A); }
}
