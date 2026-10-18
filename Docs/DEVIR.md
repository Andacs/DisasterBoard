# DEVİR: Tetkik (DisasterBoard) — 18 Ekim 03.10

Bu belge, işi başka bir bilgisayarda devralan kişi (ve onun Claude'u) içindir. Önce bunu, sonra kökteki
`CLAUDE.md`'yi oku. Claude Code kullanıyorsan ilk mesajın: **"Docs/DEVIR.md'yi ve CLAUDE.md'yi oku, sonra
sıradaki işe geç."**

---

## 1. Saatler (CIVIL GAME JAM 2026)

| Ne | Saat |
|---|---|
| **Yeni özellik yok** (kural 5, 36. saat) | **18 Ekim 06.00** — sonrası yalnız hata düzeltme ve paketleme |
| Git desteği (organizasyon) | 18 Ekim 12.00'ye kadar |
| **Teslim (form yüklemesi kaydı)** | **18 Ekim 18.00** — ZIP ~450 MB, yüklemeye erken başla |

Teslim adımları ve form alanları: `Docs/JamKit/10_TESLIM.md`.

---

## 2. Kurulum (yeni bilgisayar)

Ayrıntı: `Docs/JamKit/12_KURULUM_YENI_BILGISAYAR.md`. Kısaca:

1. **Git + Git LFS** (`git lfs install`). Repo: **https://github.com/Andacs/Tetkik** (public, eski adı
   DisasterBoard; eski bağlantı yönlenir). Tek dal: `main`.
   ```
   git clone https://github.com/Andacs/Tetkik.git C:\Dev\DisasterBoard
   ```
   Klasör adı `DisasterBoard` olsun (betikler ve belgeler bu yolu kullanıyor; zorunlu değil ama kolaylık).
2. **Unreal Engine 5.8** (Epic Launcher) + **Visual Studio 2022** "Game development with C++" iş yükü.
3. **Python 3** + `pip install pillow numpy scipy` (varlık aracı ve zemin temizleme için).
4. İlk derleme: editör **kapalıyken** `Build\derle.bat`. Bu makinede `-NoXGE -MaxParallelActions=2` şarttı
   (betikte var). İlk derleme uzun sürer.
5. Oynamak: `Build\oyna.bat` (editörden `-game`).

---

## 3. Oyunun adı

- Görünen ad **Tetkik** (takvim başlığı, hikâyedeki kutu, pencere başlığı, README, GitHub).
- **Unreal projesinin / modülün iç adı `DisasterBoard` kalır** (`DisasterBoard.uproject`, modül, `DISASTERBOARD_API`,
  `Source/DisasterBoard/`). CLAUDE.md kural 1: yeniden adlandırma projeyi kırar. Bu yüzden paketteki exe
  `DisasterBoard.exe`; README'de açıklandı.

---

## 4. Durum (03.10)

**Oyun baştan sona çalışıyor.** Otomatik test (`DBTest`): 8/8 istek çözülür, 25 TL kalır, 2 hata (biri bilerek),
başarım 6/8 (test herkesle konuşmuyor ve bilerek hata yapıyor). Son commit: `b94acb9` + bu belge.

Oyunun akışı: hikâye (4 sahne) → takvim menüsü → Oyna → Mert tarayıcıyı gösterir → tarama → Asil çanta
fabrikasını anlatır (3 sipariş) → Yiğit şehir önlemlerini, Q/E'yi anlatır → oyun (8 risk) → sonuç: fotoğraf
albümü + başarımlar.

### Bu gece eklenenler (hepsi main'de, testten geçti)
- 8 başarım (sağ üstte kart), çantalar bitince Asil "masayı kapat" der, Q/E dersi, Menü düğmesi → takvim
  (ilerleme korunur, "Oyuna devam et").
- Sonuç ekranı: kraft kâğıt albüm, çözülen her binanın fotoğrafı (polaroid), "neden önemli", başarım rozetleri.
- Çanta fabrikası: TESLİM düğmesi yerine bandın dibinde açık çanta (paket düşer, kapanır, uçup gider);
  büyük ikonlar; seçilen eşya pakete uçar.
- El çizimi arayüz çerçeveleri (panel, düğme, balon, not, kulp, oklar, rozet), radar tarayıcı.
- Sesler (hepsi koddan sentez): Tomodachi tarzı konuşma heceleri, korna (arabaya tıkla), tarayıcı, bant,
  fermuar, başarım, deklanşör, zar.
- Masada iki zar (tıklayınca atılır). Kupa, ördek, lamba kaldırıldı.
- Böcekler (sinek, kelebek, uğur böceği, örümcek + ağ), duvarda resmi jam afişi ve ekip logosu.
- Yiğit'in 3D modelleri: 11 bina, 12 kasabalı figürü, ağaç, araba, rögar, musluk, tahliye figürü, tahta, takvim.
- Codex ile üretilen 2D'ler (ekibin çizimleri referans): eksik ikonlar, 12 portre, tarayıcı, çanta, UI
  parçaları, Asil ve Yiğit pozları. Uyarı/tamam işaretleri ekibin yeni çizimi.
- Sağ tık artık duraklatma penceresini açmaz (yalnız Esc). Su birikintisi yuvarlak leke.

### Ekranda görülmemiş / doğrulanmamış (yapılması iyi olur)
- **Gerçek fareyle baştan sona bir oyun hiç oynanmadı** (test fonksiyonları doğrudan çağırıyor). Özellikle:
  fabrikada uçan eşya animasyonu, üzerine gelince bina/araba/zar hareketi, mini oyunlar (rögar, somun).
- **Sesler kulakla dinlenmedi**, yalnız ölçüldü (konuşma heceleri, korna, zar…).
- Ayarlar'daki Tam ekran / Grafik düğmeleri; 4:3 ya da ultra geniş ekranda arayüz (fabrika paneli ve albüm
  dar ekranda taşabilir).
- **Shipping paketi** bu geceki değişikliklerle hiç alınmadı (yalnız editörden test). Masaüstündeki
  `DisasterBoard_TestKiti.zip` ESKİ.

---

## 5. TESLİM İÇİN KALANLAR (sırayla)

1. **06.00'ya kadar:** biri gerçek fareyle tam bir tur oynasın, sesleri dinlesin. Bulunan hata → düzelt.
2. **README:**
   - Yapay zekâ beyanında **"Diğer" satırı hâlâ `[...]`**. Ekip başka bir yapay zekâ aracı kullandıysa
     (ChatGPT, Gemini, ses/müzik üretici…) yaz; kullanmadıysa `| Diğer | Kullanılmadı | Başka yapay zekâ aracı kullanılmadı. |`.
     Beyan edilmeyen kullanım = diskalifiye (madde 4.2). Bilinen kullanımlar zaten yazılı: Claude (kod, ses
     kodu, metin, Blender betikleri), Codex/OpenAI (2D görseller).
   - Hazır kaynaklar yazılı: Unreal temel şekiller, Baloo 2 font (OFL), Sketchfab CC BY 4.0 modeller
     (Low Poly Building – A.S.D., Low Poly Tree Pack – louieoliva, Low Poly Car Set – BetelgeuseMan,
     Low Poly Park – Das Toasty; oyunda Emeği Geçenler'de de anılıyor), jam afişi.
3. **Jam afişi izni:** duvardaki resmi CIVIL GAME JAM afişinin lisansı belirtilmemiş. **Organizasyona
   kullanım iznini sor.** İzin yoksa `Content/Arayuz/duvar_afis.png` yerine `python Tools/duvar_gorsel.py`
   ile çizilen yer tutucuyu koy (betik `duvar_logo.png`'yi de yazar; logoyu korumak için önce onu yedekle)
   ve README hazır kaynak satırından afişi çıkar.
4. **Shipping paketi:** editör kapalı → `Build\paketle.bat` (çıktı `Build\Package\Windows\`). Paketi aç,
   oyna, kapat.
5. **Teslim kontrolü:** `bash Tools/teslim_hazirla.sh` → **BOZUK: 0** olmalı (editör açıksa "editor acik"
   der, kapat). Son kontrolde README'deki `[...]` boşluklarını da yakalıyor.
6. **Tanıtım videosu:** `bash Tools/trailer_yap.sh` (≤ 60 sn, MP4, sesli; ZIP'ten ayrı yüklenir).
7. **Form:** `Docs/JamKit/10_TESLIM.md` tablosundaki alanlar. ZIP büyük, **16.00'da yüklemeye başla**.
8. Sunum: video dahil 3 dakika (10_TESLIM.md sonu).

---

## 6. Ekipten gelen dosyalar

- **Yiğit** modelleri/görselleri kendi bilgisayarında `Tools/varlik_kontrol.py` ile onaylıyor ve **kendi dalına**
  pushluyor: `yigit/onayli-modeller` (main'e değil). Yeni bir şey gönderince:
  ```
  git fetch
  git log --oneline main..origin/yigit/onayli-modeller
  git merge --no-ff origin/yigit/onayli-modeller
  ```
  Çakışma çoğunlukla `Art/Onayli/durum.json`, `Art/Onayli/RAPOR.md`, `README.md`'de çıkar:
  - `durum.json`: iki tarafın anahtarlarının birleşimi (bizimki 2D, onunki 3D için güncel), sonra
    `python Tools/varlik_kontrol.py --durum` RAPOR.md'yi yeniden yazar.
  - README: iki beyanı tek satırda birleştir (birini silme).
  - Sonra derle + `DBTest` + ekranlara bak + commit + push.
  Yiğit'in Claude'una verilecek mesaj: "Son değişikliklerin hepsini commit et ve yigit/onayli-modeller dalına
  pushla. main'e pushlama, pull/merge/rebase yapma, force push kullanma. Önce Unreal kapalı mı kontrol et."
- GitHub'da Yiğit'in açık PR'ı (`yigit/hazir-kaynak-beyani`) main'e elle işlendi; **kapatılabilir**.
- Onaylı varlık: **79 / 85**. Eksik: 4 hikâye sahnesi (`Content/Cizgiroman/sahne1..4.png`, yoksa çizilmiş yer
  tutucu görünür), `Arayuz/logo.png` (yoksa takvimde "Tetkik" yazısı), `SM_SuBirikintisi` (yoksa koddan leke).
  Hiçbiri zorunlu değil.
- Yeni 2D görsel gelirse: `python Tools/varlik_kontrol.py --ikon "<dosya>" <hedef_ad.png>` (büyük tuvali kırpar,
  oturtur, oyunda dener), sonra `--aktar` (Content'e kopyalar). Beyaz zeminli (yapay zekâ çıktısı) dosyalar
  için önce `python Tools/zemin_temizle.py <girdi klasörü> <çıktı klasörü>`. Rehber karakter pozları:
  `--karakter B <duruş> <gösterme>`.

---

## 7. Derleme, test, araçlar

| İş | Komut |
|---|---|
| Derle (editör KAPALI) | `Build\derle.bat` — "Live Coding is active" derse editörü kapat |
| Oyna | `Build\oyna.bat` |
| Otomatik test (~3 dk, kendi kapanır) | Git Bash'te: `MSYS_NO_PATHCONV=1 "/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "C:/Dev/DisasterBoard/DisasterBoard.uproject" /Game/DisasterBoard/Maps/L_Ana -game -windowed -ResX=1600 -ResY=900 -ExecCmds="DBTest" -log -abslog="C:/Dev/DisasterBoard/Saved/Logs/dbtest.log"` |
| Test sonucu | `grep "DB_TEST DONE" Saved/Logs/dbtest.log` → `para=25 hata=2`; görüntüler `Saved/Screenshots/WindowsEditor/DB_*.png` |
| Görsel turu | aynı komut, `-ExecCmds` yerine `-DBVarlikTest` (14 kare, `V_*.png`) |
| Ses kaydı/ölçüm | `-DBSesTest` → `Saved/DB_ses.wav` |
| Test paketi (Yiğit için, Development) | `Build\test_paketi.bat` → `Build\TestKiti\Windows` |
| Teslim paketi (Shipping) | `Build\paketle.bat` |
| Teslim kontrolü | `bash Tools/teslim_hazirla.sh` |
| Video | `bash Tools/trailer_yap.sh` |

Kod haritası (`Source/DisasterBoard/`): `DBData.cpp` bütün içerik ve metinler · `DBGameMode` akış, başarımlar,
fabrika, ses tetikleme · `DBHUD` bütün arayüz (Canvas) · `DBWorld` oda/tahta/binalar/modeller/efektler/zarlar ·
`DBPlayerController` girdi + otomatik testler · `DBSfx` ses sentezi · `DBMusic` müzik.

---

## 8. Tuzaklar (bu gece yaşandı)

- Editör açıkken derleme olmaz (Live Coding kilidi). Git pull/checkout öncesi de editörü kapat.
- Git Bash'te Unreal'a `/Game/...` yolu verirken `MSYS_NO_PATHCONV=1`.
- Kod dosyalarında satır sonu karışık (CRLF/LF); git `autocrlf` hallediyor, sorun değil.
- Bash heredoc'ta ters bölü (`\`) bozuluyor: düzenleme betiklerini dosyaya yazıp çalıştır.
- Arayüze emoji ya da Baloo 2 fontunda olmayan karakter koyma (oyun takılır).
- Bir adı (`U`, `S`, `W`, `H`) yerel değişken yapma: HUD'da bunlar yardımcı fonksiyon/üye; derleme hatası verir.
- `Art/AI_Seffaf/` ve `Art/AI_UI_Seffaf/` git'te izlenmeyen ara klasörler; silinebilir.
- `Content/DisasterBoard/Meshes/Deneme/` Yiğit'in onaysız ama kullanılan modelleri (yeşil tahta, tam takvim):
  oyunda görünüyor, Andaç "bu hâli iyi" dedi; silme.
