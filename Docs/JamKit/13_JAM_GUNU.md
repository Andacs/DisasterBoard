# Jam günü — 16 Ekim 2026 Cuma, 18.00

Bu sayfa yarın yeni Claude sohbetinin **ilk okuyacağı** sayfa. Hazırlık 15 Ekim gecesi bitti; burada
yalnızca sıra var.

---

## Evden çıkmadan (16 Ekim, alan girişi 14.00, kayıt en geç 16.00 — ODTÜ KALTEV)

- **Tamam (16 Eki sabah):** Andaç'ın Unreal'i **5.8.3**'e güncellendi (Yiğit'inkiyle aynı); `derle.bat` 2 dk, `SLTestLevel3` tamam=1
- İki bilgisayarda **Windows Update 1 hafta duraklatıldı**, uyku "Hiçbir zaman" (48 saatte otomatik yeniden başlatma = kayıp)
- Takım temsilcisi **Andaç**: organizasyonla iletişim ve web teslimi onda; 6 karakterli teslim kodu ona gelir, kimseyle paylaşılmaz, repoya yazılmaz (5.1, 2.4)
- Mert ve Asil'in GitHub kullanıcı adları alındı; ikisi `14_KURULUM_GITHUB_MODELCI.md`'yi (Antigravity/Gemini) uyguladı, "SONUC: GITHUB HAZIR" raporu geldi. Modeller yalnızca `Art/Modeller/`'a push edilir, Content'e Andaç aktarır
- Yanımızda: şarj aletleri, **uzatma kablosu / çoklu priz**, fare, kulaklık, telefon (internet yedeği), harici disk

## Yeni sohbete ilk mesaj (kopyala, boşlukları doldur)

> CIVIL GAME JAM başladı. Tema: **___**. Fikrimiz: **___** (oyuncu ___ yapar, ___ olunca kazanır).
> Önce `C:\Dev\JamKit\13_JAM_GUNU.md` ve `00_OKU_BENI.md`'yi oku. Proje adı **___** olsun; `yeni_proje.sh` ile
> projeyi aç, ekibi repoya ekle, sonra çekirdek mekaniğe başlayalım.

Fikir henüz yoksa ilk mesaj: "Tema ___. Fikir bulmamıza yardım et; `01_48SAAT_PLANI.md`'deki 'BİR mekanik'
kuralına göre 3 aday öner, her biri tek cümle."

---

## İlk saat (18.00–19.00)

| Saat | İş | Kim |
|---|---|---|
| 18.00–18.40 | Tema → **tek cümle**. Mekanik önce, tema giydirme sonra (`01`). 3 aday, 1 seçim | Herkes |
| 18.40 | Proje adı (İngilizce harf, büyük harfle başlar, Türkçe karakter yok; sonra değişmez) | Andaç |
| 18.45 | `bash /c/Dev/JamKit/araclar/yeni_proje.sh <Ad> "<tek cümle>"` → boş proje + public repo, **~3–5 dk** | Claude |
| 18.50 | Ekibi repoya ekle: `gh api -X PUT repos/Andacs/<Ad>/collaborators/<kullanıcı>` (her biri daveti kabul eder). Bilinen: **CezeryeF** (ikinci bilgisayar, kurulum 16 Eki TAMAM); Mert ve Asil'inki sorulacak | Claude + ekip |
| 18.50 | `CLAUDE.md`: kapsam, **kesme listesi**, sahiplik; `README.md`'nin ilk iki satırı | Andaç + Yiğit |
| 19.00 | Modelcilere liste: isim, ölçü, pivot, format (`08_ASSET_SPEC.md` bu oyuna uyarlanır) | Claude → Mert, Asil |

**`yeni_proje.sh` ne yapar** (15 Ekim gecesi üç kez denendi, her seferinde 3 dk 18 sn; boş projenin
paketi 277 sn sürdü ve ZIP'ten açıldı):

- Boş UE 5.8 C++ projesi; `.uproject`'te XGE ve AndroidFileServer kapalı
- `L_Ana` ve `L_Test` haritaları (ışık, gök, sis)
- `.gitignore`, `.gitattributes` (LFS + satır sonları), `Build/*.bat`, `Tools/*` (teslim, trailer,
  ekran görüntüsü), `Docs/JamKit/` (bu kitin kopyası), `README.md` ve `CLAUDE.md` şablonları
- İlk derleme, ilk commit, **public** GitHub reposu, açıklama ve konular (FreakLabs, civil-game-jam)

Oyun kodu içermez (madde 3.1: genel araçlar ön hazırlık olabilir).

---

## Sonraki saatler (ayrıntı `01_48SAAT_PLANI.md`)

| Ne zaman | Hedef |
|---|---|
| Cuma gece (saat 3–10) | Çekirdek mekanik, kutularla. **Ekranda görülmeyen şey yok sayılır** |
| Cumartesi sabah (saat 10–12) | Ekip oynar: eğlenceli mi? Değilse şimdi değiştir |
| Cumartesi | Kazanma / kaybetme / yeniden başlama → asset geçişi → arayüz yönü (2–3 seçenek, biri seçilir) |
| **Cumartesi akşam** | **İlk tam teslim provası:** `bash Tools/teslim_hazirla.sh` |
| **Pazar 06.00 (saat 36)** | **Özellik dondurma** |
| Pazar 12.00 | Git desteği son saati. Hata avı |
| Pazar 14.00–16.00 | Trailer, `teslim_hazirla.sh` → BOZUK: 0, temiz bilgisayarda deneme |
| **Pazar 16.00** | Forma yükle; makbuzu `TESLIM_OZET.txt`'ye yaz. **Son: 18.00, yükleme kaydı o saatten önce** |

Teslim günü sırası ve form alanları: `10_TESLIM.md`.

---

## Hatırlatmalar

- **Sallantı'dan oyun kodu taşınmaz** (3.1). Oradan alınabilen: araçlar (zaten `yeni_proje.sh` kuruyor),
  ve dersler (`11_PROVA_DERSLERI.md`). Benzer bir sistem gerekirse yeniden yazılır.
- **Kaynak ve AI kullanımı eklendiği anda README'deki iki tabloya.** Sonradan hatırlanmıyor.
- **Her commit push edilir.** Repo public kalır.
- **Model:** mekanik, hata ayıklama ve tasarım kararlarında Opus; belge güncelleme, README, betik çalıştırma,
  rutin düzeltmede Sonnet (kullanım limitini korur). Her büyük iş bitince yeni oturum: Claude `CLAUDE.md`'den
  devam eder; uzun oturum bağlamı şişirir.
- **Limit biterse:** ekip arkadaşı **kendi** Claude hesabıyla Andaç'ın bilgisayarında devam eder (hesap
  paylaşılmaz). Devretmeden önce `CLAUDE.md` → "Bilinen durum" güncellenir, commit + push.
- **İkinci bilgisayar:** arkadaşın kurulumu `12_KURULUM_YENI_BILGISAYAR.md` ile yapılıyor; raporu
  "SONUC: KURULUM TAMAM" olmalı.
- Ekran görüntüsü yalnızca oyun penceresinden (`Tools/pencere_goruntu.ps1`), masaüstünden asla.
