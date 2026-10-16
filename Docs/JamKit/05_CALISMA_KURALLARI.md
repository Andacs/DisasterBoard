# Çalışma kuralları

`04_UNREAL_TUZAKLARI.md` motorun tuzaklarını sayıyor. Bu dosya onların çoğunun **neden** başa
geldiğini anlatıyor: altı tane çalışma alışkanlığı. SonZarf'ta kaybedilen zamanın büyük kısmı bu
altı maddeden birinin ihlaliydi.

---

## 1. Ölç, tahmin etme

En pahalı ders. SonZarf'ta siyah at materyali **üç tur** ayarlandı — tint değerleri değiştirildi,
denendi, olmadı, tekrar değiştirildi. Sorun değerde değildi: materyal yanlış ebeveyne bağlıydı,
dolayısıyla **hiçbir doku miras alınmıyordu** ve hiçbir tint değeri işe yaramazdı.

Bir ölçüm script'i yazmak, üç turluk tahminden **ucuzdu**.

> **Kural:** bir sayı yazmadan önce nereden geldiğini söyleyebiliyor olmalısın. Söyleyemiyorsan
> ölçecek bir şey yaz.

Jam'de bunun pratik hâli: **bir değeri iki kez tahminle değiştirdiysen dur ve ölç.** Üçüncü tahmin
işe yaramayacak.

## 2. Ekranda görmediğin şey yoktur

SonZarf'ta "derlendi, doğrulama script'inden geçti, **PIE'de hiç bakılmadı**" diye bir kategori
oluştu ve **12 kaleme** çıktı. Her biri potansiyel yeniden-iş; üstüne yeni kod bindikçe borç büyüdü.

48 saatte bu borç ölümcül: 30. saatte "aslında hiçbiri çalışmıyormuş" demek jam'in sonu.

> **Kural: bir şey yazıldığı saat içinde ekranda görülür.** Görülmediyse bitmiş sayılmaz, commit
> mesajında da "görülmedi" yazar.

Pratik: her 6 saatte bir "ekranda görmediğimiz kaç şey var?" sorusu (`01_48SAAT_PLANI.md`).
Cevap 2'yi geçerse yeni özellik durur.

## 3. Kontrol üç şıklı olmalı: tamam / bozuk / ölçülemedi

Bir kontrolün kendi hatasını yutup temiz bir "sorun yok" basması, **hiç kontrol etmemekten kötüdür** —
çünkü artık yanlış bir güven var.

SonZarf'ta iki biçimde oldu:
- Başarısız bir çağrıdan sonra olumsuz dala düşüp "geçti" raporlamak
- Bir kontrolün *her şeye* "bozuk" demesi (bozuk olan kontroldü — deprecated API boş liste
  döndürüyordu)

> **Kural:** sonuç ya *tamam*, ya *bozuk*, ya **ölçülemedi**. Üçüncüsü eksikse kontrol yarım.
> Ve bir kontrol her şeye "bozuk" diyorsa önce bilinen-sağlam bir örnekte dene.

## 4. Var olanı önce ara

SonZarf'ta oyuncuya ayak sesi sistemi yazıldı — meğer zaten varmış ve **daha iyiymiş** (yüzey
tespitli). UHT'nin "shadowing" hatasıyla yakalandı ve geri alındı.

> **Kural:** yazmadan önce ara. `grep` bir dakika, yeniden yazmak bir saat.

Jam'de ek olarak: hazır asset'lerin içinde ne olduğuna bak. Aradığın materyal/mesh/ses paketin
içinde olabilir.

## 5. Asset yolunu ezberden yazma

`SM_PROP_chest_01` diye bir mesh olduğu varsayıldı, yoktu. Kayıt sandığı görünmez olacaktı; editör
açılışındaki CDO hatası yakaladı.

> **Kural:** yolu Content Browser'dan kopyala, ezberden yazma.

## 6. Ters alınabilir her şeye geri dönüş yolu bırak

Seviye değiştiren bir script yazıyorsan `RESTORE = True` bayrağı olsun. Kaydetmeden **önce** kendi
işini doğrula, sonra değil — SonZarf'ta bir koşu sıfır parsel üretti ve onu da kaydetti.

> **Kural:** sağlık kontrolü kayıttan önce. Beklenen sayı tutmuyorsa **kaydetme**, hata ver.

Jam'de bunun git hâli: riskli bir şeye başlamadan önce commit at. Bozarsan `git checkout` geri
getirir.

---

## Bonus: tasarım tarafında iki ders

Jam'de oynanış kararı verirken işe yarar.

### Tek kanal yetmez

Bir durum değişikliği tek kanaldan anlatılırsa fark edilmiyor. Renk filtresi "oyunun görünüşü"
sanılıyor, sayı menüde kalıyor, ses kaçırılıyor.

> **Kural: aynı bilgiyi en az üç kanaldan ver** — görsel + işitsel + nesne durumu. Üçü birden
> değişince fark etmemek imkânsız hâle geliyor.

### Soyut ceza hissedilmiyor, nesne durumu hissediliyor

"Kıtlıkta ekinler %30 yavaş büyür" oyuncunun ölçemediği bir şey. "Bacalar tütmüyor, tezgâh boş"
açıklama gerektirmiyor.

> **Kural: çarpanı da tut ama asla tek başına bırakma.** Her sayısal etkinin gözle görülür bir
> karşılığı olsun.
