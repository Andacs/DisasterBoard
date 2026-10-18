---
name: varlik-onay
description: DisasterBoard'a modelci ya da tasarımcıdan gelen PNG (2D) ve FBX (3D) dosyalarını kontrol eder, oyunda dener, sorun yoksa onaylar; hepsi bitince repoya gönderir. Yiğit bir .png / .fbx dosyası attığında ya da "dosya geldi", "bunu kontrol et", "onayla", "test et", "kaç tane kaldı", "teslim et", "gönder" dediğinde kullan.
---

# Varlık onayı (Yiğit'in bilgisayarı)

Yiğit modelci ve tasarımcıdan gelen dosyaları sana atar. Senin işin: her dosyayı kontrol aracından geçirmek, oyunda
yerinde görmek, sorun yoksa onaylı klasöre koymak, sorun varsa nedenini Yiğit'in tasarımcıya iletebileceği açık bir
dille söylemek. Hepsi onaylanınca Yiğit'e sorup repoya göndermek. Konuşma dili Türkçe.

Her şeyi `Tools/varlik_kontrol.py` yapar; elle dosya kopyalama, ölçü hesaplama yok. Ölçülerin ve adların tek kaynağı
`Tools/sablon_listesi.py`; tasarımcıların okuduğu liste `Docs/Sablonlar/OKU_BENI.md`.

## İlk sefer: kurulum kontrolü

Yiğit'in okuduğu adımlar: `Docs/YIGIT_KURULUM.md`. "kurulumu kontrol et" derse aşağıdakileri sırayla doğrula ve eksikleri söyle.

1. Repo güncel mi: `git pull` (sorun çıkarsa dur, Yiğit'e söyle).
2. Python ve Pillow: `python -c "import PIL"`; yoksa `python -m pip install pillow`.
3. Blender (3D kontrolü için): `C:\Program Files\Blender Foundation\Blender*\blender.exe` var mı. Başka yerdeyse
   `BLENDER` ortam değişkeni ya da Yiğit'ten yolu iste. Blender yoksa 3D dosyalar kontrol edilemez, 2D'ye devam.
4. Test paketi (oyun): `Build/TestKiti/Windows/DisasterBoard.exe` var mı. Yoksa önce Yiğit'in indirdiği ZIP'i ara:
   `DisasterBoard_TestKiti*.zip` (İndirilenler / `~/Downloads`, Masaüstü). Bulursan `Build/TestKiti/` altına aç
   (PowerShell: `Expand-Archive <zip> Build/TestKiti -Force`), içinden `Windows/DisasterBoard.exe` çıktığını doğrula.
   Bulamazsan kendin indir ve aç (361 MB, birkaç dakika; bu klasör git'e girmez, Unreal kurmak ya da derlemek
   gerekmez). Link boş ya da 404 verirse Yiğit'e "ZIP'i Andaç'tan iste" de:
   ```
   curl -L -o Build/TestKiti.zip https://github.com/Andacs/Tetkik/releases/download/test-kiti-1/DisasterBoard_TestKiti.zip
   ```
   sonra ZIP'i `Build/TestKiti/` altına aç (PowerShell: `Expand-Archive Build/TestKiti.zip Build/TestKiti`),
   `Build/TestKiti/Windows/DisasterBoard.exe` oluştuğunu doğrula, `Build/TestKiti.zip`'i sil.
   Yiğit ZIP'i kendisi indirdiyse onu da aynı yere açabilirsin.

## Her dosya için

1. **Dosyanın yolu:** Yiğit dosyayı sürükleyip bıraktıysa yolu mesajdadır; değilse yolu sor. Birden çok dosya
   birlikte verilebilir (bir klasör de olur).
2. **Çalıştır:** `python Tools/varlik_kontrol.py "<dosya1>" "<dosya2>" ...`
   - 2D dosyalar için oyun açılır ve kendi kendine oynar (~40 sn). Yiğit'e "oyun açılacak, pencereye dokunma" de.
   - 3D dosyalar Blender'da arka planda açılır (~10 sn).
3. **Çıktıyı oku:**
   - `ONAY`: araç onayladı. **Yine de gözle bak** (Read aracıyla resmi aç):
     - 2D: çıktının sonunda listelenen `Art/Onayli/_ekran/V_*.png` oyun görüntüleri. Görsel yerinde mi, kesik,
       ters, bulanık, yanlış renkte değil mi, karakterin kolu doğru yöne mi gösteriyor, yazı içermiyor mu (logo hariç)?
     - 3D: `Art/Onayli/3D/onizleme/<ad>.png`. Model kırmızı kutunun içinde mi, "ÖN" yazan tarafa mı bakıyor
       (kapı/yüz o tarafta mı; arabanın burnu ÖN ucunda mı), ters ya da yan yatmış değil mi?
     - Gözüne bir sorun çarparsa Yiğit'e göster ve sor. Geri almak için: `python Tools/varlik_kontrol.py --geri <dosya adı>`.
   - `RED`: sorun var. Sebepleri (araç Türkçe yazar) Yiğit'e, tasarımcıya aynen iletebileceği kısa bir liste
     olarak söyle. Dosyayı sen düzeltmeye çalışma (çizim/model tasarımcının işi). Kopyası ve nedeni
     `Art/Reddedildi/` altında durur.
     - Tek istisna **yanlış ad**: araç "En yakın ad: X" diyorsa Yiğit'e "bu X mi?" diye sor; evet derse dosyayı
       doğru adla kopyalayıp (`Art/_deneme` dışında, örneğin dosyanın yanına) yeniden çalıştır.
   - `BEKLE`: kurulum sorunu (test paketi ya da Blender yok, oyun açılmadı). Kurulum adımına dön.
4. **Kısa özet ver** ve sıradakine geç: kaç dosya onaylandı, kaç tane kaldı (`--durum` çıktısındaki toplam).
   Yiğit sonraki dosyayı atana kadar bekle.

Aynı adla yeni dosya gelirse (tasarımcı düzeltti) aynı komutla tekrar kontrol et; onaylıysa eskisinin yerine geçer.

**Büyük tuvale çizilmiş ya da adı farklı ikonlar** (ör. 6000x6000 `kedimama.png`): resme bakıp hangi şablon olduğunu
anla (`Docs/Sablonlar/OKU_BENI.md` tablosu; emin değilsen Yiğit'e sor), sonra
`python Tools/varlik_kontrol.py --ikon "<dosya>" esya_kedi_mamasi.png "<dosya2>" kit_deprem.png ...`. Araç kırpar,
ölçüye ve daireye oturtur, doğru adı koyar, kontrol eder ve hepsini tek seferde oyunda dener.

**Rehber karakterler (A = Mert, B = Asil, C = Yiğit) iki poz olarak gelir:** duruş ve gösterme (kolu kalkık). Büyük
tuvale çizilmiş, adı farklı olabilir. Hangisinin duruş hangisinin gösterme olduğunu resme bakıp anla (emin değilsen
Yiğit'e sor), sonra: `python Tools/varlik_kontrol.py --karakter A "<duruş>" "<gösterme>"`. Araç ikisini aynı kesimle
320x512'ye oturtur, adlarını koyar ve kontrol eder. "Arka plan siyah" derse: dosya mesajla fotoğraf olarak gelmiş,
şeffaflık kaybolmuş; Yiğit çizerden orijinal PNG'yi dosya/belge olarak istesin.

## Durum ve teslim

- `python Tools/varlik_kontrol.py --durum`: 85 dosyanın listesi (53 2D, 32 3D), hangisi onaylı, hangisi bekliyor.
  Tablo `Art/Onayli/RAPOR.md`.
- **Hepsi onaylanınca** (araç "HEPSİ TAMAM" der) Yiğit'e sor: "Hepsi onaylı, Andaç'a göndereyim mi?" Evet derse:
  `python Tools/varlik_kontrol.py --teslim`. Bu, `Art/Onayli/` klasörünü commit eder, `git pull --rebase` yapar
  ve push eder. Andaç oradan alıp oyuna topluca aktarır.
- Yiğit hepsi bitmeden göndermek isterse: `--teslim --kismi`.

## Yapma

- `Content/`, `Source/`, `Config/` ve `Art/Onayli/` dışında hiçbir şeyi değiştirme, commit etme. Onaylı dosyaları
  `Content/` altına Andaç aktarır (`--aktar`), sen değil.
- Yiğit sormadan push etme. `git push --force`, `reset --hard` yok. `git pull` çakışırsa dur, Yiğit'e "Andaç'a
  yaz" de.
- Aracın kurallarını gevşetmek için kodu değiştirme. Bir kural yanlış görünüyorsa Yiğit'e söyle; Andaç karar verir.
