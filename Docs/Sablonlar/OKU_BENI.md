# DisasterBoard: modelci ve tasarımcı şablonları

Bu klasörde oyuna girecek her 3D model ve 2D görsel için **oyundaki gerçek ölçüsünde bir şablon** var:
21 adet 3D kutu (hitbox) ve 41 adet 2D PNG. Her şablonun üstünde **dosya adı ve ölçüsü yazılı**.
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
4. **Ön yüz -Y.** Kutunun üstünde kırmızı **ÖN** yazan yüz kameraya / yola bakan taraf. **Araba istisna:** burnu +X ucunda (gidiş yönü).
5. **Lowpoly, karton masa oyunu:** düz renkli yüzler, doku yok ya da çok basit. Bir malzeme yeter;
   renkler kodla da boyanabiliyor (araba, tahliye kişisi).
6. **Ad:** `SM_...` aynen tablodaki gibi. Kasabalılar `SM_Kasabali_<Ad>` (12 kişi, adlar tabloda).
7. **FBX dışa aktarma:** File > Export > FBX, *Selected Objects*, Object Types: Mesh,
   **Apply Scalings: FBX All**, **Forward: -Y Forward**, **Up: Z Up**, *Apply Unit* açık, *Apply Transform* kapalı.
   Model sahnede 0,0,0'da, tabanı Z = 0'da dursun; dışa aktarmadan önce Ctrl+A > All Transforms.
8. **Teslim:** bitmiş `<ad>.fbx` dosyasını (adı tablodaki gibi) **Yiğit'e gönder**. Yiğit kontrol aracından geçirir
   (kutuya sığıyor mu, pivot, birim, üçgen sayısı); sorun varsa nedenini sana iletir. Repoya kendin koyma.

Kontrol: FBX'i Blender'a geri içe al, N panelinde Dimensions tablodakiyle aynı olmalı (± birkaç mm).

### Binalar (taban 14,6 x 14,6 cm, hepsi aynı)
| Dosya | G x D x Y (cm) | Ne |
|---|---|---|
| `SM_Bina_AyseTeyze.fbx` | 14.6 x 14.6 x 18.5 | Ayşe Teyze'nin evi. Küçük ev, beşik çatı. Gövde ~10.9 x 10.9. |
| `SM_Bina_Okul.fbx` | 14.6 x 14.6 x 21.9 | Okul. Düz çatı. Gövde ~12.2 x 12.2. |
| `SM_Bina_Siginak.fbx` | 14.6 x 14.6 x 11.6 | Sığınak: yarım küre / bunker, kapı, yeşil bayrak. Tsunamide sakinler buraya yürür. |
| `SM_Bina_Yilmazlar.fbx` | 14.6 x 14.6 x 20.9 | Yılmazların evi (bebekli aile). Beşik çatı. Gövde ~12.2 x 10.9. |
| `SM_Bina_EskiApartman.fbx` | 14.6 x 14.6 x 43.8 | Eski apartman, en yüksek bina, kolonlarda çatlak. Düz çatı + su deposu. Gövde ~12.2 x 12.2. |
| `SM_Bina_Kitapci.fbx` | 14.6 x 14.6 x 20.3 | Kitapçı (Elif). Beşik çatı. Gövde ~12.2 x 12.2. |
| `SM_Bina_Firin.fbx` | 14.6 x 14.6 x 17.3 | Fırın (Ali Usta). Baca olabilir. Gövde ~12.2 x 10.9. |
| `SM_Bina_Market.fbx` | 14.6 x 14.6 x 17.3 | Bakkal / market (Rıza Abi). Gövde ~10.9 x 10.9. |
| `SM_Bina_OgrenciEvi.fbx` | 14.6 x 14.6 x 24.3 | Öğrenci evi (Can ve kedisi Pamuk). Düz çatı. Gövde ~10.9 x 12.2. |
| `SM_Bina_Park.fbx` | 14.6 x 14.6 x 8.5 | Park: çim, 2 ağaç, bank. Toplanma alanı. |
| `SM_Bina_SahilEvi.fbx` | 14.6 x 14.6 x 18.5 | Sahil evi (balıkçı Murat). Denize bakar. Gövde ~10.9 x 10.9. |
| `SM_Bina_AhsapEvler.fbx` | 14.6 x 14.6 x 18.5 | Bitişik ahşap evler (Nermin Hanım). Gövde ~12.2 x 10.9. |

### Nesneler ve figürler
| Dosya | G x D x Y (cm) | Ne |
|---|---|---|
| `SM_Agac.fbx` | 6.7 x 6.7 x 9.1 | Ağaç (gövde + yuvarlak tepe). Boş arsalarda ve parkta. |
| `SM_Araba.fbx` | 6 x 3 x 3.4 | Araba. Uzun kenar X; burnu (farlar) +X ucunda, şablonda ÖN yazan uç: araba o yöne gider. 3 renk varyantı kodla boyanır. |
| `SM_YanginMuslugu.fbx` | 3.2 x 3.2 x 5.5 | Yangın musluğu. Bozukken kod eğik gösterir, tamirde doğrultur; tek mesh yeter. |
| `SM_RogarKapagi.fbx` | 5.4 x 5.4 x 0.6 | Rögar kapağı (ızgaraya konur). Yassı daire, üstünde çubuklar. |
| `SM_SuBirikintisi.fbx` | 7.2 x 7.2 x 0.3 | Su birikintisi (yolda, sel). Yassı, düzensiz kenar olabilir. |
| `SM_Kasabali.fbx` | 3.4 x 3.4 x 8.5 | Kasabalı figürü (binanın önünde durur, tıklanınca konuşur). 12 kişi: AyseTeyze, SelmaOgretmen, HasanAmca, Zeynep, KemalBey, Elif, AliUsta, RizaAbi, Can, Deniz, MuratAbi, NerminHanim. İsim: SM_Kasabali_<Ad>. |
| `SM_TahliyeKisi.fbx` | 2.6 x 2.6 x 6.9 | Tahliyede sığınağa yürüyen sakin (4 tane, kod renklendirir). |

### Sahne
| Dosya | G x D x Y (cm) | Ne |
|---|---|---|
| `SM_Takvim.fbx` | 96.5 x 2 x 74.3 | Duvar takvimi (çerçeve + spiral). Sayfa alanı 88.5 x 64.3, ÖN odaya bakar. Yazılar koddan, sayfaya çizilir. |
| `SM_Tahta.fbx` | 128 x 128 x 15.2 | Oyun tahtası: 13 x 13 karo (8 cm aralık, karo 6.6), pahlı kenar. Karo üstü Z = 13.8. Mevcut taslak: Art/Sahne/AnaMenu_OyunAlani.blend (Cube.002). |

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
6. **Teslim:** bitmiş PNG'yi (adı tablodaki gibi, ör. `esya_su.png`) **Yiğit'e gönder**. Yiğit kontrol aracından
   geçirir ve oyunda yerinde dener; sorun varsa nedenini sana iletir. Repoya kendin koyma.

Oyun bütün bu görselleri dosyadan okur: dosya yoksa çizilmiş yer tutucu görünür, dosya gelince kendiliğinden yerine
oturur. Adlar bu tablodaki gibi kalmalı.

### Karakterler
| Dosya (`Content/` altında) | Piksel | Pivot | Ne |
|---|---|---|---|
| `Karakterler/A_govde.png` | 320 x 512 | alt orta (ayaklar) | Mert: gösteren kol HARİÇ bütün figür, ayakta, kameraya bakıyor. Ayaklar alt kenarda, yatayda ortada. |
| `Karakterler/A_kol.png` | 256 x 96 | sol orta (omuz) | Mert: gösteren kol, yatay, SAĞA uzanmış, işaret parmağı sağ uçta. Omuz sol kenarın ortasında (oyun buradan döndürür). |
| `Karakterler/B_govde.png` | 320 x 512 | alt orta (ayaklar) | Asil: gösteren kol HARİÇ bütün figür, ayakta, kameraya bakıyor. Ayaklar alt kenarda, yatayda ortada. |
| `Karakterler/B_kol.png` | 256 x 96 | sol orta (omuz) | Asil: gösteren kol, yatay, SAĞA uzanmış, işaret parmağı sağ uçta. Omuz sol kenarın ortasında (oyun buradan döndürür). |
| `Karakterler/C_govde.png` | 320 x 512 | alt orta (ayaklar) | Yiğit: gösteren kol HARİÇ bütün figür, ayakta, kameraya bakıyor. Ayaklar alt kenarda, yatayda ortada. |
| `Karakterler/C_kol.png` | 256 x 96 | sol orta (omuz) | Yiğit: gösteren kol, yatay, SAĞA uzanmış, işaret parmağı sağ uçta. Omuz sol kenarın ortasında (oyun buradan döndürür). |

### Çizgi roman (oyunun açılış hikâyesi)
| Dosya (`Content/` altında) | Piksel | Pivot | Ne |
|---|---|---|---|
| `Cizgiroman/sahne1.png` | 1280 x 720 | - | Sahne 1: Beş arkadaş kasabada geziyor. Altyazıyı oyun yazar, resme yazı koyma. |
| `Cizgiroman/sahne2.png` | 1280 x 720 | - | Sahne 2: Oyuncakçının vitrininde tozlu kutu: DisasterBoard. Altyazıyı oyun yazar, resme yazı koyma. |
| `Cizgiroman/sahne3.png` | 1280 x 720 | - | Sahne 3: Kutudan minik kasaba çıkıyor, kartta: Afet gelmeden kasabayı hazırla. Altyazıyı oyun yazar, resme yazı koyma. |
| `Cizgiroman/sahne4.png` | 1280 x 720 | - | Sahne 4: Masaya oturuyorlar: Sıra sende!. Altyazıyı oyun yazar, resme yazı koyma. |

### İkonlar
| Dosya (`Content/` altında) | Piksel | Pivot | Ne |
|---|---|---|---|
| `Ikonlar/esya_sargi_bezi.png` | 128 x 128 | orta | Sargı Bezi (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_antiseptik.png` | 128 x 128 | orta | Antiseptik (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_yara_bandi.png` | 128 x 128 | orta | Yara Bandı (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_su.png` | 128 x 128 | orta | Su (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_el_feneri.png` | 128 x 128 | orta | El Feneri (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_duduk.png` | 128 x 128 | orta | Düdük (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_yangin_tupu.png` | 128 x 128 | orta | Yangın Tüpü (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_duman_maskesi.png` | 128 x 128 | orta | Duman Maskesi (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_kalp_ilaci.png` | 128 x 128 | orta | Kalp İlacı (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_astim_spreyi.png` | 128 x 128 | orta | Astım Spreyi (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_gozluk.png` | 128 x 128 | orta | Gözlük (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_bebek_mamasi.png` | 128 x 128 | orta | Bebek Maması (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_bebek_bezi.png` | 128 x 128 | orta | Bebek Bezi (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_kedi_mamasi.png` | 128 x 128 | orta | Kedi Maması (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_battaniye.png` | 128 x 128 | orta | Battaniye (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_powerbank.png` | 128 x 128 | orta | Powerbank (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/esya_onemli_belgeler.png` | 128 x 128 | orta | Önemli Belgeler (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın. |
| `Ikonlar/onlem_rogar_kapagi.png` | 128 x 128 | orta | Rögar Kapağı (alttaki şehir önlemleri paneli). Şeffaf arka plan. |
| `Ikonlar/onlem_tahliye.png` | 128 x 128 | orta | Tahliye (alttaki şehir önlemleri paneli). Şeffaf arka plan. |
| `Ikonlar/onlem_yangin_muslugu.png` | 128 x 128 | orta | Yangın Musluğu (alttaki şehir önlemleri paneli). Şeffaf arka plan. |
| `Ikonlar/onlem_ingiliz_anahtari.png` | 128 x 128 | orta | İngiliz Anahtarı (alttaki şehir önlemleri paneli). Şeffaf arka plan. |
| `Ikonlar/onlem_bina_guclendirme.png` | 128 x 128 | orta | Bina Güçlendirme (alttaki şehir önlemleri paneli). Şeffaf arka plan. |
| `Ikonlar/kit_ilk_yardim.png` | 128 x 128 | orta | İlk Yardım Çantası (fabrikada temel kit düğmesi, sipariş kartı). |
| `Ikonlar/kit_deprem.png` | 128 x 128 | orta | Deprem Çantası (fabrikada temel kit düğmesi, sipariş kartı). |
| `Ikonlar/kit_yangin.png` | 128 x 128 | orta | Yangın Çantası (fabrikada temel kit düğmesi, sipariş kartı). |
| `Ikonlar/isaret_uyari.png` | 96 x 96 | orta | Riskli yerin üstündeki uyarı (!) işareti. Alt ucu sivri (binayı gösterir). |
| `Ikonlar/isaret_tamam.png` | 96 x 96 | orta | Hazır olan yerin üstündeki yeşil tik işareti. |
| `Ikonlar/butce.png` | 64 x 64 | orta | Bütçe (para) simgesi, üstteki bütçe kutusunda. |

### Arayüz
| Dosya (`Content/` altında) | Piksel | Pivot | Ne |
|---|---|---|---|
| `Arayuz/tarayici.png` | 168 x 300 | orta | Tarayıcı cihazı (sol altta, Mert'in yanında; basınca tahta taranır). Antenler dahil. |
| `Arayuz/canta.png` | 168 x 176 | orta | Fabrikada bantta inen sırt çantası (boş hâli). Kit seçilince kod rengini değiştirir: açık/nötr renk yap. |
| `Arayuz/logo.png` | 1024 x 256 | orta | DisasterBoard logosu (takvim menüsünde başlık). Şeffaf arka plan. |

## Onay akışı (Yiğit)

Gelen her dosya `python Tools/varlik_kontrol.py <dosya>` ile kontrol edilir; sorun yoksa `Art/Onayli/` altına
geçer, varsa nedeni yazılır. `--durum` kalanları listeler, hepsi bitince `--teslim` repoya gönderir. Ayrıntı:
`.claude/skills/varlik-onay/SKILL.md` (Yiğit dosyayı Claude'a atar, Claude bu adımları uygular).

## Şablonları yeniden üretmek (ölçü değişirse)

Ölçülerin tek kaynağı `Tools/sablon_listesi.py`. Değiştir, sonra repo kökünde:

```
"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b --python C:\Dev\DisasterBoard\Tools\sablon_3d_blender.py -- C:\Dev\DisasterBoard\Docs\Sablonlar\3D
python Tools/sablon_2d.py Docs/Sablonlar
python Tools/sablon_belge.py Docs/Sablonlar
```
