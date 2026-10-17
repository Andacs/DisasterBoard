# 2D şablonlar (tam piksel ölçüsünde, hitbox çerçeveli, adı yazılı PNG) ve 3D ölçü tablosu.
#   python Tools/sablon_2d.py <çıktı klasörü>
# Ölçüler Tools/sablon_listesi.py'den. Şablonlar Content/ altına KONMAZ (oyun gerçek görsel sanar); tasarımcı
# şablonu açar, üstüne çizer, aynı adla Content/<yol> altına koyar.
import os, sys
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sablon_listesi import ASSETS_2D, ASSETS_3D

OUT = sys.argv[1] if len(sys.argv) > 1 else "Sablonlar"
FONT = r"C:\Windows\Fonts\segoeui.ttf"
FONTB = r"C:\Windows\Fonts\segoeuib.ttf"

def font(sz, bold=False):
    return ImageFont.truetype(FONTB if bold else FONT, max(8, int(sz)))

def wrap(d, text, f, maxw):
    words, lines, cur = text.split(), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if d.textlength(t, font=f) > maxw and cur:
            lines.append(cur); cur = w
        else:
            cur = t
    if cur: lines.append(cur)
    return lines

def dashed_rect(d, box, col, dash=8, w=2):
    x0, y0, x1, y1 = box
    for x in range(int(x0), int(x1), dash * 2):
        d.line([(x, y0), (min(x + dash, x1), y0)], fill=col, width=w); d.line([(x, y1), (min(x + dash, x1), y1)], fill=col, width=w)
    for y in range(int(y0), int(y1), dash * 2):
        d.line([(x0, y), (x0, min(y + dash, y1))], fill=col, width=w); d.line([(x1, y), (x1, min(y + dash, y1))], fill=col, width=w)

def template_2d(path, W, H, cat, note, mark):
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Açık dama zemin: şablon olduğu belli olsun (tasarımcı bu katmanı silip kendi çizimini koyar)
    c = max(8, min(W, H) // 16)
    for y in range(0, H, c):
        for x in range(0, W, c):
            d.rectangle([x, y, x + c - 1, y + c - 1], fill=(236, 236, 240, 255) if (x // c + y // c) % 2 else (248, 248, 250, 255))
    red = (214, 60, 50, 255)
    # Hitbox: görselin sınırı (bu dikdörtgenin dışına taşan çizilmez)
    d.rectangle([0, 0, W - 1, H - 1], outline=red, width=max(2, min(W, H) // 64))
    m = int(min(W, H) * 0.06)
    dashed_rect(d, (m, m, W - 1 - m, H - 1 - m), (90, 140, 200, 255), dash=max(4, min(W, H) // 30))
    if mark == "daire":
        d.ellipse([m, m, W - 1 - m, H - 1 - m], outline=(90, 140, 200, 255), width=2)
    if mark in ("ayak", "el"):
        d.line([(W * 0.2, H - 4), (W * 0.8, H - 4)], fill=(60, 160, 80, 255), width=4)
        d.ellipse([W / 2 - 7, H - 14, W / 2 + 7, H], fill=(60, 160, 80, 255))
        d.text((W / 2, H - 18), "pivot: ayaklar", font=font(13), fill=(60, 120, 60, 255), anchor="ms")
    if mark == "el":
        # Gösterme pozunda elin yeri: oyun noktalı izi buradan başlatır (çerçevenin %90 sağı, %45 aşağısı)
        hx, hy, hr = W * 0.90, H * 0.45, 24
        d.ellipse([hx - hr, hy - hr, hx + hr, hy + hr], outline=(60, 160, 80, 255), width=4)
        d.text((hx - hr - 4, hy - 8), "el", font=font(16, True), fill=(60, 120, 60, 255), anchor="rs")
    if mark == "omuz":
        d.ellipse([0, H / 2 - 7, 14, H / 2 + 7], fill=(60, 160, 80, 255))
        d.line([(16, H / 2), (W - 22, H / 2)], fill=(60, 160, 80, 255), width=3)
        d.polygon([(W - 8, H / 2), (W - 24, H / 2 - 9), (W - 24, H / 2 + 9)], fill=(60, 160, 80, 255))
        d.text((18, H / 2 + 8), "omuz (pivot)", font=font(12), fill=(60, 120, 60, 255))
        d.text((W - 10, H / 2 + 8), "parmak", font=font(12), fill=(60, 120, 60, 255), anchor="ra")
    if mark == "altyazi":
        cap = (40, H - 40 - 130, 40 + 960, H - 40)
        d.rectangle(cap, fill=(252, 238, 160, 200), outline=(180, 150, 60, 255), width=2)
        d.text((cap[0] + 16, cap[1] + 14), "ALTYAZI ALANI: oyun buraya yazı koyar, önemli bir şey çizme", font=font(24), fill=(120, 90, 30, 255))
    name = os.path.basename(path)
    title_sz = max(9, min(W // 12, H // 6, 44))
    f1, f2 = font(title_sz, True), font(max(8, title_sz * 0.6))
    lines = [(name, f1, (40, 30, 24, 255)), (f"{W} x {H} px · {cat}", f2, (120, 80, 50, 255))]
    if min(W, H) >= 160:
        for l in wrap(d, note, f2, W - 2 * m - 16):
            lines.append((l, f2, (80, 70, 60, 255)))
    total = sum(f.size * 1.25 for _, f, _ in lines)
    y = (H - total) / 2 if mark not in ("ayak", "el", "altyazi") else m + 10
    for text, f, col in lines:
        d.text((W / 2, y), text, font=f, fill=col, anchor="ma")
        y += f.size * 1.25
    os.makedirs(os.path.dirname(os.path.join(OUT, "2D", path)), exist_ok=True)
    im.save(os.path.join(OUT, "2D", path))

for path, W, H, cat, note, mark in ASSETS_2D:
    template_2d(path, W, H, cat, note, mark)

# ---- 3D ölçü tablosu: her model önden, gerçek oranında, adı ve ölçüsü yazılı
def sheet(items, scale, title, fname):
    pad, gap = 60, 40
    widths = [max(w * scale, 150) for _, w, d, h, *_ in items]
    W = int(sum(widths) + gap * (len(items) - 1) + 2 * pad)
    Hmax = max(h for _, w, d, h, *_ in items) * scale
    H = int(Hmax + 330)
    im = Image.new("RGB", (W, H), (250, 246, 238))
    dr = ImageDraw.Draw(im)
    dr.text((pad, 24), title, font=font(30, True), fill=(50, 40, 30))
    base = H - 170
    dr.line([(pad - 20, base), (W - pad + 20, base)], fill=(140, 110, 80), width=3)
    x = pad
    for (name, w, d, h, cat, note), slot in zip(items, widths):
        cx = x + slot / 2
        bw, bh = w * scale, h * scale
        dr.rectangle([cx - bw / 2, base - bh, cx + bw / 2, base], fill=(236, 150, 124) if cat == "Bina" else (150, 200, 196), outline=(214, 60, 50), width=3)
        dr.ellipse([cx - 6, base - 6, cx + 6, base + 6], fill=(60, 160, 80))
        dr.text((cx, base - bh - 10), f"{h:g} cm", font=font(18), fill=(50, 40, 30), anchor="ms")
        dr.text((cx, base + 14), name, font=font(17, True), fill=(50, 40, 30), anchor="ma")
        dr.text((cx, base + 40), f"{w:g} x {d:g} x {h:g} cm", font=font(16), fill=(110, 80, 50), anchor="ma")
        x += slot + gap
    dr.text((pad, H - 70), "Kırmızı kutu = hitbox (model bunun içinde kalır). Yeşil nokta = pivot (taban ortası). Önden görünüş, ölçekli.",
            font=font(18), fill=(90, 80, 70))
    os.makedirs(os.path.join(OUT, "3D"), exist_ok=True)
    im.save(os.path.join(OUT, "3D", fname))

sheet([a for a in ASSETS_3D if a[4] == "Bina"], 10, "Binalar: taban 14.6 x 14.6 cm (2x2 karo), ölçek 1 cm = 10 px", "olcu_tablosu_1_binalar.png")
sheet([a for a in ASSETS_3D if a[4] in ("Nesne", "Figür")], 30, "Nesneler ve figürler: ölçek 1 cm = 30 px", "olcu_tablosu_2_nesneler.png")
sheet([a for a in ASSETS_3D if a[4] == "Sahne"], 4, "Sahne: ölçek 1 cm = 4 px", "olcu_tablosu_3_sahne.png")
print("SABLON2D TAMAM", len(ASSETS_2D))
