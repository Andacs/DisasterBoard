# 48 saat planı

SonZarf'ın en pahalı dersi kapsam: **yedi fazlık bir hikâye planlandı, iki buçuğu bitti.** İki ay
sonra elde "çok iyi bir açılış" vardı ama oyun yoktu. 48 saatte bu hata telafi edilemez, o yüzden
kapsam bu dosyanın tamamı.

---

## Temel kural: BİR mekanik

48 saatte bitirilebilen şey:

- **bir** çekirdek mekanik (zıplama değil — mekanik: "ışığı taşıyorsun ve ışık tükeniyor")
- **bir** seviye
- **bir** kazanma ve **bir** kaybetme durumu
- hazır/ücretsiz asset, sıfır üretim

Bitirilemeyen şey: ikinci mekanik, menü sistemi, kayıt/yükleme, diyalog, envanter, birden fazla
seviye, kendi karakter modeli, hikâye.

> SonZarf'ta kayıt sistemi, envanter, diyalog, başarımlar, gün döngüsü, craft — hepsi var ve hepsi
> aylar aldı. Bir jam oyununda bunların hiçbiri puan getirmez; **oynanabilir bir çekirdek** getirir.

---

## Saat saat

Saatler jam başlangıcından sayılır. Uyku planın içinde — plansız uykusuzluk, doğrulanmamış iş üretir.

| Saat | İş | Bitişte elde ne var |
|---|---|---|
| **0–1** | Tema → tek cümle. Kesme listesi. Sahiplik dağıtımı. İlk commit + push | Herkes ne yaptığını biliyor |
| **1–3** | Tek seviye oluştur, grey box. Oyuncu hareket ediyor | Yürünebilir bir boşluk |
| **3–10** | **Çekirdek mekanik.** Sadece o. Kutular ve silindirlerle | Mekanik çalışıyor, çirkin |
| **10–12** | **Oynat.** Ekibin tamamı. Eğlenceli değilse **şimdi** değiştir | Karar: devam mı pivot mu |
| **12–18** | **UYKU** (en az 6 saat, vardiyalı değil — herkes birlikte) | Çalışan kafa |
| **18–24** | Döngüyü kapat: kazanma, kaybetme, yeniden başlama | Baştan sona oynanabilir |
| **24–30** | Asset geçişi: hazır model/doku/ses. Tek görsel dil | Oyun oyuna benziyor |
| **30–36** | Seviye tasarımı, zorluk eğrisi, ses karışımı | Oynanış akıyor |
| **36** | 🔴 **ÖZELLİK DONDURMA.** Bu saatten sonra yeni kod yok | — |
| **36–42** | Hata avı + **paketleme** | Çalışan paket |
| **42–44** | `teslim_hazirla.sh` BOZUK: 0, trailer, yükleme, makbuz (`10_TESLIM.md`) | Teslim edildi |
| **44–48** | Tampon. Kullanılacak | Nefes |

### Neden 36. saatte dondurma

> **Sallantı Gün 0 ölçümü (14 Ekim):** boş proje Shipping paketi sıcak önbellekle **117 sn**, 639 MB.
> Soğuk ilk deneme ~19 dakikaya varıp XGE yüzünden asıldı (`04_UNREAL_TUZAKLARI.md`). Jam'de ilk
> gerçek paket için **30 dakika** bütçeleyin; 36. saatteki paket penceresi bu yüzden 6 saat.

> **Sallantı provası (15 Ekim):** dolu projenin paketi 111–134 sn; paket + ZIP + temiz klasörde deneme +
> video + repo kontrolü tek komutta 3 dk 22 sn (`araclar/teslim_hazirla.sh`). İlk elle yapılan ZIP aslında
> tar çıktı (bkz. 04 §9): kontrol betiği olmadan bu teslimde fark edilmezdi.
>
> **Kitapçığın saatleri:** son teslim 18 Ekim 18.00, yükleme **kaydı** o saatten önce; Git desteği için sınır
> 12.00. Jam 16 Ekim 18.00'de başlıyor, yani 36. saat = 18 Ekim 06.00, 42. saat = 12.00.

Paketleme Unreal'de **her zaman** beklenenden uzun sürer ve ilk denemede patlar. SonZarf iki aydır
geliştiriliyor ve **bir kez bile paketlenmedi** — jam'de paket teslimin kendisi olduğu için bu risk
alınamaz.

Ayrıca: `ensure()` çağrıları Shipping derlemesinde atılır. SonZarf'ta ekim/hasat donması tam olarak
bu yüzden "oyuncunun görmediği" bir hata — ama tersi de doğru: Development'ta çalışan bir şey
Shipping'de farklı davranabilir. **Hangi konfigürasyonu teslim edeceksen onu Gün 0'da bir kez
paketle.**

---

## Önceden hazırlanmış kesme listesi

Baskı altında "neyi atsak" tartışması yarım saat yer ve kötü karar üretir. Sıralamayı **ilk saatte**
yap, sonra tartışmasız uygula.

Şablon — jam başında doldurulacak:

```
ATILACAKLAR SIRASI (ilk atılan en üstte)
1. ___________________   (örn. ikinci düşman tipi)
2. ___________________   (örn. ses karışımı ince ayarı)
3. ___________________   (örn. ana menü — doğrudan oyuna başla)
4. ___________________   (örn. ikinci seviye)
5. ___________________   (örn. parçacık efektleri)

ASLA ATILMAYACAK (bunlar olmadan oyun yok)
- çekirdek mekanik
- kazanma/kaybetme durumu
- yeniden başlatma
- çalışan paket
```

**Kural:** 24. saatte "aslında atılmayacak" listesindeki bir şey hâlâ çalışmıyorsa, atılacaklar
listesinin tamamı atılır. Tartışma yok.

---

## Her 6 saatte bir sorulacak üç soru

Telefona alarm kur. Alarm çalınca ekip durur ve bunları cevaplar:

1. **Oyun şu an oynanabilir mi?** (Hayır ise: yeni özellik yok, mevcut olanı oynanabilir yap)
2. **Ekranda görmediğimiz kaç şey var?** (2'den fazlaysa dur ve bak — `05_CALISMA_KURALLARI.md`)
3. **Son commit ne zaman push edildi?** (1 saatten eskiyse şimdi push et)

Üçüncüsü neden bu kadar sert: SonZarf'ta çalışma **üç hafta boyunca** tek diskte durdu — 101
commit, 2,8 GB, yedek yok. Bir disk arızası veya yanlış bir `git checkout` üç haftayı götürürdü.
48 saatte aynı olay bir jam'i götürür.

---

## Tema yorumu tuzağı

Jam temasını "derinlemesine yorumlamak" cazip ve pahalı. SonZarf'ın hikâyesi gerçekten iyi — Deccal
ifşası, erdem/konfor sorusu — ve **o hikâyenin hiçbir parçası oynanabilir hâle gelmedi** çünkü
anlatı mekaniği bekliyordu, mekanik de sistemi.

48 saatte sıra tersi: **mekanik önce, tema sonra.** Çalışan bir mekaniğe tema giydirmek bir saat;
temaya mekanik yetiştirmek 48 saat yetmez.
