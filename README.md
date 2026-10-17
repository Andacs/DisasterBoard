# DisasterBoard

Karton bir masa oyunu kasabasında tarayıcıyla binaların deprem risklerini bulursun. Her eve ihtiyacına göre afet çantası hazırlar, şehir risklerini belediye araçlarıyla giderirsin; deprem gelmeden kasabayı hazırlamaya çalışırsın.

CIVIL GAME JAM 2026 · **FreakLabs**

---

## Çalıştırma (kitapçık EK A.3)

**Platform ve gereksinimler:** Windows 10/11, 64 bit, DirectX 12 destekli ekran kartı. Ek yazılım
gerekmez (gerekirse Unreal Engine önkoşulları paketin içindeki `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe`).

**Başlatma:** ZIP'i bir klasöre açın, `DisasterBoard.exe`'yi çalıştırın. [Açılış videosu / menü varsa: nasıl geçilir.]

**Oynanış:** [Amaç, kazanma ve kaybetme. İlk adım ne.]

| Girdi | Ne yapar |
|---|---|
| [Sol tık] | [...] |
| [Esc] | [...] |

**Bilinen sorunlar:** [Yoksa "Bilinen sorun yok." Varsa: sınırlama ve geçici çözüm.]

## Kaynaktan derleme

Unreal Engine **5.8.0**. Editör kapalıyken `Build\derle.bat`, paket için `Build\paketle.bat`
(Shipping, çıktı `Build\Package\Windows`). Trailer: `bash Tools/trailer_yap.sh`. Teslim kontrolü:
`bash Tools/teslim_hazirla.sh`.

## Hazır kaynak beyanı (kitapçık 3.4, EK A.4)

Her satır: ad · üretici/sağlayıcı · bağlantı · lisans (gerçek koşulları eşleşiyorsa adıyla) · oyundaki yeri
· değişiklik. Kullanılmadıysa "Kullanılmadı". Beş satır da dolu olmalı; `teslim_hazirla.sh` bunu kontrol eder.

| Kategori | Durum | Kaynak · sağlayıcı · lisans · kullanım |
|---|---|---|
| Görsel | Kullanıldı | Unreal Engine temel şekilleri (`/Engine/BasicShapes`) ve motor malzemeleri (`BasicShapeMaterial`, `Widget3DPassThrough`) · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · bütün sahne bunlardan koddan kuruluyor, renkleri kodla değişiyor. 2D çizimler (karakterler, ikonlar) ekibin kendi çizimi (`Art/Kaynak/`); hazır görsel yok. |
| Ses | Kullanılmadı | Hazır ses ya da müzik yok. Müzik ve ses efektleri oyun içinde koddan üretiliyor (`Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp`); bkz. yapay zekâ beyanı. |
| Kod | Kullanıldı | **Unreal Engine 5.8** · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oyun motoru. Derleme, paketleme, trailer ve teslim betikleri (`Build/`, `Tools/`) takımın jam öncesi hazırladığı genel araçlardır (madde 3.1); oyuna girmez. |
| Metin | Kullanıldı | **Claude (Anthropic)** · oyun içi konuşmaların, sorun notlarının ve sonuç ekranındaki hazırlık bilgilerinin ilk taslağı · `Source/DisasterBoard/DBData.cpp` |
| Diğer | Kullanıldı | Yazı tipi **Baloo 2** · Ek Type · [Google Fonts](https://fonts.google.com/specimen/Baloo+2) ([kaynak](https://github.com/google/fonts/tree/main/ofl/baloo2)) · SIL Open Font License 1.1 (lisans metni `Content/Fonts/Arayuz_OFL.txt`) · bütün arayüz ve takvim yazıları. Değişiklik: değişken fonttan fonttools ile sabit Medium (`Arayuz.ttf`) ve Bold (`Arayuz_Kalin.ttf`) kalınlıkları üretildi; harflere dokunulmadı. |

Repoda yeniden dağıtımı kısıtlı ham asset yoktur.

## Yapay zekâ beyanı (kitapçık 4.2, EK A.5)

Her satır: araç · amaç · oyundaki yeri. Çok küçük kullanım da yazılır. Hazır kaynaktan ayrı.

| Kategori | Durum | Araç · amaç · oyundaki yeri |
|---|---|---|
| Görsel | Kullanıldı | **Codex (OpenAI), görüntü üretimi** · ekibin kendi çizimleri stil referansı verilerek 7 eşya ikonu (yara bandı, el feneri, düdük, duman maskesi, gözlük, bebek maması, bebek bezi), 5 şehir önlemi ikonu, tarayıcı, çanta ve 12 kasabalı portresi üretildi; beyaz zemin `Tools/zemin_temizle.py` ile silindi, ölçüye `Tools/varlik_kontrol.py --ikon` ile oturtuldu · `Content/Ikonlar/esya_{yara_bandi,el_feneri,duduk,duman_maskesi,gozluk,bebek_mamasi,bebek_bezi}.png`, `Content/Ikonlar/onlem_*.png`, `Content/Arayuz/{tarayici,canta}.png`, `Content/Karakterler/NPC_*.png` (ham hâlleri `Art/AI_Ham/`). **Claude (Anthropic)** · yan duvardaki jam afişi ve ekip logosunu (yer tutucu) şekillerle çizen Python betiği · `Tools/duvar_gorsel.py` → `Content/Arayuz/duvar_afis.png`, `duvar_logo.png`. Diğer bütün 2D çizimler ekibin kendi elinden. |
| Ses | Kullanıldı | **Claude (Anthropic)** · müziği ve ses efektlerini (tık, rüzgar, sayfa, tahta, su, doğru/yanlış, karakter konuşma heceleri, korna, tarayıcı, bant, çanta fermuarı, başarım, deklanşör) oyun içinde üreten kodun yazımı; ses dosyası yok · `Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp` |
| Kod | Kullanıldı | **Claude (Anthropic, Claude Code)** · oyun kodunun yazımı (C++: akış, arayüz, sahne kurulumu, sis/kum efektleri, koddan müzik üreteci, otomatik test) · `Source/DisasterBoard/` |
| Metin | [...] | [...] |
| Diğer | [...] | [...] |

## Hak sahipliği

Oyun ve özgün içerik üzerindeki haklar ekip üyelerindedir. Kitapçık madde 9 gereği organizasyona ticari
olmayan, münhasır olmayan kullanım izni verilir.
