# Sallantı provasından dersler (14–15 Ekim 2026)

Sallantı (`C:\Dev\Sallanti`, public https://github.com/Andacs/Sallanti) jam'den iki gün önce, tema
yokken yapılan prova. İki günde: üç bölüm, ana menü, açılış videosu, jenerik, trailer, test edilmiş
Shipping ZIP. **Teslim edilemez (3.1);** buradaki her şey ne çalıştığını ve ne kadar sürdüğünü gösteriyor.
Kod örneği lazım olursa oraya bak, kopyalama: jam oyunu yeni yazılır.

---

## Ölçülen süreler (bu makine)

| İş | Süre |
|---|---|
| Boş projenin ilk derlemesi | 70 sn |
| Editörün soğuk açılışı | 97 sn |
| Artımlı derleme (`derle.bat`) | 5 sn – 1 dk |
| Shipping paket, sıcak | 111–134 sn (ilk soğuk deneme XGE yüzünden asıldı, bkz. 04) |
| ZIP (Windows `tar`), 650 MB paket → 348 MB | ~1 dk |
| `teslim_hazirla.sh` tamamı | 3 dk 22 sn |
| Trailer çekimi (36,5 sn video) + ffmpeg | birkaç dk; oyun 30 fps sabit karede oynar |

## Tasarım dersleri

**Sıralı görev = simülasyon hissi.** İlk sürümde oyun "ev kur → hastane kur → yolla bağla" diye görev
veriyordu. Andaç'ın yorumu: oyuncu oynamıyor, simülasyonu izliyor. Çözüm: bölüm açık, istekler gizli
(aileye tıklayınca öğreniliyor), sıra oyuncunun. **Jam'de ilk oynanabilir sürümde sor: oyuncu karar
veriyor mu, yoksa talimat mı uyguluyor?**

**Arayüz yönü bir kez seçilir.** İlk HUD "benchmark testi gibi" bulundu. Üç farklı yön (geometrik /
karton kesik / gökyüzü pusulası) tek sayfada gösterildi, ekip C'yi seçti, bir oturumda uygulandı.
"Sade" bir yön değil. Yönü saat 24'ten önce seç; sonra yalnızca o yönde ilerle.

**Yazı tipi Gün 0 kararı.** Cinzel + Jost en sona kaldı ve UE font aktarıcısı başsız çöktü (bkz. 04).
Jam'de font ilk saatte seçilsin, `Content/Fonts/`'a girsin.

**Panelsiz yazı her zaman okunmaz.** Değişen arka plan (gece/gündüz, bulut) üstünde metin kayboluyor.
Arkasına soluk krem bir hap (pill) koymak çözdü; gölge/hale çamurlu duruyor.

**Kaynak ve AI beyanı yazarken zor, sonradan imkânsız.** Sallantı'da sonradan toplandı ve beş
kategorinin her biri tek tek hatırlanmak zorunda kaldı. Jam'de kaynak eklendiği anda README'ye.

## Teknik dersler (ayrıntısı 04'te)

- **Tek harita, bölüm URL'den:** `L_Ana?Bolum=2`, `OpenLevel` ile. Seviye kopyalanmadı, çatallanma olmadı.
- **Otomatik testler fareyi kullanamıyor** (Andaç fareyi oynatıyor); testler tıklamanın yaptığı işi
  doğrudan çağırıyor. Fare üstü efektleri ve gerçek tıklamayı bir insan denemeli.
- **Sesleri Claude duyamıyor.** Sentezlenen her ses ve müzik bir insan tarafından dinlenmeli.
- **Shipping'de konsol ve log yok.** Paketi denemek için `pencere_goruntu.ps1` (yalnızca oyun penceresi;
  masaüstünün görüntüsü hiç alınmaz, bir kez Discord yakalanmıştı).
- **Trailer oyunun içinden:** sinema komutu kamerayı senaryoya göre sürer, `-benchmark -fps=30` her
  kareyi tam 1/30 sn yapar → makine yavaş olsa da video akıcı. Açılış videosu aynı çekimden kesilir.
- **Elle yapılan ZIP aslında tar'dı.** Git Bash'in `tar -a`'sı .zip adlı tar yazıyor; Gezgin açamaz.
  Teslim kontrol betiği yazılırken fark edildi. Artık betik Windows `tar.exe` kullanıyor ve `PK`
  imzasına bakıyor.
- **AndroidFileServer anahtarı geri geldi.** Repo public yapılmadan önce silindi, editör bir sonraki
  açılışta yenisini yazdı. Eklenti `.uproject`'te kapatılınca durdu.
- **Repo public olunca commit e-postası da görünür.** İstenmiyorsa jam reposunda ilk commit'ten önce
  GitHub'ın `noreply` adresine geç.

## Jam'de farklı yapılacaklar

1. Repo ilk saatte public; `teslim_hazirla.sh` 17 Ekim akşamı bir kez tam çalışsın.
2. README şablonu ilk commit'te; beyanlar kaynak eklendikçe.
3. Font ve arayüz yönü ilk 24 saatte.
4. Menü, açılış videosu, jenerik **kesme listesinde**: kural istemiyor (yalnız video, ZIP, repo, README
   zorunlu). Sallantı'da hepsi bir akşamda yapıldı ama çekirdek oyun bitmeden başlanmaz.
