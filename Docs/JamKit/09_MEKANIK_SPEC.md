# Mekanik spesifikasyonu

Tek programcının (Andaç) uygulayacağı şey. `08_ASSET_SPEC.md` nasıl modelcinin bağlayıcı dosyasıysa
bu da kodun bağlayıcı dosyası.

Bütün sayılar **varsayılan ve gerekçeli**. Hepsi `EditAnywhere` olacak — bu makinede derleme editör
kapalı ve özel bayraklarla yapılıyor, yani koda gömülü bir sayı her denemede bir derleme demek.

---

## 1. Çekirdek döngü

> **Nesne seç → sürükle → yerleştir → denge bozulur → düzelt**

Tek cümle: *Sallanan bir platformun üstüne şehir kuruyorsun; her koyduğun şey dengeyi bozuyor.*

Bir tur **60–120 saniye** (`06_PROJE_BRIEFI.md` → hedef kitle).

---

## 2. Denge sistemi — oyunun kalbi

### Nasıl çalışıyor

Platform fiziksel bir eklem üstünde **değil**. Ağırlık merkezi hesaplanıp platformun dönüşü
**kinematik olarak** sürülüyor.

```
1. Yerleştirilmiş her nesnenin kütlesi ve platform merkezine göre XY ofseti var
2. Ağırlık merkezi:  CoM = Σ(kütle × ofsetXY) / Σ(kütle)
3. Hedef eğim:       Tilt = clamp(CoM × TiltPerCm, -MaxTilt, +MaxTilt)
4. Mevcut eğim hedefe doğru TiltSpeed ile yaklaşır
5. Üstüne sürekli bir "dünya sallanması" eklenir (sinüs)
```

### Neden gerçek fizik eklemi değil

Platformu bir constraint'e bağlayıp gerçekten devirmek daha "doğru" ama jam'de yanlış tercih:
ayarlanması saatler alıyor, patladığında sebebini bulmak saatler daha alıyor, ve tek programcılı
bir ekipte o saat yok. Hesaplanmış eğim **deterministik, ayarlanabilir ve patlamıyor** — oyuncu
aradaki farkı göremez.

### Sürekli sallanma — konseptin "sallanan dünya"sı

Hiçbir şey yerleştirilmemişken bile platform hafifçe sallanıyor:

```
AmbientSway = sin(t × 2π / SwayPeriod) × SwayAmplitude
```

Bu, dengenin **zaten kırılgan** olduğunu oyuncu daha ilk saniyede anlatıyor. Oyuncunun hatası
olmadan da dünya sallanıyor — konsept bu.

> **14 Ekim — ekip kararıyla değişti (Sallantı'da uygulandı):** binalar **sabit**, fiziğe bırakılmaz.
> 8°'de (`WarnTilt`) platform kırmızı atar; 10° (`FailTilt`) 1,5 sn aşılırsa platform binalarla birlikte
> 45°'ye devrilir ve oyun biter. Aşağıdaki A/B seçimi tarihçe olarak duruyor; A uygulandı, ekranda
> görüldü, sonra kaldırıldı. Ölçülen ders: motorun varsayılan sürtünmesiyle (0,7) 8–12°'de hiçbir
> şey kaymıyordu. Ayrıca `TiltPerCm` 0,04'te devrilme pratikte imkânsızdı → 0,08 + `PlatformMass` 12.
> Güncel durum: `C:\Dev\Sallanti\CLAUDE.md`.

### Nesneler eğimde ne yapıyor

İki yol var. **Önerilen birincisi.**

**A) Bağlı → eşikte bırakılıyor** (önerilen)

Yerleştirilen nesne platforma `AttachToComponent` ile bağlı, fizik kapalı. Eğim
`ReleaseTilt`'i geçince **fizik açılıyor** ve nesne gerçekten kayıp düşüyor.

- Titreme yok, istifleme stabil
- "Bir anda her şey bırakıyor" anı dramatik ve okunur
- Uygulaması yarım saat

**B) Sürekli simüle fizik**

Her nesne baştan simüle. Daha emergent ama platform kinematik dönerken üstündeki cisimler
sürtünmeyle takip etmek zorunda; UE'de bu titreme üretiyor ve ayarlamak uzun sürüyor.

> **Jam'de A. B'yi ancak A bitip zaman kalırsa dene.**

### Kaybetme

İki koşul, ikisi de sayılabilir:

1. Eğim `FailTilt`'i **`FailSeconds` boyunca kesintisiz** aşarsa → yapı devrildi
2. Herhangi bir nesne platformun altına düşerse (`Z < FailZ`) → düştü

Süre şartı önemli: anlık bir salınımda kaybetmek haksız hissettiriyor.

### Kazanma

`TargetObjectCount` nesne yerleştirildi **ve** eğim `FailTilt` altında kaldıysa.

> Alternatif: `SurviveSeconds` saniye ayakta kal. İkisinden **birini** seçin, ikisi birden
> oyuncunun kafasını karıştırır.

---

## 3. Sürükle ve yerleştir

### Akış

```
1. Oyuncu slot çubuğundan nesne seçer (tıkla / 1-4 tuşu)
2. İmlecin altında YARI SAYDAM HAYALET mesh belirir
3. Hayalet, imleçten platforma atılan ışınla konumlanır
4. Geçerliyse yeşilimsi, geçersizse kırmızımsı
5. Sol tık → gerçek nesne doğar, platforma bağlanır, kütlesi dengeye katılır
6. R → hayaleti 90° döndürür
7. Esc / sağ tık → iptal
```

### Geçersiz yerleştirme koşulları

- Işın platforma değmiyor (dışarısı)
- Hayaletin kutusu başka bir nesneyle çakışıyor
- Nesnenin tabanı platform kenarından taşıyor

> **Geçersizken yerleştirme engellenir ama ses/uyarı verilmez.** Hayaletin kırmızı olması yeterli;
> 13+ hedef kitlede metin uyarısı gereksiz (`06_PROJE_BRIEFI.md`).

### Izgara

Nesneler **50 cm ızgaraya yapışır** (`08_ASSET_SPEC.md` temel modülü). Serbest yerleştirme değil.

**Neden:** boşluksuz oturan parçalar fizikte stabil; 3 cm'lik boşluk kule titremesi demek. Ayrıca
oyuncunun işini kolaylaştırıyor — denge zaten yeterince zor.

---

## 4. Kamera

Sabit izometrik. Oyuncu döndürmüyor, zoom yapmıyor — jam'de kamera kontrolü hem kod hem hata demek.

| Ayar | Değer | Not |
|---|---|---|
| Projeksiyon | Perspective | Ortho'nun UE'de gölge tuhaflıkları var |
| FOV | 30° | Düşük FOV izometrik hissi verir |
| Yaw | 45° | Klasik izometrik |
| Pitch | −35° | |
| Mesafe | 1400 cm | 600 cm'lik platform ekrana rahat sığar |

Platform eğildiğinde **kamera eğilmez.** Dünya sallanıyor, bakan göz sabit — eğimin okunması buna
bağlı.

---

## 5. Girdi

| Girdi | Eylem |
|---|---|
| Fare hareketi | Hayaleti konumlandır |
| Sol tık | Yerleştir |
| Sağ tık / Esc | İptal |
| `1` `2` `3` `4` | Slot seç |
| `R` | Hayaleti 90° döndür |
| `Space` | (opsiyonel) turu başlat |

Enhanced Input kullanın; UE 5'te varsayılan o ve eski sistem fazladan kurulum istiyor.

---

## 6. Sınıflar

Önek `JM` — proje adı belli olunca değiştirin (SonZarf'ta `SZ` idi).

| Sınıf | Sorumluluk |
|---|---|
| `AJMPlatform` | Eğimi hesaplar ve uygular. Ağırlık merkezinin **tek sahibi** |
| `AJMPlaceable` | Yerleştirilen nesnenin tabanı. Kütle, boyut, bağlanma/bırakılma |
| `UJMBuildComponent` | Oyuncuda. Hayalet, ışın, geçerlilik, yerleştirme |
| `AJMPlayerController` | Girdi, slot seçimi |
| `AJMGameMode` | Kazanma/kaybetme, tur süresi, yeniden başlatma |
| `UJMHUDWidget` | Slot çubuğu, denge göstergesi, sayaç |
| `UJMGameStateSubsystem` | *(gerekirse)* tur durumu — seviye yeniden yüklenmiyorsa gerekmez |

### Tek doğruluk kaynağı

Ağırlık merkezi ve eğim **yalnızca `AJMPlatform`'da** hesaplanır. HUD göstergesi, ses ve kaybetme
kontrolü hepsi oradan **okur**; hiçbiri kendi hesabını yapmaz.

> SonZarf'ta aynı sıralama bilgisi iki yerde tutuldu ve bir commit boyunca sessizce ayrışma riski
> taşıdı. İki yerde hesaplanan bir sayı er ya da geç farklı iki cevap verir.

### Build durumu

Ayrı bir "state system" sınıfına gerek yok. `UJMBuildComponent` içinde bir enum yeter:

```cpp
enum class EJMBuildState : uint8
{
    Idle,        // slot seçilmedi
    Placing,     // hayalet aktif
    Blocked      // hayalet aktif ama geçersiz konumda
};
```

> Notlardaki "State System" maddesi bu. Ayrı bir sistem yazmak 48 saatte kazanç değil maliyet.

---

## 7. Ayarlanabilir değerler

Hepsi `EditAnywhere`. Tek tabloda, çünkü denge ayarı jam'in son saatlerinde yapılacak ve o anda
hangi sayının nerede olduğunu aramak için vakit yok.

### `AJMPlatform`

| Değer | Varsayılan | Ne yapar |
|---|---|---|
| `TiltPerCm` | 0.04 °/cm | Ağırlık merkezi ofseti → eğim çevrimi |
| `MaxTilt` | 12° | Eğimin üst sınırı |
| `TiltSpeed` | 25 °/sn | Hedefe yaklaşma hızı |
| `SwayAmplitude` | 1.5° | Sürekli dünya sallanması |
| `SwayPeriod` | 4.0 sn | Sallanmanın periyodu |
| `ReleaseTilt` | 8° | Nesnelerin fiziğe bırakıldığı eşik |
| `FailTilt` | 10° | Kaybetme eşiği |
| `FailSeconds` | 1.5 sn | Eşiğin kesintisiz aşılması gereken süre |
| `FailZ` | −200 cm | Bu Z'nin altına düşen nesne kayıp sayılır |

### `AJMPlaceable`

| Değer | Varsayılan | Ne yapar |
|---|---|---|
| `Mass` | 1.0 | Dengeye katkı ağırlığı |
| `GridSize` | 50 cm | Izgara adımı (asset spec'iyle aynı) |

### `AJMGameMode`

| Değer | Varsayılan | Ne yapar |
|---|---|---|
| `TargetObjectCount` | 12 | Kazanmak için gereken nesne |
| `RoundSeconds` | 120 | Tur üst sınırı |

> **Bu sayıların hiçbiri doğru değil.** Hepsi başlangıç tahmini; ekranda oynanarak ayarlanacak.
> Önemli olan hepsinin editörden ayarlanabilir olması — `05_CALISMA_KURALLARI.md`: bir değeri iki
> kez tahminle değiştirdiysen dur ve ölç.

---

## 8. Geri bildirim — üç kanal

`05_CALISMA_KURALLARI.md`'nin kuralı: tek kanal fark edilmiyor. Denge durumu **üçünden** okunmalı.

| Kanal | Ne yapıyor |
|---|---|
| **Görsel** | Platformun eğilmesi. Ana kanal, bedava — zaten dönüyor |
| **Nesne durumu** | `ReleaseTilt` geçilince nesnelerin kayması/devrilmesi. En güçlü kanal |
| **Ses** | Karton gıcırtısı, eğimle birlikte yükselen ses |
| *(bonus)* **HUD** | Denge göstergesi — bir çubuk ya da eğilen bir ok |

**Sadece platformun dönmesine güvenmeyin.** İzometrik kamerada küçük bir eğim fark edilmiyor;
nesnelerin kayması ve sesin yükselmesi bunu anlaşılır yapıyor.

> Eğim `FailTilt`'e yaklaşırken ses ve HUD göstergesi belirginleşsin — oyuncu kaybetmeden önce
> uyarılmış olmalı, yoksa haksız hissettiriyor.

---

## 9. Kesilenler

Notlarda vardı, bu spec'te yok. Sırayla, ihtiyaç olursa geri eklemek için:

| Kesilen | Neden | Yerine |
|---|---|---|
| **Envanter + select-slot** | Veri yapısı + UI + kalıcılık = kod saatleri | Sabit 4 nesne, 4 buton |
| **NPC / AI** | Davranış ağacı veya state machine, tek programcıda yok | Hareketsiz dekor insan modelleri |
| **Ayrı State System** | Bir enum yeterli | `EJMBuildState` |
| **Karakter animasyonu** | Rig + retarget + bağlama, tuzaklarla dolu | Başkan görünmeyebilir; imleç yeter |
| **Bina yıkım parçacıkları** | VFX zamanı | Nesne sadece devrilir |
| **Menü** | Doğrudan oyuna başla | `R` ile yeniden başlat |

> Bunların hiçbiri oyunu oyun yapan şey değil. Çekirdek döngü beşi olmadan da tam.

---

## 10. Uygulama sırası

Her adımın sonunda **ekranda görülebilir** bir şey var — `05_CALISMA_KURALLARI.md`'nin kuralı.

| # | İş | Bitişte ne görülür |
|---|---|---|
| 1 | İzometrik kamera + boş platform | Sahne duruyor |
| 2 | `AJMPlatform` sürekli sallanma | Platform nefes alıyor |
| 3 | Işın + hayalet mesh | İmleç platformda gezen bir hayalet |
| 4 | Izgaraya yapışma + geçerlilik rengi | Hayalet oturuyor, kenarda kırmızı |
| 5 | Yerleştirme → nesne doğuyor, bağlanıyor | Kule kuruluyor |
| 6 | Ağırlık merkezi → eğim | **Platform koyduğun yere eğiliyor** ← ilk gerçek doğrulama |
| 7 | `ReleaseTilt` → fizik bırakma | Her şey kayıp düşüyor |
| 8 | Kaybetme + yeniden başlatma | Döngü kapandı |
| 9 | Kazanma koşulu + sayaç | Oyun bitirilebilir |
| 10 | Ses + HUD göstergesi | Üç kanal tamam |

**6. adım projenin karar anı.** Platform koyduğun yere eğilmeye başladığında mekanik ya eğlenceli
geliyor ya gelmiyor. Gelmiyorsa 7–10 boşa gitmeden pivot yapılır.

**8. adıma kadar gelinmişse oyun teslim edilebilir.** 9 ve 10 iyileştirme.
