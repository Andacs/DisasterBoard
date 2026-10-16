# Proje briefi — <OYUN ADI henüz yok>

13 Ekim 2026 tarihli el yazısı notlardan aktarıldı. **Yanlış okuduğum yer varsa düzelt** — işaretli
maddeler emin olmadıklarım.

---

## Konsept

> **"Sallanan dünyada dengeyi ve düzeni sağlamaya çalışan başkan"**

- 3D **izometrik**
- **Cardboard** (karton) estetiği
- **Cozy / lowpoly**
- **Hedef kitle: 13–21 yaş. Oyun 13+.**
- Yer: İstanbul

Bu konsept jam için iyi: tek cümleyle anlatılıyor, görsel dili ucuz (karton = düz renk, basit
malzeme), ve mekaniği cümlenin içinde — **denge**.

---

## Ekip ve sahiplik

| Kişi | Alan |
|---|---|
| **Mert** | Model, topoloji, texture, lighting |
| **Andaç** | **Tek programcı.** Mekanik, kod, model animasyon, karakter, yerleştirme, environment yerleştirme, paket fix |
| **Asil** | Concept tasarım + modeller |
| **Yiğit** | Proje yardımcısı (Andaç'a), UI–UX–HUD tasarımı, environment ve level fikirleri |

## ⚠️ Projenin tek darboğazı: bir programcı

Kod gerektiren **her şey** Andaç'ın arkasında kuyruğa giriyor. Mert ve Asil model/texture üretiyor,
Yiğit tasarım ve fikir üretiyor — üçü de paralel çalışabiliyor ve üretimleri birikiyor. Oyunu
oynanabilir yapan tek hat tek kişi.

Bunun iki sonucu var ve ikisi de kapsamı belirliyor:

1. **Kod listesi acımasızca kesilecek.** Aşağıdaki kesme listesi tavsiye değil, zorunluluk.
2. **Andaç'ın üstünden kod olmayan işler alınacak.** Altı işten dördü aslında kod değil.

---

## Planlanan içerik

### Varlıklar
- Nesneler, binalar, cardboard malzemesi, insan modelleri

### Arayüz
- UI–UX–HUD, envanter, select–slot, build UX

### Fizik ve karakter
- Physics, karakter, animasyon, **bina physics**

### Sistemler
| Mechanic | AI | Level | Environment |
|---|---|---|---|
| Sürükle | NPC | **?** | **?** |
| Build | Default | | |
| State system | Rotate | | |
| | State system | | |

---

## Benim okuduğum riskler

Bunlar notlarda yazmıyor; ilk saatte konuşulması gereken şeyler.

### 1. Andaç'ın sütunu boşaltılacak — dördü kod değil

Altı iş yazılmış. Sadece ikisi gerçekten programcı işi:

| İş | Kod mu? | Kime geçmeli |
|---|---|---|
| Mekanik / sistem kodu | **Evet** | Andaç — başka kimse yapamaz |
| Model animasyon (import, retarget, bağlama) | Kısmen | Mert (modelci zaten içinde) |
| Karakter kurulumu | Kısmen | Andaç, ama en sona |
| **Yerleştirme** | Hayır — editör işi | **Yiğit** |
| **Environment yerleştirme** | Hayır — editör işi | **Yiğit** |
| **Paket fix / teslim** | Hayır — Gün 0'da kurulmuşsa komut | **Yiğit** |

**Yiğit "proje yardımcısı" ise, yardımın somut hâli budur.** "Yardımcı" diye soyut bir rol jam'de
hiçbir şeye dönüşmez; üç somut iş devralmak dönüşür. Üçü de kod bilmeden yapılabilir ve üçü de
Andaç'ın en çok zamanını yiyecek olanlar.

Bunlar geçerse Andaç'ta sadece **mekanik kodu** kalır — tek programcılı bir ekipte doğru dağılım bu.

**Ek:** Yiğit Blueprint yapabiliyorsa UI/HUD'u da kendisi kurabilir. Bu durumda Andaç arayüze hiç
dokunmaz. Jam başında sor: "Blueprint'te buton ve bar yapabilir misin?"

### 2. Kapsam: "sürükle + build + physics + AI + envanter" beş sistem

48 saatte biten şey bir mekanik. Bu listede en az beş var.

**Çekirdek olarak öneri:** *sürükle → yerleştir → denge bozulur → düzelt.* Bu tek döngü ve konseptin
tamamını taşıyor.

**İlk atılacaklar (kesme listesi taslağı):**
1. Envanter + select-slot → sabit 3–4 nesne, yan yana butonlar
2. NPC / AI → tamamen çıkar, ya da hareket etmeyen dekor insan
3. State system → build durumu bir enum yeter
4. Karakter animasyonu → başkan görünmeyebilir; kamera izometrik, el imleci yeter

Bunlar çıkınca geriye kalan şey hâlâ oynanabilir bir oyun; çıkmazsa hiçbiri bitmez.

### 3. Kazanma ve kaybetme yok

Notlarda yok. "Denge sağlamaya çalışan" bir oyunun **kaybetme koşulu mekaniğin kendisi**:
denge belirli bir açıyı geçerse yapı devrilir.

**Öneri:** kaybetme = yapı devrildi. Kazanma = N saniye ayakta kaldı / M bina yerleştirildi.
İlk saatte karara bağla.

### 4. Level ve Environment sütunları "?"

İkisi de boş. Yiğit'in rol tablosunda level design ve environment design var, yani kişi belli ama
**içerik belli değil.**

**Öneri:** izometrik + denge mekaniği için level = **tek bir platform**. Seviye tasarımı, platformun
şekli ve üstüne konacak nesnelerin sırası demek. Bu ilk saatte yarım sayfada kararlaştırılır.

### 5. Seviye dosyasının sahibi: Yiğit

`03_EKIP_KURALLARI.md`'nin 1. kuralı: tek seviye, tek sahip. Yiğit hem level/environment fikirlerini
üretiyor hem (yukarıdaki devirle) yerleştirmeyi yapıyor — yani `.umap`'in sahibi net olarak o.

**Andaç seviyeyi kaydetmez.** Koda bakar, test için PIE'de oynar, ama kaydetmez. Mert ve Asil de
kaydetmez; asset üretip Content'e koyarlar, yerleştirme Yiğit'te.

Bu kural tek programcılı ekipte ayrıca işe yarıyor: Andaç derleme için editörü sürekli kapatıp
açacak, ve kapalı editör kaydedilmemiş seviye değişikliğini götürür.

#### ⚠️ Mert'in "lighting" işi bu kuralla çakışıyor

Işıklandırma seviyenin içinde yaşıyor — yani Mert lighting yaparsa `.umap`'e dokunuyor, ama dosyanın
sahibi Yiğit. İki çözüm var, biri seçilecek:

- **Mert ışık ayarlarını söyler, Yiğit uygular.** Basit ama yavaş; ışık ayarı bakarak yapılan bir iş.
- **Mert'e ışık penceresi verilir.** "Seviye sende" denir, Mert ışığı kurar, kaydeder, push eder,
  "seviye bitti" der. Bu sırada Yiğit `.umap`'e dokunmaz. **Önerilen bu** — ışık tek seferlik bir
  pass, sürekli iş değil.

Hangisi olursa olsun: aynı anda iki kişi seviyede değil.

#### Andaç'ın seviyeye ihtiyacı olduğunda

Programcının test için bir şeyler yerleştirmesi gerekiyor. Çözüm: **ayrı bir test seviyesi**
(`L_Test`). Andaç orada istediğini yapar, kaydeder, kimseye sormaz.

**Ama:** `L_Test`'teki hiçbir şey ana seviyeye taşınmayacak. Deneme alanı, paralel kopya değil.
Bu ayrımın kaybedildiği an `03_EKIP_KURALLARI.md`'deki 106 aktör hikâyesi tekrarlanır.

---

## Hedef kitle ne değiştiriyor

13–21 yaş ve 13+ derecelendirme, jam'de dört somut karar demek.

### Öğretici yok, mekanik ekrandan okunsun

13 yaşındaki bir oyuncuyla 21 yaşındaki aynı sabırda değil, ve jam'de **kimse metin okumuyor** —
oyunu 2–5 dakika deneyip geçiyorlar. Mekanik ilk 30 saniyede kendini anlatmalı.

İyi haber: *sürükle–bırak–denge bozulur* zaten kendini anlatan bir mekanik. Bunu koru, üstüne
açıklama ekleme. Gerekirse **tek** satır: "Kuleyi ayakta tut."

> `05_CALISMA_KURALLARI.md` → "tek kanal yetmez": dengenin bozulduğu üç kanaldan anlaşılsın —
> platformun eğilmesi (görsel), gıcırtı (ses), nesnelerin kayması (nesne durumu). Üçü birdenken
> hiç kimse "ne oluyor" diye sormaz.

### Oyun süresi kısa

Bir tur **60–120 saniye**. Jam'de uzun oyun oynanmadan kapatılıyor; kısa tur hem bitirilir hem
tekrar oynanır.

### 13+ içerik sınırı

Karton estetiği zaten güvenli bölgede. Kaçınılacaklar: gerçekçi hasar/kan, ağır tema, şans oyunu
mekaniği. "Başkan" çerçevesi sorunsuz — siyasi gönderme eklemeyin, 13+ için gereksiz risk.

### Metin azlığı dil sorununu da çözüyor

Türkçe mi İngilizce mi tartışmasına girmeyin: UI'da ne kadar az kelime olursa o kadar iyi. Sayı,
ikon ve bar yeterli. Jam sayfası açıklaması İngilizce olsun, oyunun içi neredeyse metinsiz.

---

## Çözülecek belirsizlikler

- [ ] Yiğit Blueprint yapabiliyor mu? (Yapabiliyorsa UI/HUD tamamen ona geçer)
- [ ] Yerleştirme, environment yerleştirme ve paketleme Yiğit'e devredildi mi? (Risk 1)
- [ ] Oyunun adı
- [ ] Kazanma / kaybetme koşulu
- [ ] Oyunun adı
- [ ] Kazanma / kaybetme koşulu
- [ ] Paketleme/teslim kimde?
