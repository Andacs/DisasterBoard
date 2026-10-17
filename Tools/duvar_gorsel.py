# Yan duvar için jam afişi ve ekip logosu (kendi üretimimiz; resmi dosya gelince aynı adla değiştirilir)
import math
from PIL import Image, ImageDraw, ImageFont

ROOT = r"C:/Dev/DisasterBoard/Content"
BOLD = ROOT + "/Fonts/Arayuz_Kalin.ttf"
MED = ROOT + "/Fonts/Arayuz.ttf"
INK = (58, 42, 30)
PAPER = (250, 243, 228)
CORAL = (226, 118, 94)
TEAL = (70, 152, 146)
GOLD = (246, 196, 64)
GREEN = (120, 180, 110)
SKY = (132, 176, 212)


def font(path, px):
    return ImageFont.truetype(path, px)


def center_text(d, y, text, f, fill, w):
    b = d.textbbox((0, 0), text, font=f)
    d.text(((w - (b[2] - b[0])) / 2 - b[0], y), text, font=f, fill=fill)


def poster():
    W, H = 710, 1000
    im = Image.new("RGB", (W, H), (248, 240, 224))
    d = ImageDraw.Draw(im)
    # Üst şerit + güneş
    d.rectangle([0, 0, W, 330], fill=CORAL)
    d.ellipse([W - 155, 50, W - 45, 160], fill=GOLD)
    for k in range(12):
        a = k * math.pi / 6
        cx, cy = W - 100, 105
        d.line([cx + math.cos(a) * 68, cy + math.sin(a) * 68, cx + math.cos(a) * 90, cy + math.sin(a) * 90], fill=GOLD, width=8)
    center_text(d, 40, "CIVIL", font(BOLD, 120), PAPER, W - 170)
    center_text(d, 160, "GAME JAM", font(BOLD, 92), PAPER, W - 170)
    # 2026 rozeti
    d.rounded_rectangle([170, 290, W - 170, 390], radius=40, fill=INK)
    center_text(d, 288, "2026", font(BOLD, 86), GOLD, W)
    # Karton kasaba
    base = 700
    houses = [(60, 150, 170, TEAL, (90, 70, 60)), (190, 120, 230, GOLD, CORAL), (330, 160, 140, SKY, (90, 70, 60)),
              (470, 130, 200, GREEN, CORAL), (580, 100, 120, (172, 152, 204), (90, 70, 60))]
    for x, w, h, body, roof in houses:
        d.rectangle([x, base - h, x + w - 20, base], fill=body, outline=INK, width=6)
        d.polygon([(x - 12, base - h), (x + w - 8, base - h), (x + (w - 20) / 2, base - h - 70)], fill=roof, outline=INK)
        for wy in range(base - h + 30, base - 50, 60):
            d.rectangle([x + 22, wy, x + 52, wy + 30], fill=(72, 92, 124))
        d.rectangle([x + (w - 20) / 2 - 15, base - 60, x + (w - 20) / 2 + 15, base], fill=(120, 84, 56))
    d.rectangle([0, base, W, base + 18], fill=(196, 154, 108))
    # Dalgalar
    for row, col in ((0, (110, 174, 214)), (1, (96, 160, 204))):
        y0 = base + 18 + row * 26
        pts = [(x, y0 + 10 + 10 * math.sin(x / 30 + row)) for x in range(0, W + 10, 10)]
        d.polygon(pts + [(W, y0 + 60), (0, y0 + 60)], fill=col)
    # Tema
    d.rectangle([0, 790, W, H], fill=PAPER)
    center_text(d, 800, "Doğal afetlere hazırlık", font(BOLD, 56), INK, W)
    center_text(d, 876, "ve farkındalık", font(MED, 44), (140, 100, 66), W)
    center_text(d, 938, "48 SAAT  ·  EKİM 2026", font(BOLD, 34), CORAL, W)
    d.rectangle([0, 0, W - 1, H - 1], outline=INK, width=8)
    im.save(ROOT + "/Arayuz/duvar_afis.png")


def logo():
    S = 512
    im = Image.new("RGB", (S, S), (248, 240, 224))
    d = ImageDraw.Draw(im)
    d.ellipse([28, 28, S - 28, S - 28], fill=TEAL, outline=INK, width=12)
    d.ellipse([62, 62, S - 62, S - 62], outline=PAPER, width=6)
    # Deney şişesi (köpüren)
    cx = S / 2
    d.rectangle([cx - 26, 110, cx + 26, 170], fill=PAPER, outline=INK, width=8)
    d.polygon([(cx - 26, 166), (cx + 26, 166), (cx + 84, 280), (cx - 84, 280)], fill=PAPER, outline=INK)
    d.polygon([(cx - 60, 232), (cx + 60, 232), (cx + 82, 277), (cx - 82, 277)], fill=CORAL)
    d.line([(cx - 26, 166), (cx - 84, 280), (cx + 84, 280), (cx + 26, 166)], fill=INK, width=8)
    d.rectangle([cx - 36, 98, cx + 36, 114], fill=INK)
    for bx, by, r in ((cx - 18, 80, 12), (cx + 16, 60, 9), (cx - 2, 38, 6)):
        d.ellipse([bx - r, by - r, bx + r, by + r], fill=GOLD, outline=INK, width=4)
    center_text(d, 288, "FREAK", font(BOLD, 72), PAPER, S)
    center_text(d, 372, "LABS", font(BOLD, 46), GOLD, S)
    im.save(ROOT + "/Arayuz/duvar_logo.png")


if __name__ == "__main__":
    import os
    os.makedirs(ROOT + "/Arayuz", exist_ok=True)
    poster()
    logo()
    print("tamam")
