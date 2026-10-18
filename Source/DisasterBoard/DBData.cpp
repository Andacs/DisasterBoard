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
	enum EItem { SargiBezi, Antiseptik, YaraBandi, Su, Fener, Duduk, YanginTupu, DumanMaskesi,
		KalpIlaci, AstimSpreyi, Gozluk, BebekMamasi, BebekBezi, KediMamasi, Battaniye, Powerbank, Belgeler };
	enum ETool { Rogar, Tahliye, YanginMuslugu, IngilizAnahtari, Guclendirme };
	enum EBina { AyseEv, Okul, Siginak, YilmazEv, Apartman, Kitapci, Firin, Market, OgrenciEv, Park, SahilEv, AhsapEv };

	FDBContent Build()
	{
		FDBContent C;

		// 17 Ekim (ekip, Purble Place gibi fabrika): önce temel kit seçilir, içindekiler kendiliğinden eklenir;
		// sonra bant aşağı inerken kişiye özel eşyalar eklenir, en altta teslim.
		C.Items = {
			{ TEXT("Sargı Bezi"), TEXT("SB"), Cream },
			{ TEXT("Antiseptik"), TEXT("AN"), Lavender },
			{ TEXT("Yara Bandı"), TEXT("YB"), Rose },
			{ TEXT("Su"), TEXT("SU"), Sky },
			{ TEXT("El Feneri"), TEXT("FN"), Mustard },
			{ TEXT("Düdük"), TEXT("DÜ"), Teal },
			{ TEXT("Yangın Tüpü"), TEXT("YT"), RGB(214, 76, 66) },
			{ TEXT("Duman Maskesi"), TEXT("DM"), Slate },
			{ TEXT("Kalp İlacı"), TEXT("Kİ"), RGB(220, 90, 110) },
			{ TEXT("Astım Spreyi"), TEXT("AS"), RGB(120, 170, 220) },
			{ TEXT("Gözlük"), TEXT("GÖ"), RGB(90, 90, 100) },
			{ TEXT("Bebek Maması"), TEXT("BM"), RGB(236, 170, 190) },
			{ TEXT("Bebek Bezi"), TEXT("BB"), RGB(200, 220, 236) },
			{ TEXT("Kedi Maması"), TEXT("KM"), Kraft },
			{ TEXT("Battaniye"), TEXT("BT"), Coral },
			{ TEXT("Powerbank"), TEXT("PB"), RGB(80, 84, 92) },
			{ TEXT("Önemli Belgeler"), TEXT("ÖB"), Card },
		};
		C.Kits = {
			{ TEXT("İlk Yardım Çantası"), TEXT("İlk Yardım"), RGB(214, 76, 66), TEXT("sargı bezi, antiseptik, yara bandı"), { SargiBezi, Antiseptik, YaraBandi } },
			{ TEXT("Deprem Çantası"), TEXT("Deprem"), RGB(86, 132, 196), TEXT("su, el feneri, düdük"), { Su, Fener, Duduk } },
			{ TEXT("Yangın Çantası"), TEXT("Yangın"), RGB(232, 136, 52), TEXT("yangın tüpü, duman maskesi, el feneri"), { YanginTupu, DumanMaskesi, Fener } },
		};
		C.Stations = {
			{ TEXT("SAĞLIK"), { KalpIlaci, AstimSpreyi, Gozluk } },
			{ TEXT("BAKIM"), { BebekMamasi, BebekBezi, KediMamasi } },
			{ TEXT("İHTİYAÇ"), { Battaniye, Powerbank, Belgeler } },
		};

		// Alt panel sırası: rögar kapağı, tahliye, yangın musluğu (yenisini koy), İngiliz anahtarı (bozuğu tamir), bina güçlendirme
		C.Tools = {
			{ TEXT("Rögar Kapağı"), TEXT("RK"), RGB(80, 84, 92), 15, TEXT("Sel"), TEXT("Rögar kapakları kondu") },
			{ TEXT("Tahliye"), TEXT("TH"), Sky, 30, TEXT("Tsunami"), TEXT("Sığınağın yolu öğretildi") },
			{ TEXT("Yangın Musluğu"), TEXT("YM"), RGB(214, 76, 66), 40, TEXT("Yangın"), TEXT("Sokağa yangın musluğu kondu") },
			{ TEXT("İngiliz Anahtarı"), TEXT("İA"), RGB(120, 128, 150), 25, TEXT("Yangın"), TEXT("Yangın musluğu tamir edildi") },
			{ TEXT("Bina Güçlendirme"), TEXT("BG"), Teal, 60, TEXT("Deprem"), TEXT("Bina güçlendirildi") },
		};
		C.EvacuateTool = Tahliye;
		C.BraceTool = Guclendirme;   // güçlendirilen binaya çelik çaprazlar konur
		// Sel: marketin altındaki yolda iki karo su birikintisi; rögar kapakları ızgaraya konur (kapak başı 15 TL)
		C.FloodTool = Rogar;
		C.FloodCells = { FIntPoint(7, 5), FIntPoint(8, 6) };
		// Yangın: ahşap evlerin sokağındaki musluk bozuk; İngiliz anahtarıyla tamir edilir
		C.HydrantBuilding = AhsapEv;
		C.HydrantCell = FIntPoint(8, 9);
		C.WrenchTool = IngilizAnahtari;
		// Yangın: kitapçının sokağında hiç musluk yok; yenisi kitapçının yanındaki yola konur
		C.HydrantAddTool = YanginMuslugu;
		C.NewHydrantBuilding = Kitapci;
		C.NewHydrantCell = FIntPoint(8, 3);
		C.StartMoney = 120;   // 3 sipariş (her kitten bir, revizyon belgesi): çanta geliri 90, önlemler 185 TL
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

		// Kasabalılar: samimi konuşur (ekip, 17 Ekim). Sıra C.Buildings ile aynı.
		C.Residents = {
			{ TEXT("Ayşe Teyze"),
			  TEXT("Hoş geldin yavrum, otur bir çay iç!"),
			  TEXT("Gel yavrum gel! İlaçlarım, gözlüğüm hep ortada... Gece bir şey olsa elim ayağıma dolaşır. Bir çanta hazırlasak?"),
			  TEXT("Allah razı olsun evladım, içim rahatladı. Çayın hep hazır, bekliyorum!") },
			{ TEXT("Selma Öğretmen"),
			  TEXT("Merhaba! Teneffüs zili çalmak üzere."),
			  TEXT("Merhaba canım! Revirdeki ilk yardım çantası bomboş. Bir de astımlı öğrencimiz var, spreyi hep yanında olmalı."),
			  TEXT("Çocuklar adına çok teşekkür ederim, sen bir harikasın!") },
			{ TEXT("Hasan Amca"),
			  TEXT("Ben sığınağın bekçisiyim evlat. Dev dalga gelirse herkes buraya, yokuş yukarı! Kapım hep açık."),
			  TEXT(""),
			  TEXT("") },
			{ TEXT("Zeynep"),
			  TEXT("Şşş, bebek yeni uyudu!"),
			  TEXT("Şşş, bebek yeni uyudu... Deprem olsa ne kapıp çıkacağımızı bilmiyoruz. Mamasıyla bezini de unutmamak lazım."),
			  TEXT("Çok sağ ol! Çanta kapının yanında, artık gözüm arkada kalmaz.") },
			{ TEXT("Kemal Bey"),
			  TEXT("Apartman yöneticisiyim, aidatları sorma!"),
			  TEXT("Şu kolonlardaki çatlaklara bak... Her gece içim titriyor. Binaya bir el atılsa?"),
			  TEXT("Bina sapasağlam! Bütün apartman sana dua ediyor.") },
			{ TEXT("Elif"),
			  TEXT("Selam! Raflarımı duvara sabitledim, depremde kitaplar kimsenin başına düşmesin diye. Sen de evde yap!"),
			  TEXT("Selam! Dükkân baştan aşağı kâğıt... Sokağımızda tek bir yangın musluğu bile yok, içim hiç rahat değil."),
			  TEXT("Musluk kapımın önünde! Kitaplarım da ben de rahatladık, sağ ol.") },
			{ TEXT("Ali Usta"),
			  TEXT("Simitler fırından yeni çıktı!"),
			  TEXT("Sabahtan beri fırın yanıyor kardeşim. Bir yangın çıksa belgelerim de gider, ortada kalırım."),
			  TEXT("Eline sağlık! Al şu sıcak simidi, benden!") },
			{ TEXT("Rıza Abi"),
			  TEXT("Bakkal Rıza, her şey var!"),
			  TEXT("Her yağmurda dükkânın önü göle dönüyor! Şu su birikintilerine rögar kapağı lazım."),
			  TEXT("Ayaklarımız kupkuru, eline sağlık! Gel bir gazoz ısmarlayayım.") },
			{ TEXT("Can"),
			  TEXT("Selam! Sınav haftası, kafam dolu."),
			  TEXT("Selam! Telefonum hep şarjsız, kedim Pamuk da var. Depremde ikimize de bir çanta lazım."),
			  TEXT("Pamuk da teşekkür ediyor: miyav!") },
			{ TEXT("Deniz"),
			  TEXT("Parkta oynamak çok güzel! Büyükler deprem olursa toplanma alanının burası olduğunu söyledi."),
			  TEXT(""),
			  TEXT("") },
			{ TEXT("Murat Abi"),
			  TEXT("Bugün deniz çok sakin."),
			  TEXT("Deniz şuracıkta... Dev dalga gelse çocukları nereye götüreceğimi bilmiyorum."),
			  TEXT("Sığınağın yolunu öğrendik, çocuklar da biliyor artık. Sağ ol!") },
			{ TEXT("Nermin Hanım"),
			  TEXT("Bahçemdeki güllere bak!"),
			  TEXT("Sokağımızdaki yangın musluğu bozuk, su vermiyor! Ahşap evler bunlar, bir kıvılcım yeter."),
			  TEXT("Musluk şırıl şırıl! Bu akşam yemeğe bekliyorum, hayır demek yok.") },
		};

		C.Problems = {
			{ AyseEv, TEXT("İlk yardım"),
			  TEXT("Ayşe Teyze yalnız yaşıyor; evde hiç ilk yardım malzemesi yok."),
			  TEXT("Ayşe Teyze yalnız yaşıyor ve sık sık küçük kazalar geçiriyor. Evde ilk yardım malzemesi yok."),
			  -1, IlkYardim,
			  TEXT("Her evde ilk yardım çantası olmalı. Düzenli ilaç kullananlar birkaç günlük ilacını, gözlük kullananlar yedek gözlüğünü çantaya koymalı."),
			  TEXT("Ayşe Teyze sık sık küçük yaralar alıyor. Evinde hangi çanta olmalı?"),
			  { KalpIlaci, Gozluk } },
			{ YilmazEv, TEXT("Deprem"),
			  TEXT("Yılmazların deprem anında alacakları bir çantası yok."),
			  TEXT("Yılmaz ailesi depreme hiç hazırlık yapmamış; kapının yanında hazır bir çantaları yok."),
			  -1, Deprem,
			  TEXT("Deprem çantasında su, el feneri ve düdük olmalı; bebekli aileler mama ve bez eklemeli. Çanta kapının yakınında durmalı."),
			  TEXT("Sarsıntıda kapıdan çıkarken kapacakları bir çantaları yok."),
			  { BebekMamasi, BebekBezi } },
			{ Firin, TEXT("Yangın"),
			  TEXT("Fırında sürekli ateş yanıyor ama yangın malzemesi yok."),
			  TEXT("Fırında sabahtan akşama ateş yanıyor; bir yangın çıkarsa müdahale edecek hiçbir şey yok."),
			  -1, Yangin,
			  TEXT("Ateşle çalışılan yerde yangın tüpü ve duman maskesi hazır olmalı; önemli belgelerin kopyaları çantada dursun. Dumanda eğilerek ilerle."),
			  TEXT("Sürekli ateş yanıyor ama yangına karşı hiçbir şey yok."),
			  { Belgeler, Battaniye } },
			{ Apartman, TEXT("Deprem"),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var."),
			  TEXT("Eski apartmanın kolonlarında çatlaklar var. Bina deprem yönetmeliğinden önce yapılmış."),
			  Guclendirme, -1,
			  TEXT("Riskli binalar uzman incelemesiyle güçlendirilmeli ya da yenilenmeli. Kolon kesmek, taşıyıcı duvar yıkmak hayati tehlikedir."),
			  TEXT("Kolonlarda çatlaklar var. Bina sarsıntıya dayanabilir mi?") },
			{ Market, TEXT("Sel"),
			  TEXT("Marketin önündeki yolda su birikiyor; rögar yok."),
			  TEXT("Her yağmurda marketin önündeki yolda su birikiyor. Su birikintilerinin olduğu ızgaralara rögar kapağı koy."),
			  Rogar, -1,
			  TEXT("Sele karşı rögar ve kanallar temiz tutulmalı. Su basan yerde elektriği kapat, akan suya girme, yüksek bir yere çık."),
			  TEXT("Yağmurda sokağı su basıyor. Su nereden akıp gitmeli?") },
			{ AhsapEv, TEXT("Yangın"),
			  TEXT("Ahşap evlerin sokağındaki yangın musluğu bozuk."),
			  TEXT("Birbirine bitişik ahşap evlerin sokağındaki yangın musluğu bozuk, su vermiyor; İngiliz anahtarıyla somununu sık."),
			  IngilizAnahtari, -1,
			  TEXT("Yangın muslukları bakımlı olmalı, önleri kapatılmamalı; itfaiyenin geçeceği yollar açık tutulmalı."),
			  TEXT("Yangın çıksa itfaiye suyu nereden alacak?") },
			{ Kitapci, TEXT("Yangın"),
			  TEXT("Kitapçının sokağında hiç yangın musluğu yok."),
			  TEXT("Kitapçı kâğıt dolu ama sokağında tek bir yangın musluğu yok. Yangın çıksa itfaiye suyu nereden alacak?"),
			  YanginMuslugu, -1,
			  TEXT("Her sokakta itfaiyenin ulaşabileceği bir yangın musluğu olmalı; önüne araç park edilmez, çevresi açık tutulur."),
			  TEXT("Kâğıt çabuk tutuşur. Sokakta su alınacak bir yer var mı?") },
			{ SahilEv, TEXT("Tsunami"),
			  TEXT("Sahil evinde oturanlar sığınağın yolunu bilmiyor."),
			  TEXT("Sahil evi denize çok yakın. Tsunami uyarısında sakinlerin nereye kaçacağını bilmesi gerekiyor."),
			  Tahliye, -1,
			  TEXT("Kıyıda güçlü bir sarsıntı hissedersen ya da deniz aniden çekilirse uyarı beklemeden yüksek bir yere ya da sığınağa git."),
			  TEXT("Deniz çok yakın. Dev dalga gelirse insanlar nereye gitmeli?") },
		};

		// Ekip notu (17 Ekim): yalnızca Mert, Asil ve Yiğit konuşur (A, B, C); 5. kişi oyuncu
		C.FloodProblem = C.Problems.IndexOfByPredicate([&C](const FDBProblemDef& P) { return P.Tool == C.FloodTool; });

		C.SpeakerName[0] = TEXT("Mert");
		C.SpeakerRole[0] = TEXT("Tarayıcı");
		C.SpeakerColor[0] = Mustard;
		C.SpeakerName[1] = TEXT("Asil");
		C.SpeakerRole[1] = TEXT("Hazırlık masası");
		C.SpeakerColor[1] = Teal;
		C.SpeakerName[2] = TEXT("Yiğit");
		C.SpeakerRole[2] = TEXT("Şehir önlemleri");
		C.SpeakerColor[2] = Coral;

		// Kısa cümleler: neyi yapacağını karakterin eli gösterir (HUD'daki hedefler bu sıraya bağlı)
		using S = EDBSpeaker;
		C.IntroA = {
			{ S::A, TEXT("Ben Mert! Bu masa oyununda kasabayı afetlere hazırlayacağız.") },
			{ S::A, TEXT("Önce sorunları bulalım.") },
			{ S::A, TEXT("Şu tarayıcıya bas! (Gösterdiğim tuşa tıkla)") },
		};
		C.AfterScan = {
			{ S::A, TEXT("İşaretli yerlerin yanında ne istedikleri yazıyor.") },
		};
		// HUD hedefleri: 1 sipariş kartı, 2 temel kit, 3 kişiye özel istasyonlar, 4 teslim, 5 kapat
		C.BagIntroB = {
			{ S::B, TEXT("Ben Asil. Burası çanta fabrikası; bant yukarıdan aşağı akar, durmaz!") },
			{ S::B, TEXT("Üstteki kartta hazırlanacak çanta ve tarifi yazıyor.") },
			{ S::B, TEXT("Çanta gelince önce temel kiti seç: içindekiler kendiliğinden eklenir.") },
			{ S::B, TEXT("Sonra çanta önlerinden geçerken kişiye özel eşyaları ekle. İlk çantada doğru olanlar parlar!") },
			{ S::B, TEXT("En alta inince teslim edilir. Yanlışsa çanta baştan gelir.") },
			{ S::B, TEXT("Bitince soldaki kulpa basıp masayı kapat.") },
		};
		// HUD hedefleri: 1 önlemler, 2 bütçe, 3 önlemler, 4 çekmece
		C.PanelIntroC = {
			{ S::C, TEXT("Ben Yiğit. Şehir önlemleri bende.") },
			{ S::C, TEXT("Sel, tsunami, yangın ve deprem için önlemler burada. Birine bas, gör.") },
			{ S::C, TEXT("Önlemler bütçeden düşer; teslim ettiğin her çanta bütçe kazandırır.") },
			{ S::C, TEXT("Önlemi seç, isteği olan yere tıkla. Rögar kapağını su birikintisinin olduğu ızgaraya koy.") },
			{ S::C, TEXT("Kasabalılara tıkla, dertlerini anlatsınlar. Çantalar sağdaki çekmecede: kulpa bas.") },
			{ S::C, TEXT("Arkada kalan yerleri görmek için tahtayı döndür: alttaki oklara ya da klavyede Q / E tuşlarına bas.") },
			{ S::C, TEXT("Hepsi yeşil olunca kasaba hazır! Fare tekerleği yakınlaştırır.") },
		};

		C.Comic = {
			TEXT("Bir pazar günü beş arkadaş kasabada geziyordu."),
			TEXT("Eski bir oyuncakçının vitrininde tozlu bir kutu gördüler: Tetkik."),
			TEXT("Kutudan minicik bir kasaba çıktı. Kartta yazıyordu: Afet gelmeden kasabayı hazırla."),
			TEXT("Hemen masaya oturdular. Sıra sende!"),
		};

		C.Credits = {
			// Yiğit (18 Ekim): isimler ve görevler. "~" görev satırı, "@" alt bilgi (DrawSubPage küçük çizer)
			TEXT("FreakLabs"),
			TEXT("Şinasi Andaç Senyücel"),
			TEXT("~Game Designer  ·  Game Developer"),
			TEXT("Yiğit Şahip Göynüklü"),
			TEXT("~VFX  ·  SFX  ·  Texturing  ·  Analyzer"),
			TEXT("Asilkan Kabasakal"),
			TEXT("~2D Designer  ·  UI / UX / HUD Designer"),
			TEXT("Mert Arslan"),
			TEXT("~3D Designer"),
			TEXT(""),
			TEXT("@Unreal Engine 5.8 (Epic Games)  ·  Kod yardımı: Claude (Anthropic)"),
			TEXT("@Modeller (CC BY): A.S.D., louieoliva, BetelgeuseMan, Das Toasty"),   // Sketchfab: bina, ağaç, araba, park; lisans atıf istiyor
			TEXT("@CIVIL GAME JAM 2026"),
		};

		// Ekip görsellerinin dosya adları (Tools/sablon_listesi.py ile aynı sıra; şablonlar oradan üretiliyor)
		const TCHAR* ItemKeys[] = { TEXT("sargi_bezi"), TEXT("antiseptik"), TEXT("yara_bandi"), TEXT("su"), TEXT("el_feneri"), TEXT("duduk"),
			TEXT("yangin_tupu"), TEXT("duman_maskesi"), TEXT("kalp_ilaci"), TEXT("astim_spreyi"), TEXT("gozluk"), TEXT("bebek_mamasi"),
			TEXT("bebek_bezi"), TEXT("kedi_mamasi"), TEXT("battaniye"), TEXT("powerbank"), TEXT("onemli_belgeler") };
		const TCHAR* KitKeys[] = { TEXT("ilk_yardim"), TEXT("deprem"), TEXT("yangin") };
		const TCHAR* ToolKeys[] = { TEXT("rogar_kapagi"), TEXT("tahliye"), TEXT("yangin_muslugu"), TEXT("ingiliz_anahtari"), TEXT("bina_guclendirme") };
		check(UE_ARRAY_COUNT(ItemKeys) == C.Items.Num() && UE_ARRAY_COUNT(KitKeys) == C.Kits.Num() && UE_ARRAY_COUNT(ToolKeys) == C.Tools.Num());
		for (int32 i = 0; i < C.Items.Num(); ++i) C.Items[i].Key = ItemKeys[i];
		for (int32 i = 0; i < C.Kits.Num(); ++i) C.Kits[i].Key = KitKeys[i];
		for (int32 i = 0; i < C.Tools.Num(); ++i) C.Tools[i].Key = ToolKeys[i];
		return C;
	}
}

const FDBContent& FDBContent::Get()
{
	static const FDBContent Content = Build();
	return Content;
}
