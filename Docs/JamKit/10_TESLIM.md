# Son teslim — CIVIL GAME JAM 2026 (kitapçık v09)

Sallantı provasında (14–15 Ekim) bütün teslim baştan sona denendi: paket, ZIP, temiz klasörde açma,
trailer, README beyanları, public repo. Bu sayfa o denemenin sırası. Kural numaraları kitapçıktan.

> **Son saat: 18 Ekim 2026 Pazar 18.00 (UTC+3).** Yükleme **kaydı** sunucuda o saatten önce oluşmalı;
> düğmeye basmak yetmez (6.1). Git desteği isteyeceksek sınır **12.00** (6.3). Son 6 saatte yardım
> garantisi yok. Teslim ekranı: https://game.ktimo.com.tr/jam/teslim — giriş takım adı + organizasyonun
> verdiği 6 karakterli kod (5.1).

---

## Diskalifiye edenler (puan ne olursa olsun)

| Ne | Madde | Nasıl önlenir |
|---|---|---|
| Repo private ya da bağlantı yok | 6.5 | Repoyu **ilk saatte** public aç. `teslim_hazirla.sh` girişsiz açıyor |
| Beyan edilmemiş hazır kaynak / AI | 3.5, 4.4 | Kaynak eklendiği an README tablosuna satır. Sonradan hatırlanmıyor |
| Önceden yapılmış oyun | 3.1 | Yeni repo, ilk commit tema açıklandıktan sonra. Sallantı'dan **oyun kodu** taşınmaz |
| Çalışmayan build | 3.2, 6.6 | Temiz klasörde açıp ekranda gör. Organizasyon da girişsiz deniyor |

**Taşınabilenler (3.1: "genel araçların hazırlanması ön hazırlık olabilir"):** `araclar/` içindeki
derleme, paketleme, trailer, ekran görüntüsü ve teslim betikleri, README şablonu. README'de "Kod"
satırında jam öncesi hazırlanmış genel araçlar olarak yazılır (şablonda hazır cümle var).

---

## Proje açılırken (saat 0–1) — teslimin yarısı burada

**Tek komut:** `bash /c/Dev/JamKit/araclar/yeni_proje.sh <Ad> "<tek cümle>"` aşağıdaki 1–4'ü yapar
(3 dk 18 sn), boş projeyi derler ve haritaları kurar. Elle yapmak gerekirse sıra:

1. Yeni repo, **public** (`gh repo create FreakLabs-<Oyun> --public` ya da Andacs altında). Açıklamaya
   oyun + takım adı (9.6).
2. `araclar/derle.bat`, `paketle.bat`, `oyna.bat` → `Build/`; `teslim_hazirla.sh`, `trailer_yap.sh`,
   `pencere_goruntu.ps1` → `Tools/`. `README_SABLON.md` → `README.md`.
3. `.gitattributes`'a `*.sh text eol=lf` ve `*.bat text eol=crlf` (başka bilgisayarda klonlanınca
   betikler bozulmasın). `.gitignore`'a `Build/Package/`, `Build/Teslim/`, `Build/*.log` (ZIP repoya girmez; trailer
   repoya istenirse LFS'le elle eklenir).
4. `.uproject`'e `{"Name": "AndroidFileServer", "Enabled": false}` ve `{"Name": "XGEController",
   "Enabled": false}`. Birincisi açıkken editör her açılışta `DefaultEngine.ini`'ye yeni bir
   `SecurityToken` yazıyor (3.7); ikincisi paketi 2,5 saat astı.
5. Boş projeyi bir kez `paketle.bat` ile paketle. Süreyi yaz.

## Jam boyunca

- Hazır kaynak eklendiği **anda** README'deki beş kategoriden birine satır: ad, sağlayıcı, bağlantı,
  lisans, oyundaki yeri, değişiklik (3.4, EK A.4). Yeniden dağıtılamayan ham dosya repoya girmez,
  nasıl edinileceği yazılır (3.6).
- AI kullanımı aynı şekilde, ayrı tabloda (4.2). "Çok küçük" kullanım da yazılır.
- Oyun içinde jenerik varsa README'deki iki beyanla **aynı** içerik (3.3: kaynaklar özgün üretim gibi
  gösterilemez).
- **17 Ekim akşamı ilk tam teslim provası:** `bash Tools/teslim_hazirla.sh`. Son güne ilk paketi bırakma.

## 18 Ekim — teslim günü

| Saat | İş |
|---|---|
| **≤ 12.00** | Özellik dondurulmuş olsun. Git'te sorun varsa teknik ekibe şimdi yaz (6.3) |
| 12.00–14.00 | Hata avı. Her düzeltme commit + push |
| **14.00** | Trailer: `bash Tools/trailer_yap.sh` (≤ 60 sn, MP4, sesli). Sallantı'da çekim + ffmpeg birkaç dk |
| **14.30** | `bash Tools/teslim_hazirla.sh` → **BOZUK: 0** olana kadar düzelt. Sallantı'da tamamı 3 dk 22 sn |
| 15.00 | `Build/Teslim/deneme_ekrani.png`'ye **gözle bak.** Bir kişi ZIP'i başka bilgisayarda açıp oynasın |
| **16.00** | Forma yükle (aşağıdaki tablo). 348 MB'lık ZIP'in yükleme süresi bilinmiyor: erken başla |
| 16.30 | Makbuz: numara, sürüm, kayıt saati, dosya adları → `TESLIM_OZET.txt`'nin sonuna (5.5) |
| 16.30–18.00 | Tampon. Düzeltme gerekirse yeni sürüm başlatılabilir; son başarılı sürüm geçerli (5.4) |
| **18.00 sonrası** | Repoya **push yok**: oyun ile kaynak kodun uyumu repo kayıtlarından inceleniyor (5.6, 3.7) |

> **Kural: ZIP her zaman son push'lu commit'ten.** Betik paket son commit'ten eskiyse BOZUK der.
> Teslimden sonra repo, değerlendirme bitene kadar public kalır (5.6).

## Form alanları (5.2) — nereden kopyalanır

| Alan | Kaynak |
|---|---|
| Oyun adı, kısa açıklama | README'nin ilk iki satırı |
| Platform | Windows |
| ZIP (≤ 10 GB) | `Build/Teslim/<Proje>_Windows.zip` |
| Video (≤ 1 dk, ≤ 5 GB, MP4/AVI, ZIP'ten ayrı, bağlantı değil dosya) | `Build/Teslim/<Proje>_Trailer.mp4` |
| Public repo (HTTPS, girişsiz) | `TESLIM_OZET.txt` → Repo |
| Kurulum / çalıştırma / kontroller | README → "Çalıştırma" (EK A.3) |
| Hazır kaynak, 5 kategori | README → "Hazır kaynak beyanı" |
| AI, 5 kategori | README → "Yapay zekâ beyanı" |
| Kural kabulü | Temsilci; ekipten hak sahipliği onayı alınmış olmalı (9.8) |

## `teslim_hazirla.sh` ne yapar

Her madde TAMAM / BOZUK / ÖLÇÜLEMEDİ der; sonunda `Build/Teslim/TESLIM_OZET.txt` (commit, ZIP boyutu ve
sha256, video süresi, sonuçlar).

1. Çalışma ağacı temiz, son commit push'lu (3.7)
2. Gizli bilgi taraması: parola / anahtar / token, `.env`/`.pem` (3.7)
3. Repo PUBLIC ve girişsiz HTTP 200 (5.3, 6.5)
4. README: çalıştırma bölümleri, iki beyanda beş kategori, şablon boşluğu kalmamış (3.4, 4.2, EK A)
5. Shipping paket (`PAKETLE=0` ile atlanır), paket son commit'ten yeni mi
6. ZIP (Windows `tar.exe`; Git Bash'inki zip yazamaz), ≤ 10 GB
7. ZIP'i temiz klasöre açar, oyunu başlatır, 45 sn sonra **yalnızca oyun penceresinin** görüntüsünü alır
   (`BEKLE=60` ile uzatılır), simsiyah mı bakar, oyunu kapatır
8. Video ≤ 60 sn ve sesli

Ölçemediği: oyunun **oynanabildiği**, fare tıklamaları, sesin kulağa nasıl geldiği. Onlar insan işi.

## Final sunumu (EK A.7)

Video dahil toplam **3 dakika**. Trailer arkada oynar, bir kişi anlatır; soru-cevap yok. Jüri aynı anda
oyunu oynuyor: anlatım "ne yapıyorsun, neden zor, nasıl kazanırsın" sırasıyla, ilk 20 saniyede mekanik.
