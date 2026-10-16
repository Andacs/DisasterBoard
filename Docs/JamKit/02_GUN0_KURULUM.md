# Gün 0 kurulumu — jam başlamadan yapılacaklar

Bu listedeki hiçbir iş jam saatinden yenmemeli. Hepsi **en az iki gün önce** bitmiş olacak, çünkü
her maddenin ilk denemede patlama ihtimali var ve patlayınca çözmek saatler alıyor.

Kutular işaretlenecek. İşaretlenmemiş bir madde varsa jam'e hazır değilsin.

---

## 1. Proje

- [ ] Proje oluşturuldu, **adı bir daha değişmeyecek**

> SonZarf hâlâ `MyProject2` adıyla yaşıyor ve CLAUDE.md'de ilk sert kural "projeyi yeniden
> adlandırma" — çünkü `.uproject`, modül adı, `MYPROJECT2_API` makrosu, `Target.cs` dosyaları ve
> `DefaultEngine.ini`'deki redirect'ler birbirine bağlı. Baştan doğru adı ver, sonra dokunma.

- [ ] C++ projesi (Blueprint-only değil) — jam'de bile C++ hızlı demirleme sağlar
- [ ] Motor sürümü ekipte **birebir aynı** (5.8 ≠ 5.8.1). Farklı sürüm = açılmayan proje

## 2. Git ve LFS — binary asset'ler ilk commit'ten ÖNCE

- [ ] `git init`, uzak depo açıldı, herkes klonlayıp push edebildiğini doğruladı
- [ ] `.gitignore` yazıldı:

```
Binaries/
Intermediate/
DerivedDataCache/
Saved/
.vs/
*.sln
*.slnx
```

- [ ] **`.gitattributes` LFS kurallarıyla yazıldı ve commit edildi** — ilk `.uasset`'ten önce:

```
*.uasset filter=lfs diff=lfs merge=lfs -text
*.umap   filter=lfs diff=lfs merge=lfs -text
*.fbx    filter=lfs diff=lfs merge=lfs -text
*.glb    filter=lfs diff=lfs merge=lfs -text
*.png    filter=lfs diff=lfs merge=lfs -text
*.tga    filter=lfs diff=lfs merge=lfs -text
*.wav    filter=lfs diff=lfs merge=lfs -text
*.mp3    filter=lfs diff=lfs merge=lfs -text
*.ogg    filter=lfs diff=lfs merge=lfs -text
```

- [ ] `git lfs install` herkeste çalıştırıldı
- [ ] Bir test `.uasset` commit edilip `git show --stat` ile **pointer** olduğu doğrulandı

> SonZarf'ta 75 MB'lık PNG klasörü LFS kuralı olmadığı için düz blob olarak geçmişe gömülmek
> üzereydi; son anda yakalandı. Geçmişe girmiş bir blob'u çıkarmak history rewrite demektir ve
> ekiple çalışırken bu bir günü yer.

- [ ] İlk gerçek push denendi ve **süresi ölçüldü.** SonZarf'ın 2,8 GB'lık ilk push'u 10 dakikayı
      aştı; jam'de bunu bilmemek panik demek

## 3. Derleme

- [ ] Derleme komutu bir kez çalıştırıldı ve geçti
- [ ] Komut bir `.txt`'e yazıldı, herkes erişebiliyor

Bu makinede XGE belleği yetmediği için SonZarf'ta çalışan komut şu — yeni projede yol ve hedef adı
değişir, bayraklar aynı kalır:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" <ProjeAdi>Editor Win64 Development -project="C:/yol/<Proje>.uproject" -waitmutex -NoXGE -MaxParallelActions=2
```

- [ ] **Editör kapalıyken** derlendiği doğrulandı. Açık editör Live Coding kilidi yüzünden derlemeyi
      engelliyor

## 4. Paketleme — en kritik ve en çok atlanan madde

- [ ] Boş projenin bir kez **tam paketi** alındı
- [ ] Paket açıldı ve çalıştığı görüldü
- [ ] Paketleme süresi ölçüldü ve `01_48SAAT_PLANI.md`'ye not edildi
- [ ] Teslim edilecek konfigürasyon (Development mı Shipping mi) seçildi ve **o konfigürasyonla**
      paketlendi
- [ ] `.uproject`'te `AndroidFileServer` ve `XGEController` kapalı (`04` §1 ve §9)
- [ ] `araclar/` kopyalandı, `bash Tools/teslim_hazirla.sh` bir kez çalıştı (`10_TESLIM.md`)

> SonZarf iki aydır geliştiriliyor ve **bir kez bile paketlenmedi.** Jam'de paket teslimin kendisi.
> İlk paketleme denemesini 40. saatte yapmak, jam'i kaybetmenin en yaygın yolu.

## 5. Klasör düzeni — ilk asset'ten önce

- [ ] `Content/<ProjeAdi>/` → bizim yazdığımız her şey
- [ ] `Content/Vendor/` → satın alınan / indirilen paketler, **dokunulmaz**

> Bu ayrım SonZarf'ta işe yaradı: "bizim" ve "onların" bir bakışta ayırt edilebiliyor ve vendor
> paketleri yeniden içe aktarılabilir kalıyor.

- [ ] **Paket klasörlerini yeniden adlandırma yasağı** ekibe söylendi

> UE 5.8'de Blueprint içeren bir paketi yeniden adlandırmak `Kismet2.cpp:834` assertion'ında motoru
> çökertiyor. SonZarf'ta bir kez **34 dosya yedi.**

## 6. Hazır asset'ler — jam'den önce indirilmiş olacak

- [ ] Karakter + animasyon seti (Mixamo veya hazır pack) indirildi ve **içe aktarıldı**
- [ ] Ses paketi indirildi
- [ ] Bir UI font'u içe aktarıldı ve **Türkçe glifleri doğrulandı** (ş ğ ı İ ö ü ç)

> İki tuzak birden: `.ttf` import'u `UFontFace` üretir, `FSlateFontInfo`'nun istediği `UFont`
> **değil** — sarmalayan `UFont` ancak import factory'nin `BatchCreateFontAsset` bayrağı açıkken
> oluşuyor, yoksa widget'lar sessizce Roboto'ya düşüyor. Ve font factory headless commandlet'te
> çalışmıyor, `-ExecutePythonScript` ile normal editörden çağrılmalı. Sallantı'da daha kısa yol işe
> yaradı: `.ttf`'yi asset yapmadan oyun içinde dosyadan yüklemek (`04` §5).

- [ ] Fontta olmayan glif aranmayacağı için **UI metinlerinde emoji kullanılmayacağı** ekibe söylendi

> Fontta olmayan tek bir emoji (U+1F512) SonZarf'ta oyunu **41 saniye** dondurdu: Slate son çare
> fontuna düşüp oyun thread'inde senkron font yüklüyor.

## 7. Şablon dosyalar

- [ ] `CLAUDE.md.template` kopyalandı, doldurulup commit edildi
- [ ] `03_EKIP_KURALLARI.md` depoya kopyalandı — ekip onu projede görsün

---

## Son kontrol

Bu üçü yapılabiliyorsa hazırsın:

1. Yeni bir klasöre klonla, derle, editörü aç → çalışıyor
2. Bir `.uasset` değiştir, commit, push, başka makinede pull → LFS pointer'ı çözülüyor
3. Paketle, paketi çalıştır → oyun açılıyor
