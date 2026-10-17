# DisasterBoard

Karton bir masa oyunu kasabasında tarayıcıyla afet risklerini (deprem, sel, yangın, tsunami) bulursun; evlere doğru
afet çantasını (ilk yardım / deprem / yangın) hazırlayıp gönderirsin, şehir risklerini bütçeyle önlem alarak
giderirsin. Bütün işaretler yeşil olunca kasaba afetlere hazırdır.

Tema: **doğal afetlere hazırlık ve farkındalık** (Andaç, 17 Ekim: amaç belediye yardımı değil, hazırlık ve farkındalık). Görsel dil: 3D lowpoly, cozy, karton (cardboard) kasaba, masa oyunu.
Akış çizimleri: `Docs/Cizimler/akis_01..09.jpg`. Oda/masa/takvim taslağı: `Art/Sahne/AnaMenu_OyunAlani.blend`.

48 saatlik game jam projesi. Unreal Engine 5.8, Windows PC. 4 kişilik ekip (FreakLabs), tek programcı Andaç.

---

## Sert kurallar (onay almadan ihlal edilmeyecek)

1. **Projeyi/modülü yeniden adlandırma.** Proje adı `DisasterBoard`; `.uproject`, modül,
   `Target.cs` dosyaları, `DISASTERBOARD_API` makrosu ve `DefaultEngine.ini` redirect'leri birbirine
   bağlı.
2. **Seviye dosyasının tek sahibi var: Andaç.** Sahne (oda, masa, tahta, takvim) koddan kuruluyor; seviyede yalnızca ışık var. Başkası `L_Ana` seviyesini kaydetmez.
3. **Seviyenin kopyası açılmaz.** Deneme gerekiyorsa `L_Test` kullanılır ve oradaki hiçbir şeyin
   ana seviyeye taşınmayacağı bilinir.
4. **`Content/Vendor/` dokunulmaz.** İndirilen paketler yeniden adlandırılmaz, taşınmaz — UE 5.8
   Blueprint içeren paket yeniden adlandırmasında çöküyor.
5. **36. saatten sonra yeni özellik yok.** Sadece hata ve paketleme.

---

## Kapsam

**Çekirdek mekanik:** tara → riskli binayı gör → notu oku → çantayı hazırla / şehir aracını seç → binaya uygula → tik.

**Akış (çizimlere göre, 17 Ekim):**
1. Ana menü **duvardaki takvimde** (Oyna · Ayarlar · Emeği Geçenler · Çıkış). Ayarlar / Emeği Geçenler'e basınca takvim sayfası aşağıdan yukarı kalkar, yeni sayfada seçenekler.
2. Oyna → kamera takvimden masadaki tahtaya döner.
3. **A** (baretli görevli, sol alt): "Bir sorun var" → tarayıcıya yönlendirir. Tarayıcıya basınca tahta taranır, riskli binalarda uyarı (!) çıkar.
4. Sağda **çanta paneli** açılır; **B** nasıl kullanılacağını anlatır. Panelin sağ altındaki **not kağıdında** bulunan sorunlar yazar. Oyuncu eşyalarla çanta hazırlar.
5. Panel kapanınca **C** alt paneli anlatır: solda şehir araçları (rögar?, eşya sabitleme?, yangın musluğu, bina güçlendirme), sağda hazırlanan çantalar.
6. Çantayı / aracı seç → riskli binaya tıkla → doğruysa binada yeşil tik.
7. Deprem gelir (geri sayım). Hazır binalar ayakta kalır. Hepsi hazırsa kazanırsın; değilse kaçırılanlar ve "doğrusu ne" bilgisi gösterilir.

**Bitecekler:** mekanik · bir seviye · kazanma/kaybetme · yeniden başlama · çalışan paket

**Bitmeyecekler:** kayıt, ikinci tahta/seviye (gerekirse aynı tahta farklı veriyle), karakter animasyonu (A/B/C portre + balon), sesli diyalog.

**Kesme sırası:** 1. takvim sayfası kalkma animasyonu (düz geçiş) 2. deprem animasyonu (yalnızca sonuç ekranı) 3. Ayarlar sayfası 4. kamera geçişi (doğrudan tahtadan başla)

---

## Sahiplik

| Alan | Kim |
|---|---|
| Kod / oynanış | Andaç |
| Seviye dosyası | Andaç |
| Asset / sanat | Mert (merttaarslan-ai), Asil (WgexWasHere?) |
| Ses | belirlenmedi |
| Paketleme / teslim | Andaç, Yiğit (CezeryeF?) |

---

## Klasör düzeni

- `Content/DisasterBoard/` — bizim yazdığımız her şey
- `Content/Vendor/` — indirilen paketler, dokunulmaz
- `Source/DisasterBoard/` — C++

Asla commit edilmez: `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/`

Binary asset'ler Git LFS'ten geçiyor — `.gitattributes`'a bak.

---

## Derleme

Editör **kapalı** olmalı (Live Coding kilidi):

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" DisasterBoardEditor Win64 Development -project="<tam yol>/DisasterBoard.uproject" -waitmutex -NoXGE -MaxParallelActions=2
```

`-NoXGE -MaxParallelActions=2` bu makinede şart; normal derleme PCH için sanal bellek
oluşturamıyor.

## Paketleme

```
Build/paketle.bat   (çıktı Build/Package/Windows/DisasterBoard.exe)
```

Ölçülen süre: boş projenin ilk paketi **~5 dk** (277 sn, JamDenemeB, 15 Ekim); dolu Sallantı sıcak 111–134 sn. Teslim konfigürasyonu: **Shipping**

---

## Git akışı

Tek dal: `main`. Herkes oraya push eder. Feature branch + PR 48 saatte maliyeti faydasından fazla.

- Çalışan her şey commit edilir, **her commit push edilir**
- Pull/checkout öncesi **editör kapanır** — açık editör `.uasset` kilidi tutuyor ve bir
  `git checkout`'u yarım uygulayabiliyor
- `.umap`/`.uasset` çakışmasında merge yok, seçim var: `git checkout --theirs|--ours <yol>`.
  Seçimi seviye sahibi yapar.

---

## Çalışma kuralları

- **Ölç, tahmin etme.** Bir değeri iki kez tahminle değiştirdiysen dur ve ölç.
- **Ekranda görmediğin şey yoktur.** Yazıldığı saat içinde PIE'de görülecek; görülmediyse commit
  mesajında "görülmedi" yazar.
- **UI metinlerinde emoji yok.** Fontta olmayan tek glif oyunu saniyelerce dondurabiliyor.
- **Görünür bir şeyi sıfıra ölçekleme**, gizle. Sıfır ölçek → ters çevrilemez matris → donma.
- **Asset yolunu ezberden yazma**, Content Browser'dan kopyala.
- Bilinen motor tuzakları: `Docs/JamKit/04_UNREAL_TUZAKLARI.md`

---

## Bilinen durum

**17 Ekim ~02.30 (saat 8,5): çekirdek döngü baştan sona çalışıyor, ekranda görüldü** (`DBTest`, görüntüler
`Docs/EkranGoruntuleri/`): takvim menüsü → Ayarlar / Emeği Geçenler'de sayfa yukarı kalkıyor → Oyna → kamera
takvimden tahtaya → A konuşur → tarayıcı tıklanınca yeşil çizgi tahtayı tarar, riskli binalarda (!) çıkar → B +
hazırlık masası (8 eşya, 4'lük çanta, not kağıdı) → panel kapanınca C alt paneli anlatır → oyun: 2:30 geri
sayım, araç / çanta seç + binaya tıkla → tik; yanlış araç ve eksik çanta −10 sn ve sebebini söyler → deprem
(hazır olmayan bina çöker) → sonuç kartı (her sorun için "doğrusu ne") → Tekrar Oyna (`?Direkt=1`, menü ve
konuşmalar atlanır) / Menü.

**Kod düzeni (`Source/DisasterBoard/`, önek `DB`):**
- `DBData.cpp` — **bütün içerik**: eşyalar, şehir araçları, 10 bina (hücre, ölçü, renk), 7 sorun (not, gereken
  eşya/araç, sonuçtaki bilgi), A/B/C konuşmaları, jenerik. Metin değiştirmek için yalnızca bu dosya.
- `DBWorld` — oda, masa, tahta, binalar, takvim, iki kamera **koddan** (BasicShapes + renkli MID); tarama
  çizgisi, sayfa çevirme, deprem, zıplama animasyonları. Güneş yönü oyunda ayarlanır, seviye dosyası değişmez.
  Mert'in mesh'i gelince ilgili `Part(...)` çağrısı mesh'le değişir.
- `DBGameMode` — akış (`EDBPhase`), çantalar, seçim, ceza, deprem, sonuç. Ayarlar: `QuakeSeconds` 150,
  `WrongPenaltySeconds` 10, `BagCapacity` 4, `MaxBags` 4.
- `DBHUD` — bütün arayüz Canvas ile (asset yok), 1080p ölçüsü × `S`. Tıklanabilir alanlar her karede `Hits`.
- `DBPlayerController` — sol tık / Esc + sağ tık (geri, seçimi bırak) / F5 (baştan). Konsol: `DBTest`,
  `DBExposure <EV>`, `DBTime <sn>`, `DBQuake`.

**Sahne = `Art/Sahne/AnaMenu_OyunAlani.blend` ölçüleri (17 Ekim ~03.20, Andaç'ın Blender görüntülerine göre):**
Blender x → UE −X, y → Y, masa üstü Z=0. Takvim duvarı X=100,7, yan duvar Y=100,7. **Masa bütün zemini
kaplar** (Andaç: arayüz çizimlerdeki gibi masanın üstünde duracak). Tahta 128 cm, 13×13 oluk (8 cm), pahlı kenar, karo üstü Z=13,8. Takvim yatay 88,5×64,3 cm,
merkez (Y −2,4, Z 88,9). DBData'daki 5×5 arsa → tahtada 2×2 karo (`LotOrigin` 0,3,6,9,11), binalar ×0,62.
**Üç kamera:** menü (Blender görüntüsündeki gibi köşeye çapraz: sol üstte takvim, sağda yan duvar, altta tahta;
`MenuCamLoc` / `MenuCamLookAt` / FOV 55) · takvim yakın plan
(Ayarlar / Emeği Geçenler: kamera 0,9 sn'de yaklaşır, sonra sayfa kalkar; Geri: sayfa iner, kamera menü
açısına döner) · tahta (330 cm, −50°, yaw 45°). Pozlama sabit EV100 1,9.

**17 Ekim ~03.50, Andaç'ın kararları:** oyunda **süre yok**, **deprem sahnesi ve "çök-kapan-tutun" yok**. Bütün
binalar hazır olunca 1,4 sn sonra sonuç kartı ("Kasaba depreme hazır!", yanlış deneme sayısı, her bina için doğru
bilgi). Yanlış uygulama süre değil `Mistakes` sayar. Eski deprem kodu `bQuakeAtEnd` (kapalı) arkasında duruyor.
**Hazırlık masası = sağ ortada çekmece** (çizim 4–5): kapalıyken sağ kenarda "HAZIRLIK" kulpu, tıklayınca sola
kayarak açılır (`ADBHUD::PanelSlide`); not kağıdı panelin altına sarkar. Alt panelde yalnızca şehir araçları +
sırt çantası.

**17 Ekim ~04.30, Andaç'ın kararları (uygulandı, ekranda görüldü):**
- **Takvim yazıları sayfaya basılı:** menü ve Ayarlar/Emeği Geçenler HUD yardımcılarıyla 1024×744 render
  target'a çizilir (`ADBHUD::RenderPage`), sayfanın önündeki düzleme `Widget3DPassThrough_Opaque` ("SlateUI")
  ile kaplanır → takvimle birlikte eğik, sayfa kalkarken onunla gider. Tıklama: fare ışını sayfa düzlemiyle
  kesiştirilir (`ADBWorld::PagePointFromScreen`). Test "Oyna"nın hesaplanan yerini (527,344) düğmenin üstünde buldu.
  Işıksız malzeme parlaklığı ölçülerek `PageGain` 1,1 (0,4–2,0 denendi; 1,0'da kağıt 220 ≈ ışıklı duvar 227).
  Sayfadaki yazılar aynı renkte konturla kalın (ince harf ~%40 küçülünce açık kalıyordu).
- **3 çanta türü:** İlk Yardım (sargı bezi, yara bandı, antiseptik, makas), Deprem (su, konserve, el feneri,
  düdük), Yangın (yangın tüpü, duman maskesi, yangın örtüsü, eldiven). 12 eşya karışık sırada, çantaya 4 sığar.
  Önce tür seçilir; yanlış tür ya da eksik eşya → `Mistakes` + hangi eşyanın eksik olduğu söylenir.
- **Bütçe:** başlangıç 100 TL. Şehir önlemleri bütçeden düşer: Rögar Kapağı 30 (sel), Tahliye 30 (tsunami),
  Yangın Musluğu 40 (yangın), Bina Güçlendirme 60 (deprem). Doğru eve giden her çanta +30 TL. Para yetmezse
  hata sayılmaz, "çanta göndererek bütçe kazan" der. Test sonu: 90 TL, 2 hata.
- **Tahta:** son iki sıra deniz (dalgalanır), önünde kum; Sahil evi tahliyede 4 sakin sokaklardan sığınağa
  (yeşil kubbe) yürür (`ADBWorld::StartEvacuation`). Binalar 24'lük plakayla tam 2×2 karoya oturur (×0,608).
- 12 bina, 9 sorun (`DBData.cpp`). C artık "afet koordinatörü".

**17 Ekim ~05.30, ekip notları (oyun testi sonrası) uygulandı, ekranda görüldü:**
- **Roller:** A = Mert (tarayıcı), B = Asil (hazırlık masası), C = Andaç (şehir önlemleri); 5. kişi oyuncu.
  (Notta "Andre" yazıyordu, Andaç olarak okundu.) Konuşmalar kısaldı; neyi yapacağını karakterin eli gösterir.
- **Rehber karakter** (`ADBHUD::DrawGuide`): portre yerine aynı yerde ayakta 2D figür, kolu omuzdan hedefe döner
  ve sallanır (gösterme). Hedefler konuşma satırına bağlı (`GuideTarget`); oyunda Mert sıradaki görevin
  binasını gösterir, elinden binaya kayan noktalı iz. Görsel: `Content/Karakterler/{A,B,C}_govde.png` +
  `_kol.png` (BENIOKU.md); dosya yoksa çizilmiş yer tutucu.
- **Sıralı görev listesi** sol üstte (`DrawTasks`): sıradaki görev kartı + cevabı söylemeyen ipucu
  (`FDBProblemDef::Hint`) + "Bir çanta / şehir önlemi lazım"; bitenlerin üstü çizili. Not kağıdı ve "hazır
  çantalar" panelden kalktı (panel 600). Sıradaki görevin işareti büyük ve sarı haleli.
- **Çizgi roman girişi:** Oyna → 4 sahne × 5 sn (tıkla: sonraki, "Geç"), sonra kamera tahtaya.
  Görsel: `Content/Cizgiroman/sahne1..4.png` (BENIOKU.md), yoksa çizilmiş yer tutucu. Metin `C.Comic`.
- **Müzik** (`UDBMusicWave`): koddan üretilen piyano/müzik kutusu arpeji, Do–Lam–Fa–Sol. Tempo görev
  ilerlemesiyle 72 → 112 BPM; ilerledikçe bas, melodi, pırıltı girer. Ölçüldü (`DBMusicWav`, 40 sn, yoğunluk
  0→1): çeyreklerde 75 / 84 / 96 / 108 BPM, tepe −4…−6 dBFS, RMS ≈ −15 dBFS, kırpma yok. **Dinlenmedi.**
  M ya da Ayarlar → Müzik aç/kapa.
- **Atmosfer:** tarama artık kum rengi çizgi + savrulan kum taneleri + toz bulutu; bulunan her riskin
  çevresinde afetine göre renkli sis (deprem toz, sel mavi pus, yangın duman, tsunami deniz pusu), çözülünce
  dağılır ve kum savrulur. Bulut dokusu CPU'da üretiliyor, malzeme `Widget3DPassThrough_Translucent`.
  İlk denemede sis zemin rengine çok yakındı; fark görüntüsüyle ölçülüp koyulaştırıldı.
- **Bekleyen:** 3D eşya modelleri (masada ve grid kutularında gösterilecek): modelciden gelmeli; Yiğit'in
  çekim notu (yarım okundu).

**17 Ekim ~06.00, Andaç'ın kararları (uygulandı, ekranda görüldü) — öncekilerin üstüne yazar:**
- **Hikâye en başta, menüden önce** (oyun açılınca 4 sahne; bitince / "Geç" ile takvim menüsü). Oyna → doğrudan
  tahtaya. "Menü" düğmesi `?Menu=1` ile açar, hikâye tekrar oynamaz.
- **Görev listesi kaldırıldı.** Onun yerine **istekler net:** her riskli yerin işaretinin yanında ne istediği
  yazar (çanta türü ya da önlem adı, kendi renginde).
- **Yalnızca Mert, Asil, Yiğit konuşur** (A, B, C). Andaç konuşmaz.
- **Hazırlık masası: 7 eşya, 3 çanta, her çanta doğru bir 3'lü kombinasyon:** İlk Yardım = Su + Sargı Bezi +
  Antiseptik; Deprem = Su + El Feneri + Düdük; Yangın = Yangın Tüpü + El Feneri + Duman Maskesi (Su ve El Feneri
  iki çantaya girer, `FDBItemDef::KitMask`). Sekmenin altında ipucu (`FDBKitDef::Hint`). "Çantayı Hazırla"
  kombinasyonu kontrol eder: yanlışsa hata sayılır, yanlış eşyayı söyler, eksiğin adını söylemez.
- **Deniz:** dalga kaldı; altına mavi taban kondu ve deniz tahtanın kenarına kadar uzanıyor, kum şeridi yok
  (dalgada kahverengi görünüyordu).

**17 Ekim ~07.30, Andaç'ın kararları (uygulandı, ekranda görüldü) — hazırlık masasının yerine geçer:**
- **Çanta Fabrikası** (Purble Place'teki pasta fabrikası gibi; dikey, **durmaz**): sağdaki çekmece. Üstte
  **sipariş kartı** (bina, çantanın resmi, tarif: 1. temel kit, 2. kişiye özel eşyalar). Bant yukarıdan aşağı
  `BeltSeconds` 14 sn'de iner. İstasyonlar yukarıdan aşağı: **Temel kit** (İlk Yardım / Deprem / Yangın;
  seçilince varsayılanları kendiliğinden eklenir: sargı bezi+antiseptik+yara bandı / su+el feneri+düdük /
  yangın tüpü+duman maskesi+el feneri), **Sağlık** (kalp ilacı, astım spreyi, gözlük), **Bakım** (bebek maması,
  bebek bezi, kedi maması), **İhtiyaç** (battaniye, powerbank, önemli belgeler). İstasyon yalnızca çanta
  önündeyken tıklanır. En altta otomatik **teslim**: doğruysa istek tamam +30 TL, yanlışsa hata + "Eksik / Fazla"
  ve çanta baştan gelir. Sipariş arası `<` `>`; çanta isteyen binaya tıklamak fabrikayı o siparişle açar.
  "Baştan" çantayı tepeye alır. Sırt çantası ve eski 7 eşya / kombinasyon mekaniği kalktı.
- 5 sipariş: Ayşe (İY + kalp ilacı, gözlük), Okul (İY + astım spreyi), Yılmaz (Deprem + bebek maması, bez),
  Öğrenci (Deprem + powerbank, kedi maması), Fırın (Yangın + belgeler, battaniye). `DBData.cpp`.
- **Kamera:** tekerlek yakınlaştırır (190–520 cm), Q/E 90° döndürür, sağ tıkla sürükle serbest döndürür
  (sürüklemeden bırakılan sağ tık eskisi gibi "geri"). Kamera bir duvarın arkasına geçince o duvar (ve takvim)
  gizlenir: ilk denemede ekranda yalnızca duvarın gölgedeki arkası görünüyordu.

**17 Ekim ~08.10, Andaç'ın kararları (uygulandı, ekranda görüldü):**
- **Yollar:** arsaların arasındaki karolar (sütun/satır 2, 5, 8) asfalt, düz yolda kesik orta çizgi
  (`ADBWorld::IsRoad`). **3 araba** yollarda gidip gelir (sağ şerit).
- **Rögar kapağı ızgaraya konur:** marketin önündeki yolda iki su birikintisi (`C.FloodCells` (7,5), (8,6)).
  Rögar seçiliyken birikintiler mavi noktalı halkayla, imleçteki karo yeşil/beyaz halkayla gösterilir. Kapak başı
  15 TL; ikisi kapanınca sel sorunu çözülür. Yol olmayan / birikintisiz karoya konmaz (ceza yok, ipucu verir).
  İlk yerde (6,8)/(7,8) apartmanın arkasında kalıyordu, taşındı.
- **İngiliz Anahtarı:** ahşap evlerin sokağındaki yangın musluğu eğik ve sızdırıyor; anahtar seçip binaya
  (ya da musluğa) tıklayınca doğrulur, sızıntı kaybolur (`RepairHydrant`). Önlemlerin "yapıldı" yerine kendi
  cümlesi var (`FDBToolDef::Done`).
- **Deniz haritaya gömülü:** karanın altında, tahtanın içinde, kıyıda beyaz köpük çizgisi; dalga sürüyor.
- **Kasabalılar konuşur, samimi:** her binanın önünde bir kasabalı figürü; binaya ya da kişiye tıklayınca
  konuşma balonu (selam / dert / teşekkür, `C.Residents`). Derdi çantaysa balonda "Çantasını hazırla" fabrikayı
  o siparişle açar. Sorun çözülünce kasabalı kendiliğinden teşekkür eder (5 sn). Bildirimler alt panelin üstüne
  taşındı (balonlarla çakışıyordu).
- **Fabrika öğreticisi:** ilk doğru teslime kadar seçilmesi gereken kit ve eşyalar parlar; yanlış teslimde de
  parlamaya devam eder (`bFactoryTutorial`).

**Test:** `UnrealEditor.exe <tam yol>\DisasterBoard.uproject /Game/DisasterBoard/Maps/L_Ana -game -windowed
-ResX=1600 -ResY=900 -ExecCmds="DBTest"` (Git Bash'te `MSYS_NO_PATHCONV=1` ile). ~110 sn oynar, 18 görüntü; fabrikada 5 sipariş (1 bilerek yanlış), kamera döndürme, 9/9 → sonuç (90 TL, 2 hata). `-DBGainTest` ile sayfa parlaklığını 5 değerde çeker
`Saved/Screenshots/WindowsEditor/DB_*.png`, kendisi kapanır. Test tıklama işlevlerini doğrudan çağırıyor.

**Derlendi, ekranda görülmedi:** gerçek fareyle tıklama/hover (imleç, tooltip), Ayarlar'daki tam ekran ve
grafik düğmeleri ← 2'yi geçerse yeni özellik durur

**Bozuk:** —

**Ekipten bekleyen:** karakter ve hikâye görselleri (`Content/Karakterler`, `Content/Cizgiroman`), ikon görselleri
(şimdilik harf kısaltmaları), 3D eşya modelleri, bütçe dengesi.

---

## Jam kuralları ve araçlar (CIVIL GAME JAM 2026, kitapçık v09)

- Son teslim **18 Ekim 18.00**, yükleme **kaydı** o saatten önce; Git desteği 12.00'ye kadar.
  Teslim sırası ve form: `Docs/JamKit/10_TESLIM.md`. Unreal tuhaflığında: `Docs/JamKit/04_UNREAL_TUZAKLARI.md`.
- **Repo public kalır** (private = diskalifiye, 6.5). Parola/anahtar/token girmez (3.7).
- **Hazır kaynak ve AI kullanımı eklendiği anda README'deki iki tabloya** (3.4, 4.2; beyan edilmeyen = diskalifiye).
- Derleme `Build\derle.bat` (editör kapalı), oynama `Build\oyna.bat`, paket `Build\paketle.bat`,
  trailer `bash Tools/trailer_yap.sh`, teslim kontrolü `bash Tools/teslim_hazirla.sh` (BOZUK: 0 olmalı).
- Git Bash'ten Unreal: `.uproject` **tam yolla**; `MSYS_NO_PATHCONV=1` ile `cmd //c` aynı komutta olmaz.
- Proje 17.10.2026 01:59'te `JamKit/araclar/yeni_proje.sh` ile açıldı.
