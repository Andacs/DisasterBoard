# Docs/Sablonlar/OKU_BENI.md: modelci ve tasarımcı için iş listesi + ölçüler (Tools/sablon_listesi.py'den).
#   python Tools/sablon_belge.py <çıktı klasörü>
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sablon_listesi import ASSETS_2D, ASSETS_3D

OUT = sys.argv[1] if len(sys.argv) > 1 else "Sablonlar"
PIVOT_2D = {"ayak": "alt orta (ayaklar)", "omuz": "sol orta (omuz)", "altyazi": "-", "daire": "orta", "": "orta"}

def table_3d(cat_list):
    rows = ["| Dosya | G x D x Y (cm) | Ne |", "|---|---|---|"]
    for name, w, d, h, cat, note in ASSETS_3D:
        if cat in cat_list:
            rows.append(f"| `{name}.fbx` | {w:g} x {d:g} x {h:g} | {note} |")
    return "\n".join(rows)

def table_2d(prefix):
    rows = ["| Dosya (`Content/` altında) | Piksel | Pivot | Ne |", "|---|---|---|---|"]
    for path, W, H, cat, note, mark in ASSETS_2D:
        if path.startswith(prefix):
            rows.append(f"| `{path}` | {W} x {H} | {PIVOT_2D[mark]} | {note} |")
    return "\n".join(rows)

n3, n2 = len(ASSETS_3D), len(ASSETS_2D)
doc = f"""# DisasterBoard: modelci ve tasarımcı şablonları

Bu klasörde oyuna girecek her 3D model ve 2D görsel için **oyundaki gerçek ölçüsünde bir şablon** var:
{n3} adet 3D kutu (hitbox) ve {n2} adet 2D PNG. Her şablonun üstünde **dosya adı ve ölçüsü yazılı**.
Şablonun adı = teslim edeceğin dosyanın adı. Ölçüye uyarsan model/görsel oyuna dokunmadan oturur.

```
3D/DisasterBoard_3D_Sablonlar.blend   hepsi bir arada, etiketli (1 Blender birimi = 1 cm)
3D/FBX/<ad>.fbx                       her kutu ayrı (kendi sahnene referans olarak al)
3D/olcu_tablosu_*.png                 önden görünüş, ölçekli: binalar, nesneler, sahne
2D/<klasör>/<ad>.png                  tam piksel ölçüsünde boş şablon
```

## 3D: modelci

### Kurallar
1. **Birim santim.** Blender: Scene > Units > Unit Scale **0.01**, Length **Centimeters** (şablon .blend'i böyle).
   Oyun minyatür bir masa oyunu: tahta 128 cm, bir karo 8 cm, bir bina 2x2 karoya (14,6 x 14,6 cm) oturur.
2. **Model kırmızı/renkli kutunun içinde kalır.** Kutu = modelin en büyük hâli (çatı, baca, anten dahil).
   Küçük olabilir, taşamaz. Binaların tabanı (plaka) kutunun tabanını tam doldursun.
3. **Pivot (origin) tabanın ortası**, modelin en alt noktası Z = 0. Şablon kutuların pivotu zaten böyle.
4. **Ön yüz -Y.** Kutunun üstünde kırmızı **ÖN** yazan yüz kameraya / yola bakan taraf (arabada gidiş yönü).
5. **Lowpoly, karton masa oyunu:** düz renkli yüzler, doku yok ya da çok basit. Bir malzeme yeter;
   renkler kodla da boyanabiliyor (araba, tahliye kişisi).
6. **Ad:** `SM_...` aynen tablodaki gibi. Kasabalılar `SM_Kasabali_<Ad>` (12 kişi, adlar tabloda).
7. **FBX dışa aktarma:** File > Export > FBX, *Selected Objects*, Object Types: Mesh,
   **Apply Scalings: FBX All**, **Forward: -Y Forward**, **Up: Z Up**, *Apply Unit* açık, *Apply Transform* kapalı.
   Model sahnede 0,0,0'da, tabanı Z = 0'da dursun; dışa aktarmadan önce Ctrl+A > All Transforms.
8. **Teslim:** `Art/Modeller/<ad>.fbx` (ve istersen `.blend`), commit + push. Oyuna bağlamayı Andaç yapar;
   `Content/` altına kendin bir şey koyma.

Kontrol: FBX'i Blender'a geri içe al, N panelinde Dimensions tablodakiyle aynı olmalı (± birkaç mm).

### Binalar (taban 14,6 x 14,6 cm, hepsi aynı)
{table_3d(("Bina",))}

### Nesneler ve figürler
{table_3d(("Nesne", "Figür"))}

### Sahne
{table_3d(("Sahne",))}

## 2D: tasarımcı

### Kurallar
1. **Piksel ölçüsü tam şablon kadar** (büyük çizip küçültebilirsin; teslim bu boyutta).
2. **Şeffaf arka planlı PNG** (çizgi roman sahneleri hariç: onlar tam kare resim).
3. Şablonu aç, üstüne yeni katmanda çiz, **şablon katmanını sil**, aynı adla kaydet.
   Kırmızı çerçeve = görselin sınırı. Mavi kesikli çizgi = güvenli alan (önemli kısım içinde kalsın).
   İkonlarda mavi daire: ikon daire içine sığsın (oyun ikonları yuvarlak düğmede gösterir).
4. **Yazı koyma** (logo hariç): oyun yazıları kendisi yazar. Çizgi romanda sarı alan = altyazı yeri, boş bırak.
5. **Karakterler:** gövde ve kol ayrı. Gövde `*_govde.png`: gösteren kol HARİÇ bütün figür, ayaklar alt kenarda
   (yeşil nokta). Kol `*_kol.png`: yatay, sağa uzanmış, omuz sol kenarın ortasında (oyun kolu buradan döndürür),
   işaret parmağı sağ uçta. A = Mert, B = Asil, C = Yiğit.
6. **Teslim:** bitmiş PNG'yi tablodaki yola koy: `Content/<yol>` (ör. `Content/Ikonlar/esya_su.png`),
   commit + push. **Şablonları `Content/` altına koyma**, oyun onları gerçek görsel sanar.

Şu an oyun `Karakterler/` ve `Cizgiroman/` görsellerini dosya varsa kendisi yükler (yoksa çizilmiş yer tutucu).
`Ikonlar/` ve `Arayuz/` görselleri gelince Andaç bağlayacak; adlar bu tablodaki gibi kalmalı.

### Karakterler
{table_2d("Karakterler/")}

### Çizgi roman (oyunun açılış hikâyesi)
{table_2d("Cizgiroman/")}

### İkonlar
{table_2d("Ikonlar/")}

### Arayüz
{table_2d("Arayuz/")}

## Şablonları yeniden üretmek (ölçü değişirse)

Ölçülerin tek kaynağı `Tools/sablon_listesi.py`. Değiştir, sonra repo kökünde:

```
"C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" -b --python C:\\Dev\\DisasterBoard\\Tools\\sablon_3d_blender.py -- C:\\Dev\\DisasterBoard\\Docs\\Sablonlar\\3D
python Tools/sablon_2d.py Docs/Sablonlar
python Tools/sablon_belge.py Docs/Sablonlar
```
"""
with open(os.path.join(OUT, "OKU_BENI.md"), "w", encoding="utf-8") as f:
    f.write(doc)
print("BELGE TAMAM")
