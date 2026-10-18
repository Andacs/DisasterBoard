# Tetkik

Karton bir masa oyunu kasabasında tarayıcıyla afet risklerini (deprem, sel, yangın, tsunami) bulursun. Evlere ihtiyacına göre afet çantası hazırlar, şehir önlemleriyle riskleri giderir, kasabalılarla konuşursun. Bütün işaretler yeşil olunca kasaba afetlere hazırdır. Tema: doğal afetlere hazırlık ve farkındalık.

CIVIL GAME JAM 2026 · **FreakLabs**

---

## Çalıştırma (kitapçık EK A.3)

**Platform ve gereksinimler:** Windows 10/11, 64 bit, DirectX 12 destekli ekran kartı. Ek yazılım
gerekmez (gerekirse Unreal Engine önkoşulları paketin içindeki `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe`).

**Başlatma:** ZIP'i bir klasöre açın, `DisasterBoard.exe`'yi çalıştırın (oyunun adı Tetkik; Unreal projesinin iç adı DisasterBoard kaldı). [Açılış videosu / menü varsa: nasıl geçilir.]

**Açılış:** Oyun 4 sahnelik kısa bir hikâyeyle açılır (tıkla: sonraki sahne, "Geç": atla), sonra duvardaki takvim menüsü gelir: **Oyna**.

**Oynanış:** Amaç kasabadaki 8 afet riskini gidermek. Rehberler (Mert, Asil, Yiğit) ilk adımları gösterir: tarayıcıya bas, riskli yerlerde (!) çıkar. Çanta isteyen evler için sağdaki **Çanta Fabrikası**'nda bant akarken doğru temel kiti ve kişiye özel eşyaları seç; paket alttaki çantaya düşer ve teslim edilir (+30 TL). Şehir risklerine alttaki **şehir önlemlerinden** birini seçip riskli yere tıkla (bedeli bütçeden düşer; rögar kapağı ve İngiliz anahtarı kısa birer mini oyun). Kasabalılara tıklayıp dertlerini dinlemek Şehir Farkındalık yüzdesini artırır. Bütün işaretler yeşil olunca kasaba afetlere hazırdır: sonuçta yapılanların fotoğraf albümü ve başarımlar gelir. Süre ve kaybetme yok; yanlış denemeler sayılır ve sonuçta gösterilir.

| Girdi | Ne yapar |
|---|---|
| Sol tık | Seç / uygula / konuş; fabrikada alttaki çantaya basılı tut: bant hızlanır; mini oyunda basılı tut |
| Sağ tık | Geri: seçimi bırak, açık konuşmayı, masayı ya da mini oyunu kapat |
| Esc | Geri; açık bir şey yoksa takvim açılır, oyun duraklar (takvimde "Oyuna devam et"; Çıkış → Menüye dön / Kapat) |
| Q / E ya da alttaki oklar | Tahtayı döndür |
| Fare tekerleği | Yakınlaştır / uzaklaştır |
| Boşluk | Mini oyunda basılı tut |
| M | Müzik aç / kapa |
| F5 | Baştan başla |

**Bilinen sorunlar:** Çalıştırılan dosyanın adı `DisasterBoard.exe` (Unreal projesinin iç adı; oyunun adı Tetkik). Başka bilinen sorun yok.

## Kaynaktan derleme

Unreal Engine **5.8.0**. Editör kapalıyken `Build\derle.bat`, paket için `Build\paketle.bat`
(Shipping, çıktı `Build\Package\Windows`). Trailer: `bash Tools/trailer_yap.sh`. Teslim kontrolü:
`bash Tools/teslim_hazirla.sh`.

## Hazır kaynak beyanı (kitapçık 3.4, EK A.4)

Her satır: ad · üretici/sağlayıcı · bağlantı · lisans (gerçek koşulları eşleşiyorsa adıyla) · oyundaki yeri
· değişiklik. Kullanılmadıysa "Kullanılmadı". Beş satır da dolu olmalı; `teslim_hazirla.sh` bunu kontrol eder.

| Kategori | Durum | Kaynak · sağlayıcı · lisans · kullanım |
|---|---|---|
| Görsel | Kullanıldı | Unreal Engine temel şekilleri (`/Engine/BasicShapes`) ve motor malzemeleri (`BasicShapeMaterial`, `Widget3DPassThrough`) · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · bütün sahne bunlardan koddan kuruluyor, renkleri kodla değişiyor. 2D çizimler (karakterler, ikonlar, duvardaki ekip logosu, açılıştaki 4 hikâye sahnesi, masadaki tavşan kulaklı çıkartma) ve 3D bina/figür/nesne modelleri (aşağıdaki hazır modeller dışında) ekibin kendi işi (`Art/Kaynak/`, `Art/Onayli/`). Hazır 3D modeller: **Low Poly Tree Pack** · louieoliva, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-tree-pack-ea6e844754da494a9c38501b4fff92ad) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · boş arsalardaki ağaçlar (`SM_Agac`); değişiklik: paketten tek ağaç seçildi, şablon ölçüsüne küçültüldü. **Low Poly Car Set** · BetelgeuseMan, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-car-set-9b8a51478e404a3594e11584b90a0e91) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · yollardaki arabalar (`SM_Araba`, setteki `Car01_2`); değişiklik: tek araba seçildi, burnu +X'e döndürüldü, küçültüldü, setin palet dokuları malzemelere bağlandı. **Low Poly Park** · Das Toasty, Sketchfab · [bağlantı](https://sketchfab.com/3d-models/low-poly-park-a69b1965cc7d4b1aa53dfa5d4b389ae3) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · park arsası (`SM_Bina_Park`); değişiklik: ortalandı, arsa ölçüsüne küçültüldü. **Low Poly Building** · A.S.D. ([arvindsudarshan53](https://sketchfab.com/arvindsudarshan53)), Sketchfab · [model sayfası](https://sketchfab.com/3d-models/low-poly-building-f0722265cf8d4a41ba78ab1587d18ac8) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · tahtadaki kasaba binası modeli; değişiklik: ölçek, pivot ve yön oyunun bina şablonuna göre ayarlandı, yüzlere oyunun paletinden renk verildi. Hazır modellerin yazarları oyunda Emeği Geçenler sayfasında da anılıyor. Yan duvardaki **CIVIL GAME JAM 2026 afişi** · etkinlik organizasyonu (KTMMOB İMO) · [game.ktimo.com.tr](https://game.ktimo.com.tr/) · lisans belirtilmemiş; organizasyondan kullanım izni alındı · `Content/Arayuz/duvar_afis.png`; değişiklik: çerçeveye sığsın diye iki yandan kırpıldı. |
| Ses | Kullanılmadı | Hazır ses ya da müzik yok. Müzik ve ses efektleri oyun içinde koddan üretiliyor (`Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp`); bkz. yapay zekâ beyanı. |
| Kod | Kullanıldı | **Unreal Engine 5.8** · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oyun motoru. Derleme, paketleme, trailer ve teslim betikleri (`Build/`, `Tools/`) takımın jam öncesi hazırladığı genel araçlardır (madde 3.1); oyuna girmez. |
| Metin | Kullanıldı | **Claude (Anthropic)** · oyun içi konuşmaların, sorun notlarının ve sonuç ekranındaki hazırlık bilgilerinin ilk taslağı · `Source/DisasterBoard/DBData.cpp` |
| Diğer | Kullanıldı | Yazı tipi **Baloo 2** · Ek Type · [Google Fonts](https://fonts.google.com/specimen/Baloo+2) ([kaynak](https://github.com/google/fonts/tree/main/ofl/baloo2)) · SIL Open Font License 1.1 (lisans metni `Content/Fonts/Arayuz_OFL.txt`) · bütün arayüz ve takvim yazıları. Değişiklik: değişken fonttan fonttools ile sabit Medium (`Arayuz.ttf`) ve Bold (`Arayuz_Kalin.ttf`) kalınlıkları üretildi; harflere dokunulmadı. |

Repoda yeniden dağıtımı kısıtlı ham asset yoktur.

## Yapay zekâ beyanı (kitapçık 4.2, EK A.5)

Her satır: araç · amaç · oyundaki yeri. Çok küçük kullanım da yazılır. Hazır kaynaktan ayrı.

| Kategori | Durum | Araç · amaç · oyundaki yeri |
|---|---|---|
| Görsel | Kullanıldı | **Codex (OpenAI), görüntü üretimi** · ekibin kendi çizimleri stil referansı verilerek duman maskesi ikonu, 5 şehir önlemi ikonu ve 12 kasabalı portresi üretildi (18 Ekim: Elif'in yerine Aysin, `NPC_Aysin.png`, aynı yolla); beyaz zemin `Tools/zemin_temizle.py` ile silindi, ölçüye `Tools/varlik_kontrol.py --ikon` ile oturtuldu · `Content/Ikonlar/esya_duman_maskesi.png`, `Content/Ikonlar/onlem_*.png`, `Content/Karakterler/NPC_*.png` (ham hâlleri `Art/AI_Ham/`). Aynı yolla arayüz parçaları (panel, düğme, konuşma balonu ve not kağıdı çerçeveleri, çekmece kulpu, döndürme okları, başarım rozetleri, maskeleme bandı, kraft kağıt dokusu) üretildi · `Content/Arayuz/ui_*.png` (ham hâlleri `Art/AI_UI/`). Rehber karakterler Asil ile Yiğit'in pozları da önce bu yolla üretilmişti; 18 Ekim'de Asil'in kendi çizimleriyle değiştirildi, Codex sürümleri artık oyunda yok. **Claude (Anthropic)** · yan duvardaki jam afişi ve ekip logosunun yer tutucularını şekillerle çizen Python betiği (`Tools/duvar_gorsel.py`; artık yerlerinde resmi afiş ve ekip logosu var, Claude'un yaptığı yalnız afişin çerçeveye kırpılması). Ekibin 3D modellerinin (bütün evler, okul, apartman, öğrenci evi, sığınak, 12 kasabalı ve tahliye figürü, yangın musluğu, rögar kapağı, takvim, tahta) boyanması ve şablona oturtulması: yüzlere oyunun kasaba paletinden düz renk atayan, renk varyantları üreten, ölçekleyen, tabanı Z = 0'a ve ön yüzü −Y'ye çeviren Blender Python betikleri; hazır modellerin (ağaç, araba, park) ölçek, birim ve döndürmesi ile araba setinin palet dokularının bağlanması da aynı betiklerle. Modelleme ekibin ve hazır modellerin yazarlarının; görüntü üretici model yok · `Content/DisasterBoard/Meshes/` (`SM_*` ve `T_*_Palet` dokuları), `Art/Onayli/3D/`. Mert'in karakteri, kit ve eşya ikonlarının ilk 13'ü, uyarı/tamam işaretleri ve bütçe simgesi ekibin kendi çizimi; yara bandı, el feneri, düdük, gözlük, bebek maması, bebek bezi ikonları ile tarayıcı ve çanta da 18 Ekim'de Asil'in çizimiyle değiştirildi (Codex sürümleri artık oyunda yok). Açılıştaki 4 hikâye sahnesi Asil'in çizimi; Claude yalnız 16:9 kadraja kırptı (Python betiği) · `Content/Cizgiroman/sahne1..4.png`. Açılış logosu da Asil'in 2. sahnesindeki "TETKİK!" kutusu; Claude yalnız kutuyu kesip zeminini saydam yaptı (Python betiği, `Tools/trailer_kurgu.py` `make_logo`) · `Content/Arayuz/logo_kutu.png`, kaynak `Art/Kaynak/Cizgiroman/sahne2_logo.jpg`. Asil ile Yiğit'in rehber pozları (`Content/Karakterler/{B,C}_*.png`) ekibin çizimi; Claude yalnız `varlik_kontrol.py --karakter` ile 320×512'ye oturttu. Masadaki çıkartma ekibin çizimi; Claude yalnız siyah zeminini silip beyaz kenar ve gölge ekledi (Python betiği) · `Content/Arayuz/masa_sticker.png`, kaynak `Art/Kaynak/Arayuz/masa_sticker_kaynak.jpg`. |
| Ses | Kullanıldı | **Claude (Anthropic)** · müziği ve ses efektlerini (tık, rüzgar, sayfa, tahta, su, doğru/yanlış, karakter konuşma heceleri, korna, tarayıcı, bant, çanta fermuarı, başarım, deklanşör) oyun içinde üreten kodun yazımı; ses dosyası yok · `Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp` |
| Kod | Kullanıldı | **Claude (Anthropic, Claude Code)** · oyun kodunun yazımı (C++: akış, arayüz, sahne kurulumu, sis/kum efektleri, koddan müzik üreteci, otomatik test) · `Source/DisasterBoard/`. Ayrıca tanıtım videosu çekim komutu `DBSinema` (oyunu `DBTest` gibi kendi kendine oynatır, başlık ve altyazı çizer, her kareyi kaydeder, oyunun koddan müziğini dosyaya yazar; yalnız çekimde açık, oynanışı değiştirmez) ve video betikleri: ham çekim (`-DBSinemaHam`: yazısız, yavaş kamera hareketli) ve kurgu betiği `Tools/trailer_kurgu.py` (ham kareler + ekibin çizgi romanı ve kutu logosu → büyük hareketli yazılar, geçişler, yakınlaşmalar; whoosh / vuruş sesleri koddan) · `Source/DisasterBoard/DBPlayerController.cpp`, `DBHUD.cpp`, `Tools/trailer_yap.sh`, `Tools/trailer_kurgu.py` |
| Metin | Kullanıldı | **Claude (Anthropic, Claude Code)** · ekibin yönlendirmesiyle oyundaki metinlerin yazımı: rehberlerin ve kasabalıların konuşmaları, ipuçları, sonuçtaki "neden önemli" bilgileri, hikâye altyazıları, başarım adları · `Source/DisasterBoard/DBData.cpp`, `DBGameMode.cpp` |
| Diğer | Kullanılmadı | Başka yapay zekâ aracı kullanılmadı. |

## Hak sahipliği

Oyun ve özgün içerik üzerindeki haklar ekip üyelerindedir. Kitapçık madde 9 gereği organizasyona ticari
olmayan, münhasır olmayan kullanım izni verilir.
