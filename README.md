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
| Görsel | Kullanıldı | Unreal Engine temel şekilleri (`/Engine/BasicShapes`) · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oda, masa, tahta, binalar ve takvim koddan bu şekillerle kurulur (`Source/DisasterBoard/DBWorld.cpp`). **Low Poly Building** · A.S.D. ([arvindsudarshan53](https://sketchfab.com/arvindsudarshan53)), Sketchfab · [model sayfası](https://sketchfab.com/3d-models/low-poly-building-f0722265cf8d4a41ba78ab1587d18ac8) · [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) · tahtadaki kasaba binası modeli; değişiklik: ölçek, pivot ve yön oyunun bina şablonuna göre ayarlandı. |
| Ses | Kullanılmadı | Hazır ses ya da müzik yok. Müzik ve ses efektleri oyun içinde koddan üretiliyor (`Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp`); bkz. yapay zekâ beyanı. |
| Kod | Kullanıldı | **Unreal Engine 5.8** · Epic Games · [Unreal Engine EULA](https://www.unrealengine.com/eula) · oyun motoru. Derleme, paketleme, trailer ve teslim betikleri (`Build/`, `Tools/`) takımın jam öncesi hazırladığı genel araçlardır (madde 3.1); oyuna girmez. |
| Metin | Kullanıldı | **Claude (Anthropic)** · oyun içi konuşmaların, sorun notlarının ve sonuç ekranındaki hazırlık bilgilerinin ilk taslağı · `Source/DisasterBoard/DBData.cpp` |
| Diğer | [...] | [Yazı tipleri: ad · sağlayıcı · Google Fonts bağlantısı · SIL OFL 1.1 (lisans metni `Art/Fonts/*/OFL.txt`) · nerede kullanıldı. Roboto · Google · Apache 2.0 · motorla gelir, yedek yazı tipi.] |

[Repoda bulunmayan dosyalar: yeniden dağıtımı kısıtlı ham asset varsa adı, nereden edinileceği, projeye
nasıl ekleneceği. Yoksa: "Repoda yeniden dağıtımı kısıtlı ham asset yoktur."]

## Yapay zekâ beyanı (kitapçık 4.2, EK A.5)

Her satır: araç · amaç · oyundaki yeri. Çok küçük kullanım da yazılır. Hazır kaynaktan ayrı.

| Kategori | Durum | Araç · amaç · oyundaki yeri |
|---|---|---|
| Görsel | [...] | [...] |
| Ses | Kullanıldı | **Claude (Anthropic)** · müziği ve ses efektlerini (tık, rüzgar, sayfa, tahta, su, doğru/yanlış) oyun içinde üreten kodun yazımı; ses dosyası yok · `Source/DisasterBoard/DBMusic.cpp`, `DBSfx.cpp` |
| Kod | Kullanıldı | **Claude (Anthropic, Claude Code)** · oyun kodunun yazımı (C++: akış, arayüz, sahne kurulumu, sis/kum efektleri, koddan müzik üreteci, otomatik test) · `Source/DisasterBoard/` |
| Metin | [...] | [...] |
| Diğer | [...] | [...] |

## Hak sahipliği

Oyun ve özgün içerik üzerindeki haklar ekip üyelerindedir. Kitapçık madde 9 gereği organizasyona ticari
olmayan, münhasır olmayan kullanım izni verilir.
