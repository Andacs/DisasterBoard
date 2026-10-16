#include "DBTypes.h"

// Oyunun içeriği. Metinler ekip tarafından değiştirilebilir; dizinler (eşya/araç/bina numaraları) birbirine bağlı.

namespace
{
	using DBColor::RGB;

	// Karton paleti
	const FLinearColor Coral = RGB(232, 128, 104);
	const FLinearColor Mustard = RGB(236, 188, 86);
	const FLinearColor Teal = RGB(96, 170, 164);
	const FLinearColor Sky = RGB(132, 176, 212);
	const FLinearColor Lavender = RGB(172, 152, 204);
	const FLinearColor Sage = RGB(150, 188, 120);
	const FLinearColor Cream = RGB(244, 232, 208);
	const FLinearColor Card = RGB(226, 200, 158);
	const FLinearColor Kraft = RGB(196, 154, 108);
	const FLinearColor DarkKraft = RGB(140, 100, 66);
	const FLinearColor Brick = RGB(186, 92, 70);

	enum EItem { Su, Konserve, IlkYardim, Ilac, Fener, Duduk, Battaniye, Mama };
	enum ETool { Toplanma, Sabitleme, YanginMuslugu, Guclendirme };
	enum EBina { AyseEv, Apartman, YilmazEv, OgrenciEv, Kitapci, Firin, Park, Okul, Market, Ev };

	FDBContent Build()
	{
		FDBContent C;

		C.Items = {
			{ TEXT("Su"), TEXT("SU"), Sky },
			{ TEXT("Konserve"), TEXT("KN"), Coral },
			{ TEXT("İlk Yardım"), TEXT("+"), RGB(220, 80, 70) },
			{ TEXT("Kişisel İlaç"), TEXT("İL"), Lavender },
			{ TEXT("El Feneri"), TEXT("FN"), Mustard },
			{ TEXT("Düdük"), TEXT("DÜ"), Teal },
			{ TEXT("Battaniye"), TEXT("BT"), Kraft },
			{ TEXT("Bebek Maması"), TEXT("MA"), RGB(236, 160, 180) },
		};

		// Çizimdeki alt panel sırası: (1) ? (2) ? (3) yangın musluğu (4) bina güçlendirme.
		// İlk iki ikonun anlamı ekipten teyit bekliyor.
		C.Tools = {
			{ TEXT("Toplanma Alanı"), TEXT("TA"), Sage },
			{ TEXT("Eşya Sabitleme"), TEXT("ES"), Mustard },
			{ TEXT("Yangın Musluğu"), TEXT("YM"), RGB(220, 80, 70) },
			{ TEXT("Bina Güçlendirme"), TEXT("BG"), Teal },
		};

		C.Buildings = {
			{ TEXT("Ayşe Teyze'nin evi"), FIntPoint(0, 1), FVector(18, 18, 20), Coral, DarkKraft },
			{ TEXT("Eski apartman"), FIntPoint(1, 3), FVector(20, 20, 62), Card, Brick },
			{ TEXT("Yılmazların evi"), FIntPoint(3, 1), FVector(20, 18, 24), Sky, Brick },
			{ TEXT("Öğrenci evi"), FIntPoint(4, 3), FVector(18, 20, 30), Lavender, DarkKraft },
			{ TEXT("Kitapçı"), FIntPoint(2, 2), FVector(20, 20, 22), Mustard, Brick },
			{ TEXT("Fırın"), FIntPoint(4, 0), FVector(20, 18, 18), Cream, Coral },
			{ TEXT("Park"), FIntPoint(0, 4), FVector(22, 22, 2), Sage, Sage, true },
			{ TEXT("Okul"), FIntPoint(1, 0), FVector(22, 20, 26), Teal, Brick },
			{ TEXT("Market"), FIntPoint(3, 4), FVector(18, 18, 18), Coral, Cream },
			{ TEXT("Ev"), FIntPoint(2, 0), FVector(18, 18, 22), Cream, Brick },
		};

		C.Problems = {
			{ AyseEv,
			  TEXT("Ayşe Teyze yalnız; kalp ilacı alıyor, karanlıkta zor görüyor."),
			  TEXT("Ayşe Teyze yalnız yaşıyor ve her gün kalp ilacı alıyor. Elektrik kesilince evde hiçbir şey göremiyor. Enkaz altında kalırsa sesini duyurması zor."),
			  -1, { Ilac, Fener, Duduk },
			  TEXT("Düzenli ilaç kullananların çantasında birkaç günlük ilaç olmalı. Düdük, enkaz altında sesini duyurmanın en az yorucu yolu.") },
			{ YilmazEv,
			  TEXT("Yılmazların 6 aylık bebeği var; evde su yok, geceler soğuk."),
			  TEXT("Yılmaz ailesinin 6 aylık bir bebeği var. Evde içme suyu stoğu yok ve geceler artık çok soğuk."),
			  -1, { Mama, Su, Battaniye },
			  TEXT("Bebekli aileler çantaya mama eklemeli. Su, evdeki kişi sayısına ve birkaç güne yetecek kadar olmalı.") },
			{ OgrenciEv,
			  TEXT("Öğrenci evinde ne yiyecek ne su var; ecza dolabı da yok."),
			  TEXT("Öğrenciler evde hiç yiyecek ve su tutmuyor. Küçük kesik ve yanıklar sık oluyor ama ecza dolabı bile yok."),
			  -1, { Konserve, Su, IlkYardim },
			  TEXT("Afet çantasında bozulmayan yiyecek, su ve ilk yardım malzemesi temel üçlüdür. Son kullanma tarihleri yılda bir kontrol edilmeli.") },
			{ Apartman,
			  TEXT("Eski apartmanın kolonlarında çatlaklar var."),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var. Bina deprem yönetmeliğinden önce yapılmış."),
			  Guclendirme, {},
			  TEXT("Riskli binalar uzman incelemesiyle güçlendirilmeli ya da yenilenmeli. Kolon kesmek, taşıyıcı duvar yıkmak hayati tehlikedir.") },
			{ Kitapci,
			  TEXT("Kitapçıdaki yüksek raflar duvara sabitlenmemiş."),
			  TEXT("Kitapçının yüksek rafları ve dolapları duvara sabitlenmemiş; sarsıntıda insanların üstüne devrilebilir."),
			  Sabitleme, {},
			  TEXT("Depremde yaralanmaların önemli bir kısmı devrilen eşyalardan olur. Dolap, raf ve televizyonları duvara sabitle.") },
			{ Firin,
			  TEXT("Fırının sokağında yangın musluğu yok."),
			  TEXT("Fırının sokağında yangın musluğu yok. Depremden sonra çıkan bir yangına müdahale edilemez."),
			  YanginMuslugu, {},
			  TEXT("Depremden sonra en sık görülen tehlikelerden biri yangındır. Sarsıntı bitince gazı ve elektriği kapat, söndürücünün yerini bil.") },
			{ Park,
			  TEXT("Mahallenin toplanma alanı belli değil."),
			  TEXT("Parkın bir toplanma alanı olması gerekiyor ama tabelası yok; kimse depremden sonra nereye gideceğini bilmiyor."),
			  Toplanma, {},
			  TEXT("Ailecek en yakın toplanma alanını önceden öğrenin ve buluşma planı yapın. Toplanma alanları e-Devlet'ten sorgulanabilir.") },
		};

		C.SpeakerName[0] = TEXT("Ayaz");
		C.SpeakerRole[0] = TEXT("Afet gönüllüsü");
		C.SpeakerColor[0] = Mustard;
		C.SpeakerName[1] = TEXT("Berna");
		C.SpeakerRole[1] = TEXT("Hazırlık masası");
		C.SpeakerColor[1] = Teal;
		C.SpeakerName[2] = TEXT("Cem");
		C.SpeakerRole[2] = TEXT("Belediye");
		C.SpeakerColor[2] = Coral;

		using S = EDBSpeaker;
		C.IntroA = {
			{ S::A, TEXT("Merhaba! Ben Ayaz, bu mahallenin afet gönüllüsüyüm.") },
			{ S::A, TEXT("Bir sorun var: kasabada deprem için hazır olmayan binalar var ve hangileri olduğunu bilmiyoruz.") },
			{ S::A, TEXT("Sol alttaki tarayıcıya bas, tahtayı birlikte tarayalım.") },
		};
		C.AfterScan = {
			{ S::A, TEXT("Uyarı işaretli binaları gördün mü? Her birinin bir eksiği var.") },
			{ S::A, TEXT("Berna sana hazırlık masasını gösterecek.") },
		};
		C.BagIntroB = {
			{ S::B, TEXT("Ben Berna. Burası hazırlık masası; afet çantalarını burada hazırlıyoruz.") },
			{ S::B, TEXT("Sağ alttaki not kağıdında tarayıcının bulduğu sorunlar yazıyor. Önce onu oku.") },
			{ S::B, TEXT("Eşyalara tıklayıp çantaya koy. Bir çantaya en fazla 4 eşya sığar; fazlası taşınamaz.") },
			{ S::B, TEXT("Her ev için ihtiyacına göre ayrı bir çanta hazırla ve 'Çantayı Hazırla'ya bas.") },
			{ S::B, TEXT("Bitince sağ üstteki X ile masayı kapat.") },
		};
		C.PanelIntroC = {
			{ S::C, TEXT("Ben Cem, belediyeden. Aşağıdaki paneli anlatayım.") },
			{ S::C, TEXT("Soldaki kutular şehir araçları: bunlar eve değil, binaya ve mahalleye yapılan düzenlemeler.") },
			{ S::C, TEXT("Sağdaki sırt çantasında senin hazırladığın çantalar duruyor.") },
			{ S::C, TEXT("Bir araç ya da çanta seç, sonra uyarı işaretli binaya tıkla. Doğruysa bina güvene alınır.") },
			{ S::C, TEXT("Acele et, deprem her an gelebilir!") },
		};

		C.Credits = {
			TEXT("FreakLabs"),
			TEXT(""),
			TEXT("Andaç: kod, oynanış"),
			TEXT("Mert: model, doku, ışık"),
			TEXT("Asil: konsept, model"),
			TEXT("Yiğit: arayüz, tasarım"),
			TEXT(""),
			TEXT("Unreal Engine 5.8 (Epic Games)"),
			TEXT("Kod yardımı: Claude (Anthropic)"),
			TEXT("CIVIL GAME JAM 2026"),
		};
		return C;
	}
}

const FDBContent& FDBContent::Get()
{
	static const FDBContent Content = Build();
	return Content;
}
