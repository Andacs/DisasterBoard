#pragma once

#include "CoreMinimal.h"

// Oyunun bütün içeriği (çanta türleri, eşyalar, şehir önlemleri, binalar, sorunlar, konuşmalar, jenerik) DBData.cpp'de.
// Metin ya da denge değiştirmek için yalnızca o dosya düzenlenir.

enum class EDBPhase : uint8
{
	Menu,      // duvardaki takvimde ana menü
	MenuPage,  // takvimde Ayarlar / Emeği Geçenler sayfası
	ToBoard,   // kamera takvimden tahtaya dönüyor
	Comic,     // giriş çizgi romanı (4 sahne × 5 sn, tıkla: sonraki)
	Talk,      // A / B / C konuşuyor (tıkla: devam)
	WaitScan,  // tarayıcıya basılmasını bekliyor
	Scanning,  // tarama çizgisi tahtayı geçiyor
	BagPrep,   // ilk çanta hazırlığı (sağ panel açık)
	Play,      // önlem / çanta seç, binaya uygula
	Quake,     // deprem (bQuakeAtEnd kapalıyken kullanılmıyor)
	Result     // sonuç kartı
};

enum class EDBMenuPage : uint8 { None, Settings, Credits };

enum class EDBSpeaker : uint8 { A, B, C };

// Üç çanta türü: ilk yardım, deprem, yangın
struct FDBKitDef
{
	FString Name;          // "Deprem Çantası"
	FString Short;         // sekme ve sırt çantası etiketi
	FLinearColor Color;
};

struct FDBItemDef
{
	FString Name;
	FString Icon;          // kartta büyük yazılan kısaltma (emoji yok: fontta olmayan glif oyunu dondurur)
	FLinearColor Color;
	int32 Kit = -1;        // hangi çantaya ait
};

// Şehir önlemi: bedeli bütçeden düşer
struct FDBToolDef
{
	FString Name;
	FString Icon;
	FLinearColor Color;
	int32 Cost = 0;
	FString Hazard;        // Sel / Tsunami / Yangın / Deprem
};

struct FDBBuildingDef
{
	FString Name;
	FIntPoint Lot;         // 5×5 arsa (her arsa tahtada 2×2 karo); Y=4 sırası deniz
	FVector Size;          // tasarım ölçüsü (≈cm, kök BuildingScale ile küçülür); X,Y ≤ 20
	FLinearColor Body;
	FLinearColor Roof;
	bool bPark = false;    // düz yeşil alan + ağaç
	bool bShelter = false; // sığınak (tahliyenin hedefi)
};

struct FDBProblemDef
{
	int32 Building = -1;
	FString Hazard;        // not kağıdındaki etiket
	FString Short;         // not kağıdında tek satır
	FString Note;          // binaya seçimsiz tıklayınca
	int32 Tool = -1;       // >= 0: bu şehir önlemi gerekir
	int32 Kit = -1;        // >= 0: bu türde, eşyaları tam bir çanta gerekir
	FString Lesson;        // sonuç kartında: doğrusu ne
	FString Hint;          // görev listesinde: cevabı söylemeden yönlendiren ipucu
};

struct FDBLine
{
	EDBSpeaker Who;
	FString Text;
};

struct FDBContent
{
	TArray<FDBKitDef> Kits;
	TArray<FDBItemDef> Items;
	TArray<FDBToolDef> Tools;
	TArray<FDBBuildingDef> Buildings;
	TArray<FDBProblemDef> Problems;
	int32 ShelterBuilding = -1;
	int32 EvacuateTool = -1;   // bu önlem uygulanınca sakinler sığınağa yürür

	int32 StartMoney = 100;
	int32 KitReward = 30;      // doğru eve gönderilen her çanta

	TArray<FDBLine> IntroA;       // oyun başı: A tarayıcıyı gösterir
	TArray<FDBLine> AfterScan;    // tarama bitti: A
	TArray<FDBLine> BagIntroB;    // sağ panel: B
	TArray<FDBLine> PanelIntroC;  // sağ panel kapandı: C alt paneli anlatır

	FString SpeakerName[3];
	FString SpeakerRole[3];
	FLinearColor SpeakerColor[3];

	TArray<FString> Credits;
	TArray<FString> Comic;        // giriş çizgi romanı: sahne başına bir cümle (görsel: Content/Cizgiroman/sahneN.png)

	TArray<int32> KitItems(int32 Kit) const
	{
		TArray<int32> Out;
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			if (Items[i].Kit == Kit)
			{
				Out.Add(i);
			}
		}
		return Out;
	}

	static const FDBContent& Get();
};

// Karton paleti (sRGB'den çevrilir; MID "Color" parametresi doğrusal renk ister)
namespace DBColor
{
	inline FLinearColor RGB(uint8 R, uint8 G, uint8 B) { return FLinearColor(FColor(R, G, B)); }
	inline FColor UI(uint8 R, uint8 G, uint8 B, uint8 A = 255) { return FColor(R, G, B, A); }
}
