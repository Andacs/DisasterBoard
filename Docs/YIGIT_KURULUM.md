# Yiğit: görsel ve model onayı (kurulum)

Mert ve Asil'den gelen çizimleri (PNG) ve modelleri (FBX) kendi bilgisayarında oyunla deneyip onaylayacaksın.
Kontrolü Claude yapar; sen dosyayı atarsın, o yerleştirir, oyunda dener, sorun varsa nedenini söyler.

## Bir kere kur (~20 dk)

1. **GitHub daveti:** github.com/Andacs/DisasterBoard/invitations adresinden kabul et (e-postana da geldi).
2. **Programlar:**
   - Git (Git LFS dahil gelir): https://git-scm.com
   - Python 3: https://python.org (kurarken "Add Python to PATH" işaretli olsun). Sonra komut satırında:
     `python -m pip install pillow`
   - Blender (4.x ya da 5.x): https://blender.org (3D modelleri ölçmek için)
   - Claude Code (masaüstü uygulaması ya da `claude` komutu)
3. **Repoyu indir:**
   ```
   git clone https://github.com/Andacs/DisasterBoard.git
   ```
4. **Claude'u repo klasöründe aç** ve yaz: `kurulumu kontrol et`. Eksikleri söyler; oyunun test paketini
   (361 MB) kendisi indirip `Build\TestKiti\` altına açar. Unreal kurman ya da derlemen gerekmez.
   (Elle indirmek istersen: https://github.com/Andacs/DisasterBoard/releases/tag/test-kiti-1)

## Her gün

- Çizer/modelci sana dosya gönderince (fotoğraf olarak değil, **dosya/belge** olarak; yoksa şeffaflık kaybolur)
  dosyayı Claude'a sürükle: `bunu kontrol et`.
  - Görseller için oyun kendiliğinden açılır, ~1 dk oynar, kapanır. Pencereye dokunma.
  - Rehber karakterler (Mert = A, Asil = B, Yiğit = C) **iki poz** gelir: duruş + kolu kalkık. İkisini birlikte at.
- Claude "RED" derse sebebini çizere aynen ilet; düzeltip tekrar göndersin.
- `kaç tane kaldı` → onaylı / bekleyen listesi (toplam 85: 53 görsel, 32 model).
- Hepsi bitince (ya da Andaç isterse) `teslim et` → Claude onaylıları repoya gönderir (önce sana sorar).

Ölçüler ve dosya adları: `Docs/Sablonlar/OKU_BENI.md` (çizerlere şablon ZIP'iyle birlikte gönder).
