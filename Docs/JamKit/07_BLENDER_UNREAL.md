# Blender → Unreal aktarımı

İki modelci (Mert, Asil) tek programcıya asset besliyor. Konvansiyon Gün 0'da bağlanmazsa her asset
ayrı bir sorun olur ve o sorunların hepsi Andaç'ın masasına düşer — yani tek darboğazı tıkar.

---

## Ne geçer, ne geçmez

| Geçer | Geçmez |
|---|---|
| Mesh geometrisi | **Shader node'ları** (Principled BSDF dahil) |
| UV'ler | **Modifier'lar** (uygulanmadıysa) |
| Vertex color | **Geometry Nodes** (realize edilmediyse) |
| Armature + skin ağırlıkları | Blender ışıkları ve kameraları |
| Animasyon | Fizik kurulumu |
| Materyal **slotları** (isim olarak) | Materyalin kendisi |

**En çok yanlış anlaşılan satır materyal.** Blender'da yaptığın shader Unreal'e gelmiyor; sadece
"burada bir materyal slotu var" bilgisi geliyor. Prosedürel bir şey yaptıysan **texture'a bake
etmen** gerekiyor.

> **Bu proje için iyi haber:** karton estetiği tek bir master materyalle çözülür. Modelciler
> materyal yapmasın — Unreal'de bir `M_Cardboard` olsun, hepsi onu kullansın. Renk farkı gerekiyorsa
> material instance + parametre. Bu hem tutarlı görünür hem aktarım derdini tamamen siler.

---

## Dört tuzak

### 1. Ölçek — en sık patlayan şey

Blender'da 1 birim = **1 metre**, Unreal'de 1 birim = **1 santimetre**. Yanlış ayarla model 100 kat
küçük ya da büyük geliyor.

FBX export'ta: `Forward: -Y`, `Up: +Z`, `Apply Scalings: FBX All`.

> **Ama bu ayarı ezberleme — doğrula.** Blender sürümüne göre davranış değişiyor. Gün 0'da **bir
> küp** çıkar: Blender'da 1×1×1 m, Unreal'de 100×100×100 birim görünmeli. Görünmüyorsa ayarı
> düzelt, sonra herkes aynı preset'i kullansın.

### 2. Transform'ları uygula

Export'tan önce Blender'da `Ctrl+A → Rotation & Scale`. Uygulanmamış dönüş/ölçek Unreal'de baked
olarak yanlış geliyor.

Modifier'lar da aynı: ya uygula, ya export'ta "Apply Modifiers" işaretle.

### 3. Pivot — bu projede kritik

Blender'daki **object origin**, Unreal'de pivot oluyor. Sürükle-bırak-yerleştir mekaniği olan bir
oyunda yanlış pivot her binayı yere yarı gömülü bırakır.

> **Konvansiyon: origin tabanın ortasında.** Blender'da nesneyi seç → tabana imleç koy →
> `Object → Set Origin → Origin to 3D Cursor`.

Bu kararı Gün 0'da ver. SonZarf'ta "yerleştirilen şey havada kalıyor veya gömülü" hatası **üç kez**
tekrarlandı, hepsi referans/pivot meselesiydi.

### 4. Collision — denge oyununda mekaniğin kendisi

Fizikle devrilen bir kule yapıyorsun; çarpışma şekli oyunun hissi demek.

- **Basit kutu collision kullan.** `UBX_<MeshAdı>_01` isimli bir kutu mesh'i Blender'da yanına koy,
  Unreal otomatik collision olarak alır.
- Karmaşık convex hull hem yavaş hem fizikte **kararsız** — kuleler titreyip kendiliğinden devrilir.
- Karton kutu/bina formları zaten kutuya yakın, yani bu hem doğru hem kolay.

Prefix'ler: `UBX_` kutu · `USP_` küre · `UCX_` convex · `UCP_` kapsül.

---

## Dosya formatı

**FBX** varsayılan olsun. glTF de çalışıyor ama iskeletli mesh'te FBX hâlâ daha güvenli.

---

## İsimlendirme

```
SM_<Kategori>_<Ad>        SM_Bina_Ev01, SM_Nesne_Sandik
UBX_<MeshAdı>_01          collision
T_<Ad>_<Tip>              T_Cardboard_D (diffuse)
M_<Ad>                    M_Cardboard
```

Asset'ler `Content/<ProjeAdi>/` altına. `Content/Vendor/`'a dokunulmaz.

---

## Güncelleme kuralı — SonZarf'tan

> **Mevcut bir asset'in üstüne import yapma.** Import ayarları sadece *yeni yaratılan* varlığa
> uygulanıyor; üstüne import eski ayarları koruyor. Modelci bir mesh'i güncelleyip yeniden
> gönderdiğinde, Unreal'de **önce sil sonra import et** — yoksa düzelttiğin ayar geri gelmiyor.

Ayrıca: bir mesh'i yeniden import etmek onu kullanan seviyeyi etkiliyor, yani seviye sahibi
(Yiğit) haberdar olsun.

---

## Animasyon notu

Karakter animasyonu kullanacaksanız:

- **Mesh ve klipler aynı turdan inmeli.** Farklı rig turlarından gelen mesh + klip kombinasyonu
  hiçbir ölçekle düzelmiyor — SonZarf'ta kemik oranları gövdede 0.75, kolda 0.53 çıktı ve karakter
  "şişik hâlde doğru boyda" oldu.
- **Mixamo'dan geçireceksen dokuyu geçirme.** Mixamo'nun 2014 FBX SDK'sı çoklu dokuda rastgele
  seçim yapıyor ve dokuyu öldürüyor. Rig'i al, dokuyu Unreal'de bağla.
- Aynı rig'den gelen bütün karakterler **tek bir Skeleton asset'ine** bağlansın, yoksa animasyon
  paylaşamıyorlar.

> **Bu projede muhtemelen gerek yok.** İzometrik kamera + sürükle-bırak mekaniğinde başkan
> görünmeyebilir bile. Karakter animasyonu kesme listesinin üst sıralarında — `06_PROJE_BRIEFI.md`.

---

## Gün 0 testi

Modelciler üretime başlamadan önce **bir** asset uçtan uca geçsin:

1. Blender'da basit bir karton kutu, origin tabanda, `UBX_` collision'ı yanında
2. FBX export, anlaşılan preset'le
3. Unreal'e import
4. **Ölç:** boyutu Blender'daki ölçünün 100 katı mı?
5. Seviyeye koy: yere oturuyor mu, gömülü/havada mı?
6. Üstüne bir tane daha at: fizik stabil mi, titriyor mu?

Altısı da geçtiyse preset doğru, herkes onu kullanır. Geçmediyse **şimdi** düzelt — 40 asset
üretildikten sonra değil.
