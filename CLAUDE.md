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

<Jam boyunca güncellenecek. Her 6 saatte bir:>

**Çalışıyor ve ekranda görüldü:** boş proje: L_Ana ve L_Test (ışık, gök, sis), pakette açıldı. Oyun kodu yok.

**Derlendi, ekranda görülmedi:** <___>  ← 2'yi geçerse yeni özellik durur

**Bozuk:** <___>

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
