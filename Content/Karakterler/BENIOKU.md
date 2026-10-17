# Rehber karakterler (A = Mert, B = Asil, C = Andaç)

Oyun bu klasörü **dosyadan** okur, Unreal'e import gerekmez. Dosya yoksa koddan çizilmiş yer tutucu figür görünür.

Her karakter için iki PNG (şeffaf arka plan):

- `A_govde.png`, `B_govde.png`, `C_govde.png` — **gösteren kol hariç** bütün figür, ayakta, kameraya bakıyor.
  Ayaklar resmin **alt kenarında**, figür yatayda ortada. Önerilen 512 yükseklik.
- `A_kol.png`, `B_kol.png`, `C_kol.png` — gösteren kol **yatay, sağa doğru uzanmış**, işaret parmağı sağ uçta.
  Omuz resmin **sol kenarının ortasında** (oyun kolu buradan döndürür). Önerilen 256×96.

Oyun kolu omuzdan (gövdenin üst-orta noktasının ~%60 yukarısı) hedefe doğru döndürür ve hafifçe sallar
(gösterme hareketi). Konuşurken hedef yoksa el sallar.
