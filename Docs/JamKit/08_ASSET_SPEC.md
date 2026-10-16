# Asset spesifikasyonu

Modelcilerin (Mert, Asil) üretime başlamadan önce okuyacağı dosya. Buradaki her sayı **bağlayıcı**;
değiştirilecekse Gün 0'da değiştirilir ve herkes aynı anda öğrenir.

Sayılar varsayılan ve gerekçeli. Beğenmediğiniz varsa **şimdi** tartışın — 40 asset üretildikten
sonra değil.

---

## 1. Temel birim: 50 cm modül

Unreal'de 1 birim = **1 cm**. Bu projenin temel modülü:

> ### 1 modül = 50 cm = 50 Unreal birimi

**Her nesnenin her boyutu 50'nin katı olacak.** 50, 100, 150, 200…

**Neden bu kadar sert:** bu bir denge/istifleme oyunu. 50 yerine 49.7 genişliğinde bir kutu,
üstüne konan şeyle arasında boşluk bırakır; fizik o boşlukta sallanır ve kule kendiliğinden
devrilir. Oyuncu bunu "oyun bozuk" diye okur, "zorlu" diye değil.

**Neden 50:** izometrik kamerada okunur büyüklük, ve 1 m'lik modülden daha fazla çeşitlilik veriyor.

> Serbest yerleştirme yapacak olsanız bile bu kural geçerli — grid'e yapışma meselesi değil,
> parçaların birbirine oturması meselesi.

---

## 2. Boyut tablosu

| Kategori | Modül | Santim (G×D×Y) | Not |
|---|---|---|---|
| Küçük nesne (sandık, kutu) | 1×1×1 | 50×50×50 | İstifleme birimi |
| Orta nesne | 1×1×2 | 50×50×100 | |
| Küçük bina | 2×2×2 | 100×100×100 | |
| Orta bina | 2×2×3 | 100×100×150 | |
| Büyük bina | 3×3×4 | 150×150×200 | En büyük yapı |
| İnsan figürü | — | **160 cm** boy | Ölçek referansı |
| Platform (zemin) | 12×12 | 600×600 | Oyun alanı |

**İnsan 160 cm, çünkü ölçek referansı insan bedeni.** Bina 150 cm ise bir insandan alçak demektir —
izometrik karton estetiğinde bu doğru, oyuncaklı bir dünya. Binaları gerçek ölçüye çıkarırsanız
platform da büyür ve kamera uzaklaşır; karton detayı kaybolur.

---

## 3. Pivot — origin tabanın ortasında

Blender'daki object origin, Unreal'de pivot olur.

> **Kural: origin, nesnenin tabanının tam ortasında. X ve Y merkezde, Z = 0 (en alt nokta).**

Blender'da: nesneyi seç → `Shift+S` ile imleci tabana koy → `Object → Set Origin → Origin to 3D Cursor`.

**Neden:** sürükle-bırak-yerleştir mekaniğinde pivot nereye denk gelirse nesne oraya oturur. Merkezde
pivot = her bina yere yarı gömülü. SonZarf'ta bu hata üç farklı biçimde tekrarlandı.

**İstisna yok.** Kapı, baca, çatı parçası — hepsi taban ortası.

---

## 4. Collision — basit kutu, elle

Fizikle devrilen bir kule yapıyorsunuz; collision şekli mekaniğin kendisi.

> **Kural: her mesh'in yanında `UBX_` önekli basit bir kutu collision mesh'i olacak.**

- Kutu, nesnenin bounding box'ıyla **birebir aynı** olacak (50'nin katları, yani zaten tam oturur)
- Blender'da ayrı bir mesh objesi olarak, aynı FBX'in içinde export edilir
- İsim: `UBX_<MeshAdı>_01`

**Neden otomatik collision değil:** Unreal'in auto-convex'i karmaşık hull üretiyor; karmaşık hull
fizikte **kararsız**. Kuleler titrer, kendiliğinden kayar, sebepsiz devrilir. Basit kutu hem
hızlı hem stabil — ve karton kutu formları zaten kutu.

Diğer prefix'ler gerekirse: `USP_` küre · `UCP_` kapsül · `UCX_` convex (son çare).

---

## 5. Poligon bütçesi

| Kategori | Üst sınır (tris) |
|---|---|
| Küçük nesne | 300 |
| Bina | 1.500 |
| İnsan figürü | 3.000 |
| Platform | 2.000 |

Lowpoly karton için fazlasıyla yeterli. Karton estetiği zaten düz yüzey ve sert kenar demek —
bütçeyi zorluyorsanız muhtemelen stile aykırı bir şey yapıyorsunuz.

**Bevel yok, subdivision yok.** Karton kenarı keskindir.

---

## 6. UV ve texture

### Tek tileable karton dokusu

Bütün proje **tek bir karton dokusu** kullanıyor:

```
T_Cardboard_D        1024×1024, tileable, renkli (diffuse)
T_Cardboard_N        1024×1024, tileable (normal) — opsiyonel, zaman kalırsa
```

### UV ölçeği — bu kural sertleşmeli

> **1 UV tile = 1 metre.** Yani 50 cm'lik bir yüzey UV'de 0.5 tile kaplar.

**Neden:** karton oluğu (corrugation) her nesnede **aynı büyüklükte** görünmeli. Bir kutuda iri,
binada ince görünürse ikisi aynı malzemeden yapılmamış gibi durur ve bütün estetik dağılır.

Bu ölçek tutarlılığı tek tek göz kararıyla sağlanamaz; Blender'da UV'yi ölçeklerken sayıya bakın.

### Atlas yok

Her nesne aynı tileable dokuyu kullandığı için atlas'a gerek yok. Renk/varyasyon **material
instance parametresiyle** yapılır, yeni doku üretilerek değil.

---

## 7. Materyal

> **Modelciler materyal yapmaz.**

Unreal'de tek master materyal:

```
M_Cardboard              master
  MI_Cardboard_Kraft     açık kahve (varsayılan)
  MI_Cardboard_Dark      koyu
  MI_Cardboard_White     beyaz karton
```

Renk farkı master'daki bir `Tint` parametresinden gelir. Blender'da shader kurmayın — aktarılmıyor
zaten (bkz. `07_BLENDER_UNREAL.md`).

Mesh'te **tek materyal slotu** olsun. Çok slotlu mesh, çok material instance ataması demek ve
jam'de kimsenin vakti yok.

---

## 8. İsimlendirme

| Tip | Kalıp | Örnek |
|---|---|---|
| Static mesh | `SM_<Kategori>_<Ad>` | `SM_Bina_Ev01` |
| Collision | `UBX_<MeshAdı>_01` | `UBX_SM_Bina_Ev01_01` |
| İskeletli mesh | `SK_<Ad>` | `SK_Baskan` |
| Animasyon | `A_<Karakter>_<Eylem>` | `A_Baskan_Idle` |
| Texture | `T_<Ad>_<Tip>` | `T_Cardboard_D` |
| Materyal | `M_<Ad>` | `M_Cardboard` |
| Material instance | `MI_<Ad>_<Varyant>` | `MI_Cardboard_Dark` |
| Seviye | `L_<Ad>` | `L_Main`, `L_Test` |

**Kategoriler:** `Bina`, `Nesne`, `Zemin`, `Dekor`

Türkçe isim kullanıyoruz ama **Türkçe karakter yok** — `SM_Bina_Kutuphane` (ü/ş/ğ yok). Asset
isimlerindeki Türkçe karakter hem araçlarda hem PowerShell script'lerinde sorun çıkarıyor.

---

## 9. Klasör yapısı

```
Content/<ProjeAdi>/
    Meshes/
        Bina/
        Nesne/
        Zemin/
    Materials/
    Textures/
    Characters/
    Maps/
    UI/
Content/Vendor/          <- indirilen paketler, DOKUNULMAZ
```

---

## 10. FBX export ayarları

Blender → `File → Export → FBX`:

| Ayar | Değer |
|---|---|
| Path Mode | Copy (+ gömme ikonu açık) |
| Limit to | **Selected Objects** |
| Object Types | Mesh (+ Armature, karakterse) |
| Transform → Forward | **-Y Forward** |
| Transform → Up | **+Z Up** |
| Apply Scalings | **FBX All** |
| Apply Unit | işaretli |
| Geometry → Apply Modifiers | işaretli |
| Geometry → Smoothing | **Face** |
| Tangent Space | işaretli |

**Export öncesi Blender'da:** `Ctrl+A → Rotation & Scale` (transform'ları uygula).

> ⚠️ **Bu tabloyu ezberlemeyin, doğrulayın.** Blender sürümüne göre `Apply Scalings` davranışı
> değişiyor. Gün 0 testinde 50 cm'lik bir küp Unreal'de **50 birim** görünmeli. Görünmüyorsa ayarı
> düzeltin, düzeltilmiş hâli bu dosyaya yazın, herkes onu kullansın.

---

## 11. Teslim kontrol listesi

Modelci bir asset'i "bitti" demeden önce bunların hepsini işaretleyecek:

- [ ] Bütün boyutlar 50'nin katı
- [ ] Origin tabanın ortasında (X, Y merkez · Z en alt)
- [ ] `Ctrl+A → Rotation & Scale` uygulandı
- [ ] Modifier'lar uygulandı
- [ ] Tek materyal slotu
- [ ] UV ölçeği 1 tile = 1 m
- [ ] `UBX_` collision mesh'i var ve bounding box'la aynı
- [ ] İsimlendirme kalıba uygun, Türkçe karakter yok
- [ ] Tris bütçesinin altında
- [ ] FBX export preset'iyle çıkarıldı

**Unreal'e aktaran kişi ayrıca:**

- [ ] Boyut doğru mu? (Unreal'de ölç, Blender'daki cm değeriyle aynı olmalı)
- [ ] Yere oturuyor mu, gömülü/havada değil?
- [ ] Collision basit kutu olarak geldi mi? (Mesh editöründe görünür)
- [ ] Üstüne bir tane daha atınca fizik stabil mi, titriyor mu?

---

## 12. Model listesi

Üretim takibi. Jam başında doldurulup güncellenecek.

| # | İsim | Kategori | Modül | Kim | Durum |
|---|---|---|---|---|---|
| 1 | `SM_Zemin_Platform` | Zemin | 12×12 | | ⬜ |
| 2 | `SM_Nesne_Kutu01` | Nesne | 1×1×1 | | ⬜ |
| 3 | `SM_Bina_Ev01` | Bina | 2×2×2 | | ⬜ |
| 4 | | | | | ⬜ |
| 5 | | | | | ⬜ |

**Durum:** ⬜ yapılmadı · 🟡 Blender'da bitti · 🟢 Unreal'de ve ekranda görüldü

> `05_CALISMA_KURALLARI.md`: **ekranda görmediğin şey yoktur.** Bir asset "🟡" kaldığı sürece
> oyunda yok sayılır.

---

## 13. İlk üretim dalgası — öneri

48 saatte 40 asset üretilmez. Çekirdek döngü için gereken minimum:

1. `SM_Zemin_Platform` — oyun alanı
2. `SM_Nesne_Kutu01` — istiflenen temel birim
3. `SM_Bina_Ev01` · `SM_Bina_Ev02` — iki varyant yeter
4. `T_Cardboard_D` + `M_Cardboard`

**Bu beş şey bitmeden altıncıya geçilmeyecek.** Beşi bitince oyun oynanabilir; gerisi süs.
