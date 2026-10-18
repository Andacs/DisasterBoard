# Tanıtım videosu — Yiğit'in Claude'u için iş belgesi (18 Ekim)

Hedef: **Sallantı'daki trailer gibi** oyunun kendi içinden çekilmiş, akıcı, altyazılı, müzikli ≤ 60 sn tanıtım
videosu: `Build/Teslim/DisasterBoard_Trailer.mp4`. Teslim kontrolünde (`bash Tools/teslim_hazirla.sh`) kalan tek
BOZUK bu. Teslim **bugün 18.00**; video ZIP'ten ayrı yüklenir (≤ 1 dk, MP4, sesli).

Önce kökteki `CLAUDE.md`'yi ve `Docs/DEVIR.md`'yi oku (kurallar, derleme, test). Saat 06.00'dan sonra oyuna yeni
özellik yok: **bu iş yalnız video çekim aracı**; oynanışı değiştirme.

---

## 1. Örnek: Sallantı (public repo)

https://github.com/Andacs/Sallanti — aynı yöntemle çekilmiş trailer'ı var (`Build/Teslim/Sallanti_Trailer.mp4`, 36,5 sn).
İncelenecek yerler (klonla ya da GitHub'dan oku):
- `Source/Sallanti/SLPlayerController.cpp`: `SLSinema()` (kaydı başlatır, kamerayı ayarlar), `CinemaStep()` (her
  0,5 sn bir sahne adımı), `Caption()` (alt yazı), kayıt sırasında her karenin `REC_f_#####.png` olarak alınması
  (`bRecording`, `RecFrame`, `FScreenshotRequest`).
- `SLHUD`: `bCinema`, `CinemaTitle`, `CinemaSub`, `CinemaCaption` — açılış başlığı ve altyazı çizimi (oyun arayüzü
  çekimde sade).
- `Tools/trailer_yap.sh` (Tetkik'te de aynısı var): oyunu `-benchmark -fps=30` ile açar (her kare tam 1/30 sn →
  makine yavaş da olsa video akıcı), kareleri ffmpeg ile müzikle birleştirir.

## 2. Tetkik'te yapılacaklar

1. **`DBSinema` konsol komutu** (`DBPlayerController`, `UFUNCTION(Exec)`, `DBTest`'in yanına). Oyun akışını
   `DBTest`'teki gibi GameMode fonksiyonlarını doğrudan çağırarak sürer (yeni oynanış yok), her karede
   `REC_f_#####.png` kaydeder. Önerilen sahneler (toplam ~50–55 sn, her biri 4–7 sn):
   1. Takvim menüsü, oda, duvardaki afiş — açılış başlığı **"TETKİK"** + alt satır **"afetlere hazır bir kasaba kur"**.
   2. Oyna → kamera takvimden tahtaya iner (rüzgar geçişi).
   3. Mert tarayıcıyı gösterir, tarama: radar halkaları, riskli yerlerde (!) — altyazı "Kasabanı tara".
   4. Çanta fabrikası: kit + eşyalar seçilir, paket çantaya düşer, çanta uçar — "Doğru afet çantasını hazırla".
   5. Şehir önlemleri: rögar mini oyunu (`bMiniAutoTest`), bina güçlendirme (çelik çaprazlar) — "Şehri önlemlerle koru".
   6. Kasabalıya tıklanır, konuşma balonu ve portre — "Komşularını dinle".
   7. Tahta oklarla döner (havada döner, iner).
   8. Kalan istekler çözülür → başarım kartı → sonuç: fotoğraf albümü — "Kasaba afetlere hazır!".
   9. Kapanış kartı: **"Tetkik · FreakLabs · CIVIL GAME JAM 2026"**.
   Çekim sırasında HUD'da hata ayıklama/test izi olmasın; Menü düğmesi ve görev şeridi gizlenebilir
   (yalnız `DBSinema` açıkken; normal oyun değişmesin).
2. **Müzik:** oyunun kendi müziği zaten koddan; `DBMusicWav` komutu 40 sn'lik müziği (yoğunluk 0→1)
   `Saved/DB_muzik_test.wav`'a yazar. Gerekirse süreyi videoya uyacak şekilde uzat (≥ 60 sn), dosyayı
   `Art/Audio/MUS_Ana.wav` olarak koy. Hazır müzik kullanılırsa README hazır kaynak beyanına yaz (kural 3.4).
3. **`Tools/trailer_yap.sh`'i uyarla:** `SINEMA="DBSinema"`, `MUZIK` yukarıdaki dosya, `HARITA` aynı kalır.
   Betiğin ikinci yarısındaki **Intro.mp4 bölümünü kaldır** (Tetkik açılışta video oynatmıyor; `Content/Movies`'e
   dosya koyma, pakete girer). Süre ≤ 60 sn olsun.
4. **Çek ve bak:** editör KAPALI → `Build\derle.bat` → `bash Tools/trailer_yap.sh`. Videodan birkaç kare çıkarıp
   (ffmpeg `-ss`) gözle kontrol et: altyazılar okunuyor mu, siyah/boş kare var mı, süre ≤ 60, ses var mı
   (`ffprobe`). Beğenilmeyen sahneyi düzelt, yeniden çek.

## 3. Bitirmeden

- `DBTest`'i çalıştır: hâlâ `DB_TEST DONE para=25 hata=2` (oynanış değişmedi).
- Kod değiştiği için paket eskidi: `bash Tools/teslim_hazirla.sh` paketi yeniden alır; sonuç **BOZUK: 0** olmalı
  (video artık var). ZIP `Build/Teslim/DisasterBoard_Windows.zip`, video `Build/Teslim/DisasterBoard_Trailer.mp4`
  (ikisi de git'e girmez; forma bu bilgisayardan yüklenir).
- README yapay zekâ beyanı "Kod" satırına "tanıtım videosu çekim komutu (DBSinema) ve video betiği" ekle.
- Commit + push (`main`; önce `git pull`). Force push yok. Andaç ve devralan arkadaşla **aynı anda** çalışmayın.
- `Docs/DEVIR.md`'ye kısa not: video hazır, nerede, kaç sn.
