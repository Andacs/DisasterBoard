# Jam Kiti — 48 saat · Unreal 5.x · ekiple

Bu klasör SonZarf'ın bedeli ödenmiş hatalarından çıkarılmış bir başlangıç kiti. Amacı tek şey:
**jam saatini motorla boğuşmaya değil oyun yapmaya harcamak.**

Kaynak: `C:\Dev\SonZarf` deposunun kendi kayıtları — `Docs/SESSION_HANDOFF.md`'deki "tekrar
etmemesi gereken hatalar" bölümü (25 madde), `Docs/Status_History.md`, `.claude/skills/unreal-automation`
ve Ağustos–Eylül oturumlarının tutanakları. Buradaki her kural gerçekten bir şeye mal olmuş.

---

> **16 Ekim jam günü: önce [`13_JAM_GUNU.md`](13_JAM_GUNU.md).** Yeni sohbete ilk mesaj, ilk saatin
> sırası ve tek komutluk proje kurulumu (`araclar/yeni_proje.sh`) orada.

## Dosyalar ve ne zaman okunacağı

### Süreç — jam'den bağımsız, her projede geçerli

| Dosya | Ne zaman | Kim okur |
|---|---|---|
| [`01_48SAAT_PLANI.md`](01_48SAAT_PLANI.md) | **Jam başlamadan** bir kez; sonra her 6 saatte bir kontrol | Herkes |
| [`02_GUN0_KURULUM.md`](02_GUN0_KURULUM.md) | **Jam başlamadan en az 2 gün önce.** Bu listedeki hiçbir iş jam saatinden yenmemeli | Andaç |
| [`03_EKIP_KURALLARI.md`](03_EKIP_KURALLARI.md) | Jam başlarken, ekibe birlikte okunacak. 15 dakika sürer, bir gün kazandırır | Herkes |
| [`04_UNREAL_TUZAKLARI.md`](04_UNREAL_TUZAKLARI.md) | Bir şey tuhaf davrandığında **önce buraya bak.** Belirti → sebep tablosu | Andaç, Yiğit |
| [`05_CALISMA_KURALLARI.md`](05_CALISMA_KURALLARI.md) | Jam başlamadan bir kez; özellikle "ölç, tahmin etme" | Herkes |
| [`10_TESLIM.md`](10_TESLIM.md) | **Jam başında bir kez** (ilk saatte yapılacaklar), 17 Ekim akşamı prova, 18 Ekim teslim günü | Andaç, Yiğit |
| [`11_PROVA_DERSLERI.md`](11_PROVA_DERSLERI.md) | Jam başlamadan bir kez: Sallantı provasında ölçülen süreler, ne işe yaradı | Herkes |
| [`12_KURULUM_YENI_BILGISAYAR.md`](12_KURULUM_YENI_BILGISAYAR.md) | Ekipten biri kendi bilgisayarında derleyecekse: kendi Claude'una verir, Claude kurar ve 6 testi koşar | Yiğit ve kim gerekiyorsa |
| [`14_KURULUM_GITHUB_MODELCI.md`](14_KURULUM_GITHUB_MODELCI.md) | Modelciler (Mert, Asil) kendi yapay zekâ asistanına verir: yalnızca Git + LFS + GitHub, model gönderme kuralları (`Art/Modeller/`) | Mert, Asil |
| [`13_JAM_GUNU.md`](13_JAM_GUNU.md) | **16 Ekim 18.00'de ilk okunacak:** ilk mesaj, ilk saat, sonraki saatler | Andaç, Claude |
| [`araclar/`](araclar/) | `yeni_proje.sh <Ad>` hepsini yeni projeye kurar (boş C++ proje, araçlar, kit, public repo) | Andaç |
| [`CLAUDE.md.template`](CLAUDE.md.template) | Proje açılır açılmaz kopyala, doldur, commit et | Andaç |

### Bu proje — GDD'nin kendisi

| Dosya | İçerik | Kim okur |
|---|---|---|
| [`06_PROJE_BRIEFI.md`](06_PROJE_BRIEFI.md) | Konsept, ekip, sahiplik, riskler, kesme listesi | Herkes |
| [`07_BLENDER_UNREAL.md`](07_BLENDER_UNREAL.md) | Aktarım hattı, export ayarları | Mert, Asil |
| [`08_ASSET_SPEC.md`](08_ASSET_SPEC.md) | Ölçü, pivot, collision, isim, poly bütçesi, UV | **Mert, Asil — bağlayıcı** |
| [`09_MEKANIK_SPEC.md`](09_MEKANIK_SPEC.md) | Denge sistemi, sınıflar, ayarlanabilir değerler, uygulama sırası | **Andaç — bağlayıcı** |

Henüz yazılmadı, gerekirse istenecek: UI/HUD spec (Yiğit), ses spec, level/tasarım spec.

---

## Üç cümlelik özet

Acelen varsa sadece bunları oku:

1. **Kurulum jam saatinden yenmez.** Proje, LFS, .gitignore, derleme komutu ve **bir test paketi**
   jam başlamadan hazır olacak. SonZarf bir kez bile paketlenmedi — jam'de paket teslimin kendisi.

2. **Ekranda görmediğin şey yok sayılır.** SonZarf'ta "derleniyor ama PIE'de hiç bakılmadı" listesi
   12 kaleme çıktı. 48 saatte bu liste 2'yi geçerse jam bitti demektir.

3. **Seviyeyi kopyalayıp "onda çalışmak" yasak.** SonZarf'ta tam bu yapıldı: üç hafta sonra iki
   seviye çatallandı, 106 aktör yanlış dosyada kaldı ve taşımak script'le mümkün olmadı.

---

## Jam'in ilk 30 dakikası

Tema açıklandıktan sonra sırayla:

1. **Tek cümle yaz.** "Oyuncu ___ yapar, ___ olunca kazanır." Cümle kurulamıyorsa fikir hazır değil.
2. **Kesme listesini şimdi yap** (`01_48SAAT_PLANI.md` → "Önceden hazırlanmış kesme listesi").
   Neyin atılacağına baskı altında değil, şimdi karar ver.
3. **Sahiplik dağıt** (`03_EKIP_KURALLARI.md` → "Kim neyin sahibi"). Özellikle: **seviye dosyasının
   tek bir sahibi var.**
4. **İlk commit'i at.** Boş proje bile olsa. Push et. Herkes klonlayabildiğini doğrulasın.
5. **Repoyu public yap, `araclar/`'ı ve README şablonunu koy** (`10_TESLIM.md` → "Proje açılırken").
   Private repo puandan bağımsız diskalifiye (madde 6.5).

Bu beş madde bitmeden kod yazılmaya başlanmayacak.
