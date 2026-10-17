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
			  TEXT("Her evde sargı bezi, yara bandı, antiseptik ve makas bulunan bir ilk yardım çantası olmalı; yerini herkes bilmeli."),
			  TEXT("Ayşe Teyze sık sık küçük yaralar alıyor. Evinde hangi çanta olmalı?") },
			{ Okul, TEXT("İlk yardım"),
			  TEXT("Okulun revirindeki ilk yardım çantası boş."),
			  TEXT("Okulda yüzlerce öğrenci var ama revirdeki ilk yardım çantası bomboş."),
			  -1, IlkYardim,
			  TEXT("Okul ve iş yerlerinde ilk yardım çantası kolay ulaşılan bir yerde durmalı, malzemeleri düzenli yenilenmeli."),
			  TEXT("Revirdeki çanta bomboş. Yaralanan bir öğrenciye ne lazım?") },
			{ YilmazEv, TEXT("Deprem"),
			  TEXT("Yılmazların deprem anında alacakları bir çantası yok."),
			  TEXT("Yılmaz ailesi depreme hiç hazırlık yapmamış; kapının yanında hazır bir çantaları yok."),
			  -1, Deprem,
			  TEXT("Deprem çantasında su, bozulmayan yiyecek, el feneri ve düdük olmalı; çanta kapının yakınında durmalı."),
			  TEXT("Sarsıntıda kapıdan çıkarken kapacakları bir çantaları yok.") },
			{ OgrenciEv, TEXT("Deprem"),
			  TEXT("Öğrenci evinde depremden sonrası için hiçbir hazırlık yok."),
			  TEXT("Öğrenciler depremden sonraki ilk saatlerde ne yiyip içeceklerini hiç düşünmemiş."),
			  -1, Deprem,
			  TEXT("Deprem çantası evdeki kişi sayısına göre hazırlanmalı; su ve yiyeceklerin tarihleri yılda bir kontrol edilmeli."),
			  TEXT("Depremden sonraki ilk saatlerde ne yiyip içecekler?") },
			{ Firin, TEXT("Yangın"),
			  TEXT("Fırında sürekli ateş yanıyor ama yangın malzemesi yok."),
			  TEXT("Fırında sabahtan akşama ateş yanıyor; bir yangın çıkarsa müdahale edecek hiçbir şey yok."),
			  -1, Yangin,
			  TEXT("Ateşle çalışılan yerlerde yangın tüpü, yangın örtüsü, duman maskesi ve ısıya dayanıklı eldiven hazır olmalı."),
			  TEXT("Sürekli ateş yanıyor ama yangına karşı hiçbir şey yok.") },
			{ Apartman, TEXT("Deprem"),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var."),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var. Bina deprem yönetmeliğinden önce yapılmış."),
			  Guclendirme, -1,
			  TEXT("Riskli binalar uzman incelemesiyle güçlendirilmeli ya da yenilenmeli. Kolon kesmek, taşıyıcı duvar yıkmak hayati tehlikedir."),
			  TEXT("Kolonlarda çatlaklar var. Bina sarsıntıya dayanabilir mi?") },
			{ Market, TEXT("Sel"),
			  TEXT("Yağmurda marketin sokağını su basıyor; rögar kapaksız."),
			  TEXT("Her yağmurda marketin önündeki sokağı su basıyor; rögar tıkalı ve kapağı yok."),
			  Rogar, -1,
			  TEXT("Sele karşı rögar ve kanallar temiz tutulmalı. Su basan yerde elektriği kapat, akan suya girme, yüksek bir yere çık."),
			  TEXT("Yağmurda sokağı su basıyor. Su nereden akıp gitmeli?") },
			{ AhsapEv, TEXT("Yangın"),
			  TEXT("Ahşap evlerin sokağında yangın musluğu yok."),
			  TEXT("Birbirine bitişik ahşap evlerin sokağında yangın musluğu yok; yangın hızla yayılabilir."),
			  YanginMuslugu, -1,
			  TEXT("Yangın musluklarının önü kapatılmamalı, itfaiyenin geçeceği yollar açık tutulmalı."),
			  TEXT("Yangın çıksa itfaiye suyu nereden alacak?") },
			{ SahilEv, TEXT("Tsunami"),
			  TEXT("Sahil evinde oturanlar sığınağın yolunu bilmiyor."),
			  TEXT("Sahil evi denize çok yakın. Tsunami uyarısında sakinlerin nereye kaçacağını bilmesi gerekiyor."),
			  Tahliye, -1,
			  TEXT("Kıyıda güçlü bir sarsıntı hissedersen ya da deniz aniden çekilirse uyarı beklemeden yüksek bir yere ya da sığınağa git."),
			  TEXT("Deniz çok yakın. Dev dalga gelirse insanlar nereye gitmeli?") },
		};

		// Ekip notu (17 Ekim): A = Mert, B = Asil, C = Andaç; 5. kişi oyuncu
		C.SpeakerName[0] = TEXT("Mert");
		C.SpeakerRole[0] = TEXT("Tarayıcı");
		C.SpeakerColor[0] = Mustard;
		C.SpeakerName[1] = TEXT("Asil");
		C.SpeakerRole[1] = TEXT("Hazırlık masası");
		C.SpeakerColor[1] = Teal;
		C.SpeakerName[2] = TEXT("Andaç");
		C.SpeakerRole[2] = TEXT("Şehir önlemleri");
		C.SpeakerColor[2] = Coral;

		// Kısa cümleler: neyi yapacağını karakterin eli gösterir (HUD'daki hedefler bu sıraya bağlı)
		using S = EDBSpeaker;
		C.IntroA = {
			{ S::A, TEXT("Ben Mert! Bu masa oyununda kasabayı afetlere hazırlayacağız.") },
			{ S::A, TEXT("Önce sorunları bulalım: depreme, sele, yangına, tsunamiye hazır olmayan yerler var.") },
			{ S::A, TEXT("Şu tarayıcıya bas!") },
		};
		C.AfterScan = {
			{ S::A, TEXT("İşaretli yerlerin bir eksiği var. Görevler sol üstte, sırayla.") },
		};
		// 1 görevler, 2 çanta türleri, 3 eşyalar, 4 hazırla, 5 kapat
		C.BagIntroB = {
			{ S::B, TEXT("Ben Asil. Çantaları bu masada hazırlıyoruz.") },
			{ S::B, TEXT("Hangi çanta lazım? Görevdeki ipucuna bak.") },
			{ S::B, TEXT("Önce çantanın türünü seç.") },
			{ S::B, TEXT("Sonra o çantaya ait 4 eşyayı bul.") },
			{ S::B, TEXT("Hazır olunca buraya bas.") },
			{ S::B, TEXT("Bitince masayı kapat.") },
		};
		// 1 önlemler, 2 bütçe, 3 sırt çantası, 4 çekmece
		C.PanelIntroC = {
			{ S::C, TEXT("Ben Andaç. Şehirdeki riskleri ben gösteriyorum.") },
			{ S::C, TEXT("Bunlar şehir önlemleri: sel, tsunami, yangın ve deprem için.") },
			{ S::C, TEXT("Önlemlerin bedeli bütçeden düşer; doğru çanta göndermek bütçe kazandırır.") },
			{ S::C, TEXT("Hazırladığın çantalar burada. Seç, sonra işaretli yere tıkla.") },
			{ S::C, TEXT("Yeni çanta için çekmeceyi çek.") },
			{ S::C, TEXT("Hepsi yeşil olunca kasaba hazır!") },
		};

		C.Comic = {
			TEXT("Bir pazar günü beş arkadaş kasabada geziyordu."),
			TEXT("Eski bir oyuncakçının vitrininde tozlu bir kutu gördüler: DisasterBoard."),
			TEXT("Kutudan minicik bir kasaba çıktı. Kartta yazıyordu: Afet gelmeden kasabayı hazırla."),
			TEXT("Hemen masaya oturdular. Sıra sende!"),
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
