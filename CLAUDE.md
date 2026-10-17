# DisasterBoard

Karton bir masa oyunu kasabasında tarayıcıyla binaların deprem risklerini bulursun; her eve uygun afet çantasını hazırlar, şehir risklerini araçlarla giderirsin. Deprem gelmeden bütün binalar hazırsa kazanırsın.

Tema: **depreme hazırlık ve farkındalık.** Görsel dil: 3D lowpoly, cozy, karton (cardboard) kasaba, masa oyunu.
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
Blender x → UE −X, y → Y, kaide üstü Z=0. Takvim duvarı X=100,7, yan duvar Y=100,7, zemin −160,7. Kaide köşeye
dayalı 2 m blok. Tahta 128 cm, 13×13 oluk (8 cm), pahlı kenar, karo üstü Z=13,8. Takvim yatay 88,5×64,3 cm,
merkez (Y −2,4, Z 88,9). DBData'daki 5×5 arsa → tahtada 2×2 karo (`LotOrigin` 0,3,6,9,11), binalar ×0,62.
**Üç kamera:** menü (yatay, üstte takvim altta tahta; `MenuCamLoc` / `MenuCamLookAt`) · takvim yakın plan
(Ayarlar / Emeği Geçenler: kamera 0,9 sn'de yaklaşır, sonra sayfa kalkar; Geri: sayfa iner, kamera menü
açısına döner) · tahta (330 cm, −50°, yaw 45°). Pozlama sabit EV100 1,9.

**Test:** `UnrealEditor.exe <tam yol>\DisasterBoard.uproject /Game/DisasterBoard/Maps/L_Ana -game -windowed
-ResX=1600 -ResY=900 -ExecCmds="DBTest"` (Git Bash'te `MSYS_NO_PATHCONV=1` ile). ~90 sn oynar, 21 görüntü
`Saved/Screenshots/WindowsEditor/DB_*.png`, kendisi kapanır. Test tıklama işlevlerini doğrudan çağırıyor.

**Derlendi, ekranda görülmedi:** gerçek fareyle tıklama/hover (imleç, tooltip), Ayarlar'daki tam ekran ve
grafik düğmeleri ← 2'yi geçerse yeni özellik durur

**Bozuk:** —

**Ekipten bekleyen kararlar:** alt paneldeki ilk iki ikonun anlamı (şimdilik "Toplanma Alanı", "Eşya
Sabitleme"); A/B/C'nin adları (şimdilik Ayaz, Berna, Cem); portre görselleri; sorun/eşya listesi.

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
