# Çizgi roman sahneleri

Oyun bu klasörü **dosyadan** okur, Unreal'e import gerekmez. Dosya yoksa koddan çizilmiş yer tutucu görünür.

- `sahne1.png` … `sahne4.png` — her biri **16:9**, önerilen **1280×720** (en az 1280 genişlik), PNG.
- Sahne başına 5 sn; altyazı metni koddan gelir (`Source/DisasterBoard/DBData.cpp` → `C.Comic`), resme yazı koyma.
  1. Beş arkadaş kasabada geziyor
  2. Oyuncakçının vitrininde tozlu bir kutu: DisasterBoard
  3. Kutudan minik kasaba çıkıyor, kartta "Afet gelmeden kasabayı hazırla"
  4. Masaya oturuyorlar — "Sıra sende!"
- Altyazı kutusu sol altta durur (resmin alt ~%20'si): önemli bir şeyi oraya çizme.
