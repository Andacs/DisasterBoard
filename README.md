# DisasterBoard

Karton bir masa oyunu kasabasında tarayıcıyla binaların deprem risklerini bulursun. Her eve ihtiyacına göre afet çantası hazırlar, şehir risklerini belediye araçlarıyla giderirsin; deprem gelmeden kasabayı hazırlamaya çalışırsın.

CIVIL GAME JAM 2026 · **FreakLabs**

---

## Çalıştırma (kitapçık EK A.3)

**Platform ve gereksinimler:** Windows 10/11, 64 bit, DirectX 12 destekli ekran kartı. Ek yazılım
gerekmez (gerekirse Unreal Engine önkoşulları paketin içindeki `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe`).

**Başlatma:** ZIP'i bir klasöre açın, `DisasterBoard.exe`'yi çalıştırın. [Açılış videosu / menü varsa: nasıl geçilir.]

**Oynanış:** [Amaç, kazanma ve kaybetme. İlk adım ne.]

| Girdi | Ne yapar |
|---|---|
| [Sol tık] | [...] |
| [Esc] | [...] |

**Bilinen sorunlar:** [Yoksa "Bilinen sorun yok." Varsa: sınırlama ve geçici çözüm.]

## Kaynaktan derleme

Unreal Engine **5.8.0**. Editör kapalıyken `Build\derle.bat`, paket için `Build\paketle.bat`
(Shipping, çıktı `Build\Package\Windows`). Trailer: `bash Tools/trailer_yap.sh`. Teslim kontrolü:
`bash Tools/teslim_hazirla.sh`.

## Hazır kaynak beyanı (kitapçık 3.4, EK A.4)

Her satır: ad · üretici/sağlayıcı · bağlantı · lisans (gerçek koşulları eşleşiyorsa adıyla) · oyundaki yeri
· değişiklik. Kullanılmadıysa "Kullanılmadı". Beş satır da dolu olmalı; `teslim_hazirla.sh` bunu kontrol eder.

| Kategori | Durum | Kaynak · sağlayıcı · lisans · kullanım |
|---|---|---|
| Görsel | Kullanıldı | Unreal Engine temel şekilleri (`/Engine/BasicShapes`) ve motor malzemeleri (`BasicShapeMaterial`, `Widget3DPassThrough`) · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · bütün sahne bunlardan koddan kuruluyor, renkleri kodla değişiyor. 2D çizimler (karakterler, ikonlar, duvardaki ekip logosu) ve 3D bina/figür/nesne modelleri ekibin kendi işi (`Art/Kaynak/`, `Art/Onayli/`). Hazır 3D modeller: **Low Poly Tree Pack** · louieoliva, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-tree-pack-ea6e844754da494a9c38501b4fff92ad) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · boş arsalardaki ağaçlar (`SM_Agac`); değişiklik: paketten tek ağaç seçildi, şablon ölçüsüne küçültüldü. **Low Poly Car Set** · BetelgeuseMan, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-car-set-9b8a51478e404a3594e11584b90a0e91) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · yollardaki arabalar (`SM_Araba`, setteki `Car01_2`); değişiklik: tek araba seçildi, burnu +X'e döndürüldü, küçültüldü, setin palet dokuları malzemelere bağlandı. **Low Poly Park** · Das Toasty, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-park-a69b1965cc7d4b1aa53dfa5d4b389ae3) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · park arsası (`SM_Bina_Park`); değişiklik: ortalandı, arsa ölçüsüne küçültüldü. Yan duvardaki **CIVIL GAME JAM 2026 afişi** · etkinlik organizasyonu (KTMMOB İMO) · [game.ktimo.com.tr](https://game.ktimo.com.tr/) · lisans belirtilmemiş, kullanım izni organizasyondan teyit edilmeli · `Content/Arayuz/duvar_afis.png`; değişiklik: çerçeveye sığsın diye iki yandan kırpıldı. |
| Ses | Kullanılmadı | Hazır ses ya da müzik yok. Müzik ve ses efektleri oyun içinde koddan üretiliyor (`Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp`); bkz. yapay zekâ beyanı. |
| Kod | Kullanıldı | **Unreal Engine 5.8** · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oyun motoru. Derleme, paketleme, trailer ve teslim betikleri (`Build/`, `Tools/`) takımın jam öncesi hazırladığı genel araçlardır (madde 3.1); oyuna girmez. |
| Metin | Kullanıldı | **Claude (Anthropic)** · oyun içi konuşmaların, sorun notlarının ve sonuç ekranındaki hazırlık bilgilerinin ilk taslağı · `Source/DisasterBoard/DBData.cpp` |
| Diğer | Kullanıldı | Yazı tipi **Baloo 2** · Ek Type · [Google Fonts](https://fonts.google.com/specimen/Baloo+2) ([kaynak](https://github.com/google/fonts/tree/main/ofl/baloo2)) · SIL Open Font License 1.1 (lisans metni `Content/Fonts/Arayuz_OFL.txt`) · bütün arayüz ve takvim yazıları. Değişiklik: değişken fonttan fonttools ile sabit Medium (`Arayuz.ttf`) ve Bold (`Arayuz_Kalin.ttf`) kalınlıkları üretildi; harflere dokunulmadı. |

Repoda yeniden dağıtımı kısıtlı ham asset yoktur.

## Yapay zekâ beyanı (kitapçık 4.2, EK A.5)

Her satır: araç · amaç · oyundaki yeri. Çok küçük kullanım da yazılır. Hazır kaynaktan ayrı.

| Kategori | Durum | Araç · amaç · oyundaki yeri |
|---|---|---|
| Görsel | Kullanıldı | **Claude (Anthropic)** · yan duvardaki jam afişi ve ekip logosunun yer tutucularını şekillerle çizen Python betiği (`Tools/duvar_gorsel.py`; artık yerlerinde resmi afiş ve ekip logosu var, Claude'un yaptığı yalnız afişin çerçeveye kırpılması). Ekibin 3D modellerinin (bütün evler, okul, apartman, öğrenci evi, sığınak, 12 kasabalı ve tahliye figürü, yangın musluğu, rögar kapağı, takvim, tahta) boyanması ve şablona oturtulması: yüzlere oyunun kasaba paletinden düz renk atayan, renk varyantları üreten, ölçekleyen, tabanı Z = 0'a ve ön yüzü −Y'ye çeviren Blender Python betikleri; hazır modellerin (ağaç, araba, park) ölçek, birim ve döndürmesi ile araba setinin palet dokularının bağlanması da aynı betiklerle. Modelleme ekibin ve hazır modellerin yazarlarının; görüntü üretici model yok · `Content/DisasterBoard/Meshes/` (`SM_*` ve `T_*_Palet` dokuları), `Art/Onayli/3D/` |
| Ses | Kullanıldı | **Claude (Anthropic)** · müziği ve ses efektlerini (tık, rüzgar, sayfa, tahta, su, doğru/yanlış, karakter konuşma heceleri, korna, tarayıcı, bant, çanta fermuarı, başarım, deklanşör) oyun içinde üreten kodun yazımı; ses dosyası yok · `Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp` |
| Kod | Kullanıldı | **Claude (Anthropic, Claude Code)** · oyun kodunun yazımı (C++: akış, arayüz, sahne kurulumu, sis/kum efektleri, koddan müzik üreteci, otomatik test) · `Source/DisasterBoard/` |
| Metin | [...] | [...] |
| Diğer | [...] | [...] |

## Hak sahipliği

Oyun ve özgün içerik üzerindeki haklar ekip üyelerindedir. Kitapçık madde 9 gereği organizasyona ticari
olmayan, münhasır olmayan kullanım izni verilir.
