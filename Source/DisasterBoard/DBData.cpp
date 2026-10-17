#include "DBTypes.h"

// Oyunun içeriği. Metinler ekip tarafından değiştirilebilir; dizinler (çanta/eşya/önlem/bina numaraları) birbirine bağlı.
// Amaç: doğal afetlere (deprem, sel, yangın, tsunami) hazırlık ve farkındalık.

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
	const FLinearColor Rose = RGB(220, 140, 160);
	const FLinearColor Slate = RGB(110, 120, 140);

	enum EKit { IlkYardim, Deprem, Yangin };
	enum EItem { Su, SargiBezi, YanginTupu, Duduk, YaraBandi, DumanMaskesi, Konserve, Antiseptik, YanginOrtusu, Fener, Makas, Eldiven };
	enum ETool { Rogar, Tahliye, YanginMuslugu, Guclendirme };
	enum EBina { AyseEv, Okul, Siginak, YilmazEv, Apartman, Kitapci, Firin, Market, OgrenciEv, Park, SahilEv, AhsapEv };

	FDBContent Build()
	{
		FDBContent C;

		C.Kits = {
			{ TEXT("İlk Yardım Çantası"), TEXT("İlk Yardım"), RGB(214, 76, 66) },
			{ TEXT("Deprem Çantası"), TEXT("Deprem"), RGB(86, 132, 196) },
			{ TEXT("Yangın Çantası"), TEXT("Yangın"), RGB(232, 136, 52) },
		};

		// Sıra karışık: hangi eşyanın hangi çantaya ait olduğunu oyuncu bulur. Her çantaya tam 4 eşya.
		C.Items = {
			{ TEXT("Su"), TEXT("SU"), Sky, Deprem },
			{ TEXT("Sargı Bezi"), TEXT("SB"), Cream, IlkYardim },
			{ TEXT("Yangın Tüpü"), TEXT("YT"), RGB(214, 76, 66), Yangin },
			{ TEXT("Düdük"), TEXT("DÜ"), Teal, Deprem },
			{ TEXT("Yara Bandı"), TEXT("YB"), Rose, IlkYardim },
			{ TEXT("Duman Maskesi"), TEXT("DM"), Slate, Yangin },
			{ TEXT("Konserve"), TEXT("KN"), Coral, Deprem },
			{ TEXT("Antiseptik"), TEXT("AN"), Lavender, IlkYardim },
			{ TEXT("Yangın Örtüsü"), TEXT("YÖ"), Brick, Yangin },
			{ TEXT("El Feneri"), TEXT("FN"), Mustard, Deprem },
			{ TEXT("Makas"), TEXT("MK"), RGB(150, 150, 156), IlkYardim },
			{ TEXT("Eldiven"), TEXT("EL"), Kraft, Yangin },
		};

		// Alt panel sırası çizimdeki gibi: rögar kapağı, tahliye, yangın musluğu, bina güçlendirme
		C.Tools = {
			{ TEXT("Rögar Kapağı"), TEXT("RK"), RGB(80, 84, 92), 30, TEXT("Sel") },
			{ TEXT("Tahliye"), TEXT("TH"), Sky, 30, TEXT("Tsunami") },
			{ TEXT("Yangın Musluğu"), TEXT("YM"), RGB(214, 76, 66), 40, TEXT("Yangın") },
			{ TEXT("Bina Güçlendirme"), TEXT("BG"), Teal, 60, TEXT("Deprem") },
		};
		C.EvacuateTool = Tahliye;
		C.StartMoney = 100;
		C.KitReward = 30;

		C.Buildings = {
			{ TEXT("Ayşe Teyze'nin evi"), FIntPoint(0, 0), FVector(18, 18, 20), Coral, DarkKraft },
			{ TEXT("Okul"), FIntPoint(2, 0), FVector(20, 20, 26), Teal, Brick },
			{ TEXT("Sığınak"), FIntPoint(3, 0), FVector(20, 20, 14), Sage, RGB(110, 150, 90), false, true },
			{ TEXT("Yılmazların evi"), FIntPoint(4, 0), FVector(20, 18, 24), Sky, Brick },
			{ TEXT("Eski apartman"), FIntPoint(1, 1), FVector(20, 20, 62), Card, Brick },
			{ TEXT("Kitapçı"), FIntPoint(3, 1), FVector(20, 20, 22), Mustard, Brick },
			{ TEXT("Fırın"), FIntPoint(0, 2), FVector(20, 18, 18), Cream, Coral },
			{ TEXT("Market"), FIntPoint(2, 2), FVector(18, 18, 18), Coral, Cream },
			{ TEXT("Öğrenci evi"), FIntPoint(4, 2), FVector(18, 20, 30), Lavender, DarkKraft },
			{ TEXT("Park"), FIntPoint(0, 3), FVector(20, 20, 2), Sage, Sage, true },
			{ TEXT("Sahil evi"), FIntPoint(1, 3), FVector(18, 18, 20), Cream, Sky },
			{ TEXT("Ahşap evler"), FIntPoint(3, 3), FVector(20, 18, 20), Kraft, DarkKraft },
		};
		C.ShelterBuilding = Siginak;

		C.Problems = {
			{ AyseEv, TEXT("İlk yardım"),
			  TEXT("Ayşe Teyze yalnız yaşıyor; evde hiç ilk yardım malzemesi yok."),
			  TEXT("Ayşe Teyze yalnız yaşıyor ve sık sık küçük kazalar geçiriyor. Evde ilk yardım malzemesi yok."),
			  -1, IlkYardim,
			  TEXT("Her evde sargı bezi, yara bandı, antiseptik ve makas bulunan bir ilk yardım çantası olmalı; yerini herkes bilmeli.") },
			{ Okul, TEXT("İlk yardım"),
			  TEXT("Okulun revirindeki ilk yardım çantası boş."),
			  TEXT("Okulda yüzlerce öğrenci var ama revirdeki ilk yardım çantası bomboş."),
			  -1, IlkYardim,
			  TEXT("Okul ve iş yerlerinde ilk yardım çantası kolay ulaşılan bir yerde durmalı, malzemeleri düzenli yenilenmeli.") },
			{ YilmazEv, TEXT("Deprem"),
			  TEXT("Yılmazların deprem anında alacakları bir çantası yok."),
			  TEXT("Yılmaz ailesi depreme hiç hazırlık yapmamış; kapının yanında hazır bir çantaları yok."),
			  -1, Deprem,
			  TEXT("Deprem çantasında su, bozulmayan yiyecek, el feneri ve düdük olmalı; çanta kapının yakınında durmalı.") },
			{ OgrenciEv, TEXT("Deprem"),
			  TEXT("Öğrenci evinde depremden sonrası için hiçbir hazırlık yok."),
			  TEXT("Öğrenciler depremden sonraki ilk saatlerde ne yiyip içeceklerini hiç düşünmemiş."),
			  -1, Deprem,
			  TEXT("Deprem çantası evdeki kişi sayısına göre hazırlanmalı; su ve yiyeceklerin tarihleri yılda bir kontrol edilmeli.") },
			{ Firin, TEXT("Yangın"),
			  TEXT("Fırında sürekli ateş yanıyor ama yangın malzemesi yok."),
			  TEXT("Fırında sabahtan akşama ateş yanıyor; bir yangın çıkarsa müdahale edecek hiçbir şey yok."),
			  -1, Yangin,
			  TEXT("Ateşle çalışılan yerlerde yangın tüpü, yangın örtüsü, duman maskesi ve ısıya dayanıklı eldiven hazır olmalı.") },
			{ Apartman, TEXT("Deprem"),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var."),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var. Bina deprem yönetmeliğinden önce yapılmış."),
			  Guclendirme, -1,
			  TEXT("Riskli binalar uzman incelemesiyle güçlendirilmeli ya da yenilenmeli. Kolon kesmek, taşıyıcı duvar yıkmak hayati tehlikedir.") },
			{ Market, TEXT("Sel"),
			  TEXT("Yağmurda marketin sokağını su basıyor; rögar kapaksız."),
			  TEXT("Her yağmurda marketin önündeki sokağı su basıyor; rögar tıkalı ve kapağı yok."),
			  Rogar, -1,
			  TEXT("Sele karşı rögar ve kanallar temiz tutulmalı. Su basan yerde elektriği kapat, akan suya girme, yüksek bir yere çık.") },
			{ AhsapEv, TEXT("Yangın"),
			  TEXT("Ahşap evlerin sokağında yangın musluğu yok."),
			  TEXT("Birbirine bitişik ahşap evlerin sokağında yangın musluğu yok; yangın hızla yayılabilir."),
			  YanginMuslugu, -1,
			  TEXT("Yangın musluklarının önü kapatılmamalı, itfaiyenin geçeceği yollar açık tutulmalı.") },
			{ SahilEv, TEXT("Tsunami"),
			  TEXT("Sahil evinde oturanlar sığınağın yolunu bilmiyor."),
			  TEXT("Sahil evi denize çok yakın. Tsunami uyarısında sakinlerin nereye kaçacağını bilmesi gerekiyor."),
			  Tahliye, -1,
			  TEXT("Kıyıda güçlü bir sarsıntı hissedersen ya da deniz aniden çekilirse uyarı beklemeden yüksek bir yere ya da sığınağa git.") },
		};

		C.SpeakerName[0] = TEXT("Ayaz");
		C.SpeakerRole[0] = TEXT("Afet gönüllüsü");
		C.SpeakerColor[0] = Mustard;
		C.SpeakerName[1] = TEXT("Berna");
		C.SpeakerRole[1] = TEXT("Hazırlık masası");
		C.SpeakerColor[1] = Teal;
		C.SpeakerName[2] = TEXT("Cem");
		C.SpeakerRole[2] = TEXT("Afet koordinatörü");
		C.SpeakerColor[2] = Coral;

		using S = EDBSpeaker;
		C.IntroA = {
			{ S::A, TEXT("Merhaba! Ben Ayaz, bu mahallenin afet gönüllüsüyüm.") },
			{ S::A, TEXT("Bir sorun var: kasabada depreme, sele, yangına ve tsunamiye hazır olmayan yerler var.") },
			{ S::A, TEXT("Sol alttaki tarayıcıya bas, tahtayı birlikte tarayalım.") },
		};
		C.AfterScan = {
			{ S::A, TEXT("Uyarı işaretli yerleri gördün mü? Her birinin bir eksiği var.") },
			{ S::A, TEXT("Berna sana hazırlık masasını gösterecek.") },
		};
		// HUD bu sıraya göre parlatır: 1 not, 2 çanta türleri, 3 eşyalar, 4 hazırla düğmesi, 5 kapat
		C.BagIntroB = {
			{ S::B, TEXT("Ben Berna. Burası hazırlık masası; afet çantalarını burada hazırlıyoruz.") },
			{ S::B, TEXT("Sağ alttaki not kağıdında tarayıcının bulduğu sorunlar yazıyor. Önce onu oku.") },
			{ S::B, TEXT("Üç çeşit çanta var: ilk yardım, deprem ve yangın çantası. Önce çantanın türünü seç.") },
			{ S::B, TEXT("Sonra eşyalara tıklayıp çantaya koy. Her çantaya 4 eşya sığar; o çantaya ait doğru 4 eşyayı bulmalısın.") },
			{ S::B, TEXT("Hazır olunca 'Çantayı Hazırla'ya bas. Çanta alttaki sırt çantasına gider.") },
			{ S::B, TEXT("Bitince sağ üstteki X ile masayı kapat.") },
		};
		// HUD: 1 önlemler, 2 bütçe, 3 sırt çantası, 4 çekmece
		C.PanelIntroC = {
			{ S::C, TEXT("Ben Cem, mahallenin afet koordinatörüyüm. Aşağıdaki paneli anlatayım.") },
			{ S::C, TEXT("Soldakiler şehir önlemleri: sel için rögar kapağı, tsunami için tahliye, yangın için yangın musluğu, deprem için bina güçlendirme.") },
			{ S::C, TEXT("Her önlemin bir bedeli var; bütçen yukarıda yazıyor. Doğru eve gönderdiğin her çanta bütçeye katkı sağlar.") },
			{ S::C, TEXT("Sağdaki sırt çantasında hazırladığın çantalar duruyor. Bir çanta ya da önlem seç, sonra uyarı işaretli yere tıkla.") },
			{ S::C, TEXT("Yeni çanta gerekirse sağdaki çekmeceyi çek: hazırlık masası açılır.") },
			{ S::C, TEXT("Bütün işaretler yeşil olunca kasaba afetlere hazır demektir. Kolay gelsin!") },
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
