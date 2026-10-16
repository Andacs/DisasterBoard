# Unreal tuzakları — belirti → sebep

Hepsi SonZarf'ta gerçekten oldu ve çözülmesi saatler ya da günler aldı. **Bir şey tuhaf davrandığında
önce buraya bak**; listedeki belirtilerden biriyse cevabı burada.

Sıralama kabaca "jam'de başına gelme ihtimaline" göre.

---

## 1. Donmalar ve çökmeler

### Oyun saniyelerce donuyor, sebep yok gibi

**İki ayrı sebebi oldu ve ikisi de bu listede:**

**a) UI metninde fontta olmayan bir glif.** Kilit emojisi (`🔒`) yüzünden Slate son çare fontuna
düşüp **oyun thread'inde senkron font yükledi: 41 saniye.** Prompt oyuncu nesnenin yanındayken
saniyede on kez sorulduğu için donma "nesnenin yanında durmaktan" geliyordu.

> **Kural: UI metinlerinde emoji yok.** Yıldız, tik, kilit — hiçbiri. İkon gerekiyorsa doku kullan.
> PT Serif'te `★`/`☆` de yok; Slate tofu kutusu çiziyor.

**b) Görünür bir component'in ölçeği tam sıfır oldu.** Ters çevrilemez matris → `NaN` →
`ensure` dosyalamak **1,27 saniye.** SonZarf'ta yere düşen eşya toplanırken `BaseScale * (1 - Alpha)`
son karede sıfırlanıyordu.

> **Kural: bir şeyi sıfıra ölçekleme, gizle.** %1'de durdur veya `SetVisibility(false)`.

### Editör bir paketi yeniden adlandırırken çöküyor

Blueprint içeren bir paketi yeniden adlandırmak UE 5.8'de `Kismet2.cpp:834` assertion'ında motoru
çökertiyor. SonZarf'ta bir kez **34 dosya yedi.**

> **Kural: satın alınan/indirilen paket klasörleri yeniden adlandırılmaz, taşınmaz.**

### Instanced mesh'ler gri, renk/doku hiç uygulanmıyor

Sallantı'da (14 Ekim) yeşil karolar `UInstancedStaticMeshComponent` ile çizildi ve ekranda **nötr gri**
çıktı; `Tint` parametresi işe yaramıyor gibiydi. Logda tek satır vardı:
`Material M_Cardboard missing usage flag InstancedStaticMeshes! Default Material will be used in game.`

Malzemenin "Used with Instanced Static Meshes" bayrağı kapalıysa ISM, HISM ve foliage **sessizce motorun
varsayılan malzemesine** düşer. Önceki koyu ızgara çizgileri de aynı sebeple griydi ve koyu oldukları için
fark edilmedi.

> **Çözüm:** malzemede `used_with_instanced_static_meshes = True`, kaydet (Python: `set_editor_property`
> + `save_asset(..., only_if_is_dirty=False)`, sonucu kontrol et). Aynı aile: Niagara, skeletal mesh,
> spline mesh bayrakları.

> **Kural: "renk tutmuyor" dendiğinde önce piksel ölç, sonra logda `usage flag` ara.** Tint değerini
> tahminle değiştirmek üç tur yer, sorunu çözmez.

### Paketleme cook'ta bitmiyor, log susuyor

Sallantı Gün 0'ında (14 Ekim) oldu. Cook bütün paketleri bitirdi, son satır
`Exiting ShaderCompilingThread` oldu ve süreç **2,5 saat** orada asılı kaldı. Hata yok, çıkış yok.
Logdaki tek iz başta, `Error` olarak ama "Handled ensure" diye geçiyor:
`Ensure condition failed: GetShadowIndex() == 0` → `XGEControllerModule` → named pipe.

Bu makinede XGE yok; XGE denetleyicisi yine de shader derlemesine bağlanmaya çalışıyor.

> **Çözüm:** `.uproject`'te `{"Name": "XGEController", "Enabled": false}` ve `DefaultEngine.ini`'de
> `[ConsoleVariables] r.XGEShaderCompile=0`. Editörü yeniden derle. Sonrasında paket 117 sn'de
> tamamlandı.

> **Kural: paket logu 10 dakika yazılmıyorsa asılmıştır.** Beklemek bir şey çözmüyor; süreci
> öldür ve logun **başındaki** ensure'lara bak, sonundakilere değil.

### `git checkout` yarım uygulandı

Açık editör `.uasset`/`.umap` kilidi tutuyor. Kaynak dosyalar silindi, binary'ler silinmedi.

> **Kural: pull/checkout/branch değiştirmeden önce editör kapanır.** Olduysa:
> `git reset --mixed` + `git checkout -- .`

---

## 2. Ölçüm ve yerleştirme

Bu bölümün tamamı tek bir derse indirgenebilir: **ölç, tahmin etme — ve neyle ölçtüğüne dikkat et.**

### Yerleştirilen şey havada kalıyor veya gömülü

Zemin yüksekliğini yanlış kaynaktan almak. SonZarf'ta **üç kez** yanlış yapıldı:

1. Kesilebilir ağacın bounds tabanı alındı → ağaç 40 cm toprağa gömülü olduğu için herkes gömüldü
2. "Ayakta duran insan daha iyi referans" diye satıcı alındı → herkes 15 cm havada kaldı, çünkü
   **satıcının kendisi 15 cm havadaydı**
3. `PlayerStart`'ın Z'si alındı → o kapsül **merkezi**, karakteri 90 cm havaya attı

Doğru cevap zeminin **geometrisi**: zemin slabının üstü, ikinci bir kaynakla (üstünde duran çit)
doğrulanmış.

> **Kural: zemini zeminden al.** Yerleştirilecek şeyin *cinsinden* bir aktörü referans alma —
> yerleşimi sorgulanan şey odur. İki kaynakla doğrula, ayrışırlarsa sus ve söyle.

**Not:** aşağı doğru trace de çare değil — commandlet'in editör dünyasında fizik sahnesi yok, trace
boş döner.

### Ölçüm doğru çıkıyor ama ekranda yanlış duruyor

**Animasyonlu bir karakteri ref-pozdan ölçmek.** Editör/commandlet animasyonu değerlendirmiyor;
`get_actor_bounds` ref-pozu anlatır, ekran klibi gösterir.

> SonZarf'ta dört karakter de "tam 176 cm" ölçülürken ekranda 176/191/202 duruyordu ve yerleştirme
> script'i "kalan sapma 0.0 cm" diye rapor ediyordu — diz boyu gömülü karakterler hakkında.

> **Kural: klip ölçülecekse klip değerlendirilir** (kemik zincirini kompoze et). Ref-poza göre
> hesaplanmış bir düzeltmeyi çalışan poza uygulamak sapmayı gidermez, **ekler**.

### Aynı şeyin iki ölçümü uyuşmuyor

**Farklı yöntemle ölçmek.** Bir karakteri kemikten, ötekini bounding box'tan ölçüp oran çıkarmak
anlamsız bir sayı verir.

> **Kural: karşılaştırılacak iki şey aynı yöntemle ölçülür.**

### Yürüme klibinde ayak yüksekliği saçma

Tek karede ölçmek. `t=0`'da bir ayak zaten havadadır; kusursuz bir yürüyüş "12 cm havada" diye
raporlandı.

> **Kural: döngü boyunca iki ayağın en alçak noktasına bak.**

---

## 3. Animasyon ve karakter

### Karakter şişik/bozuk duruyor, hiçbir ölçek düzeltmiyor

**Mesh ve klipler farklı rig turlarından geliyor.** Fırıncının mesh'i eski Mixamo turundan, klipleri
yeniden: kemik oranları gövdede 0.75, kolda 0.53, bacakta 0.51. Toplam boyu 176'ya ölçeklemek
karakteri düzeltmiyor, **şişik hâlde 176 yapıyor.**

> **Kural: mesh ve klipler aynı turdan iner.**

### Karakter yerden kopuk

**Animasyonlu karakterin yeri mesh'ten değil klipten gelir.** Mesh'e uygulanan kaldırmayı kliplere
de uygulamak karakteri 86 cm havaya çıkardı.

> Mesh'in ref-pozu bounds için; klibin kendi track'leri ekrandaki yer ve boy için.

### Karakterin dokusu kayboldu

**Mixamo dokuyu öldürüyor** — 2014 FBX SDK, çoklu dokuda rastgele seçim.

> **Kural: doku Mixamo turundan geçmesin.** Rig'i al, dokuyu ayrı bağla.

### İki NPC aynı rig'e sahip ama animasyon oynatmıyor

Kendi `Skeleton` asset'lerine bağlı import edilmişler. Aynı iskeleti paylaşmaları gerekiyor.

> **Kural: aynı rig'den gelen her karakter tek bir Skeleton asset'ine bağlanır.**

### Önizlemede tuhaf görünen şey

**Genelde gerçektir.** Mixamo'da karakter grid'e gömülü görünüyordu; ölçünce mesh'in pivotu gerçekten
belde çıktı ve **16 teslimatın hepsinde** vardı. Kamera sanılıp geçilseydi 15 yarı gömük karakter
yerleştirilecekti.

---

## 4. Ses

### `FadeIn` çağırdım, hiç ses gelmiyor

`SpawnSound2D(..., VolumeMultiplier=0.f, ...)` sonra `FadeIn()` **hiç başlamıyor** — sessiz spawn
edilen ses fade devreye girmeden cull ediliyor.

> **Çözüm:** `UAudioComponent`'i doğrudan kur (`NewObject` + `RegisterComponentWithWorld`) ve
> `FadeIn()` ilk gerçek `Play()` olsun.

### Tek tıkta iki ses çalıyor, gecikme gibi duruyor

Aynı olaya iki ses bindirmek (buton sesi + panel kapanma sesi) iki kasıt gibi değil **gecikme/bulanıklık**
gibi okunuyor.

> **Kural: bir olay, bir ses.**

### Üç ortam sesi birlikte gürültü duvarı

Hepsi 1.0 ses seviyesinde başlatılmış. "Yağmur gibi" diye bildirildi.

> **Kural: bir katman taşır (0.6), ötekiler doku olarak altta durur (0.18 / 0.06).**

---

## 5. UI / Slate

### Butonun dolgusu hiç görünmüyor

`ContentPadding` **arka planı da itiyor.** Dolgu yazının kendi slot'una konulmalı.

### Panelde yazılar üst üste biniyor

`FillHeight` sıfıra sıkışınca içerik kendi slot'unun dışına taşıyor.

> **Çözüm:** sabit yükseklik + `SScrollBox`.

### Butonlar panelden kayboldu

İçerik büyüyünce panel dışına itilip `ClipToBounds` tarafından kırpılıyor.

### Her şey Roboto'da çiziliyor, font'u import ettim

`.ttf` import'u **`UFontFace`** üretir, `FSlateFontInfo`'nun istediği **`UFont`** değil. Sarmalayan
`UFont` (`<Ad>_Font`) ancak import factory'nin `BatchCreateFontAsset` bayrağı açıkken oluşuyor;
yoksa `FObjectFinder<UFont>` sessizce başarısız olup motor varsayılanına düşüyor.

**Ayrıca:** font factory headless commandlet'te çalışmıyor (`SlateApplication` yok), normal editörde
`-ExecutePythonScript` ile çağrılmalı.

**Sallantı'da işe yarayan yol (15 Ekim): font'u asset yapma, dosyadan oku.** Aktarıcı başsız komutta
onay penceresi açmaya çalışıp `Assertion failed: CurrentApplication.IsValid()` ile çöktü. Bunun yerine
`.ttf` → `Content/Fonts/`, `DefaultGame.ini`'de `+DirectoriesToAlwaysStageAsNonUFS=(Path="Fonts")`, HUD
açılışta `UFont` kurar: `FontCacheType=Runtime`, `FTypefaceEntry(TEXT("Regular"), Yol,
EFontHinting::Default, EFontLoadingPolicy::LazyLoad)`. `Build.cs`'e **`SlateCore`** (yoksa `FFontData`
LNK2019). Dosya bulunamazsa Roboto'ya düşer, logda yazdır. Font'ların punto karakteri farklı: Jost
Roboto yanında küçük kaldı, ×1,18 büyütüldü.

### `ToUpper` Türkçe harfleri atlıyor

`FString::ToUpper` `i`'yi `İ`'ye, `ş/ğ/ı/ö/ü/ç`'yi büyütmüyor; başlıklarda "SALLANTi" çıkıyor. Kendi
eşleme tablonla büyüt (Sallantı'da `TrUpper`).

---

## 6. Kamera ve post-process

### Sürekli açık post-process efekti hiç görünmüyor

`APlayerCameraManager::AddCachedPPBlend` yalnızca aktif bir `SetViewTargetWithBlend` geçişi
sırasında tüketiliyor — genel "her zaman açık" kanca olarak çalışmıyor. Şüphe tabanlı bir
vinyet/desatürasyon efekti bu yüzden cutscene dışında hiç görünmedi.

> **Çözüm:** `UCameraModifier` türet, `ModifyPostProcess` override et.

### Grade çalışıyor ama kimse fark etmiyor

İki ayrı tuzak:

- **Çok yavaş değişen görsel fark edilmiyor** (değişim körlüğü). Doğrudan bakılıyorken bile.
  Geçişi 1–2 saniyede bitir, dakikalarca sürüklemeyin.
- **Küresel renk filtresi tek başına "oyunun görünüşü" sanılıyor**, oyuncunun yaptığı bir şey değil.
  Yanına yerel bir değişim koy (bir nesnenin durumu, bir ışık).

### Sürekli açık karanlık grade reddedildi

Playtest geri bildirimi: gündelik hayatın sıcak okunması gerekiyorsa sürekli karanlık grade onunla
çatışıyor.

> **Kural: sıcak/normal taban, kötü durum sapma.** Sapmanın gerekçesi oyuncunun yaptığı bir şey olsun.

---

### Pastel renkler ekranda kirli ve karanlık

Otomatik pozlama büyük açık renkli arka plana bakıp sahneyi karartıyor: Sallantı'da lavanta (0,55 0,45
0,78) ekranda (43,36,57) çıktı, ~20 kat. Kamerada **sabit pozlama** (`AutoExposureMethod=Manual`,
Sallantı'da EV100 1,9). Unlit malzemeler lit olanlarla eşleşsin diye parlaklık çarpanı ister. Tint'i
tahminle değiştirme, pikseli ölç.

---

## 7. Python / editör otomasyonu

Jam'de script yazacaksan bunlar; yazmayacaksan atla.

### Commandlet'te seçim yok

`set_selected_level_actors` headless'ta seçimi **boş bırakıyor**, dolayısıyla seçime dayanan her
işlem sessiz no-op. `move_selected_actors_to_level` 0 döndürüyor, hata vermiyor.

### Aktörler seviyeler arası kopyalanamıyor

25 Ağustos'ta üç yol ölçüldü, üçü de kapandı:

| Yol | Sonuç |
|---|---|
| İkinci seviyeyi streaming ekle + `duplicate_actor` | **Editör çöküyor** (access violation) |
| `move_actors_to_level` | 0 döndürüyor, hiçbir şey yapmıyor |
| Seçim + `move_selected_actors_to_level` | Seçim boş, 0 döndürüyor |

> **Çözüm:** interaktif editörde World Outliner'dan sınıfa göre filtrele, Ctrl+C / Ctrl+V. Yapıştırma
> dünya koordinatlarını ve bütün özellikleri koruyor. Spawn headless çalışıyor, sorun yalnızca
> kopyalamada.

### `get_world()` None dönüyor

`LevelEditorSubsystem.get_world()` commandlet altında **None.** `UnrealEditorSubsystem.get_editor_world()`
kullan. None'ı `add_level_to_world`'e vermek "Ensure condition failed: InWorld" diye motor
callstack'i veriyor ve dünyanın sorun olduğunu söylemiyor.

### `set_current_level_by_name` çalışmıyor

Haritanın **kısa adını** istiyor (`map_village_day21`), kalıcı seviyenin `get_name()`'ini değil — o
`PersistentLevel` döndürür ve reddedilir. Bool dönüyor; güvenme, `get_current_level()`'ı nesneyle
karşılaştır.

### Python'da struct dizisini değiştirmek işe yaramıyor

`for` ile dolaşınca eleman **kopya** gelir. Üstünde değişiklik yapıp diziyi geri yazmak hiçbir şeyi
değiştirmez.

> **Çözüm:** indeksle geri yaz, kaydetmeden önce yeniden oku.

### C++ sınıfının CDO'suna Python'dan yazdığım değer kalmıyor

**Native C++ sınıfının CDO'suna yapılan property değişikliği kaydedilmiyor** — yalnızca Blueprint
CDO'su kalıcı.

> **Çözüm:** asset referanslarını sınıfın kendi constructor'ında `ConstructorHelpers::FObjectFinder`
> ile bağla.

### Blueprint aktöründe component değişikliği diske yazılmıyor

Örnek seviyesindeki component değişiklikleri kalıcı değil.

### Kontrol her şeye "bozuk" diyor

**Bozuk olan kontroldür.** `MaterialEditingLibrary.get_used_textures` UE 5.8'de deprecated ve
commandlet altında **her** materyal için boş liste döndürüyor — motorun kendi Mannequin'i dahil.

> **Kural: raporlamadan önce bilinen-sağlam bir örnekte dene.**

### Kayıt "başarılı" raporluyor ama dosya değişmemiş

`save_current_level()` ve `save_asset()` **sonuç döndürüyor.** SonZarf'ta bir script koşulsuz
"seviye kaydedildi" bastı ve bir "Error moving file" hatasını bir oturum boyunca sakladı.

Ayrıca `EditorAssetLibrary.save_asset` varsayılan olarak `only_if_is_dirty=True` ve dirty bayrağı
set edilmemişse sessizce hiçbir şey yapmıyor.

> **Kural: sonucu kontrol et, başarısızlıkta `raise`. Önemliyse `only_if_is_dirty=False` geç.**

### Commandlet 255 ile çıkıyor ama iş olmuş

**Commandlet başarıda da 255 döner.** Logdan karar ver, çıkış kodundan değil.

---

## 8. PowerShell (Türkçe Windows)

### Üretilen isimden harf kayboldu

`-replace` / `-match` varsayılan olarak büyük-küçük harf duyarsız ve katlamayı **kültüre göre**
yapıyor. Bu makine `tr-TR`: büyük `I` noktasız `ı`'ya katlanıyor, `[A-Za-z]`'ın dışında kalıyor ve
`[^A-Za-z0-9]` onu yakalayıp siliyor.

**`Idle` → `dle`.** Harf sessizce kayboluyor ve sonuç bir hata gibi değil **bir isimlendirme tercihi
gibi** duruyor.

> **Kural: isim üreten her yerde `-creplace` / `-cmatch` kullan**, harf varyantlarını desenin içine
> yaz. Bu sınıf hata SonZarf'ta bir script'i üç kez yazdırdı.

---

## 9. Paket ve teslim (Sallantı provası, 15 Ekim)

### Repo public olacak ama `DefaultEngine.ini`'de `SecurityToken` var

UE şablonu **AndroidFileServer** eklentisini açık getiriyor; editör her açılışta
`[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]` altına rastgele bir
`SecurityToken` yazıyor. Satırı silmek yetmedi: editör bir sonraki açılışta yenisini yazdı.

> **Çözüm:** `.uproject` → `{"Name": "AndroidFileServer", "Enabled": false}`, satırı sil, `derle.bat`.
> Kural 3.7 public repoda erişim anahtarı istemiyor; `teslim_hazirla.sh` bunu tarar.

### "ZIP" Windows'ta açılmıyor / paketten büyük

Git Bash'in `tar`'ı GNU tar: `tar -a -c -f x.zip *` **adı .zip olan bir tar arşivi** yazıyor, hata
vermiyor. Sallantı'nın ilk teslim ZIP'i (682 MB, paketten büyük) böyleydi; Gezgin açamaz, jüri oyunu
çalıştıramazdı. Fark eden: boyutun paketten büyük olması ve `file x.zip` → "POSIX tar archive".

> **Kural: ZIP'i Windows'un tar'ıyla yaz** (`/c/Windows/System32/tar.exe -a -c -f x.zip *`; 650 MB
> paket → 348 MB) **ve ilk iki baytın `PK` olduğunu kontrol et.** `teslim_hazirla.sh` ikisini de yapıyor.

### Paketi açtım, süreç var ama pencere yok gibi

`<Proje>.exe` yalnızca başlatıcı; pencereli süreç `<Proje>-Win64-Shipping.exe`. Ekran görüntüsü ve
`taskkill` o isimle.

### Deneme klasörü silinmiyor

Oyun süreci kapanmadan klasör kilitli. `taskkill` sonrası birkaç saniye bekle, sonra sil.

### Açılış videosu pakette oynamıyor

`Content/Movies/<Ad>.mp4` + `DefaultGame.ini` → `[/Script/MoviePlayer.MoviePlayerSettings]`
`+StartupMovies=<Ad>` ve **`+DirectoriesToAlwaysStageAsNonUFS=(Path="Movies")`** (yoksa pakete girmiyor).
`bMoviesAreSkippable=True` ver; jüri beklemek istemez. `.mp4` LFS'e (`.gitattributes`).

### Shipping'de konsol yok, log yok

Davranışı görmenin yolu ekran: `pencere_goruntu.ps1` yalnızca oyun penceresini `PrintWindow` ile alır.
**Masaüstünün görüntüsünü alma**: bir kez kullanıcının Discord'u kayda girdi.

