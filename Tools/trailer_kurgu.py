#!/usr/bin/env python
"""Tetkik tanitim videosunun kurgusu (oyuna girmez).

Girdi:
  - ham oyun kareleri: DBSinema -DBSinemaHam (Tools/trailer_yap.sh HAM=1) -> Saved/Screenshots/WindowsEditor/REC_f_#####.png
  - sahne baslari: oyun log'undaki "DB_SAHNE <ad> kare=N" satirlari
  - Asil'in cizgi romani: Content/Cizgiroman/sahne1..4.png, logo: Art/Kaynak/Cizgiroman/sahne2_logo.jpg (kutu)
  - oyunun koddan muzigi: Saved/DB_muzik_trailer.wav; font: Content/Fonts/Arayuz_Kalin.ttf (Baloo 2 Bold, OFL)
Cikti: 1920x1080 30 fps MP4 (<= 60 sn): kisa kesitler, buyuk hareketli yazilar, whip / zoom gecisleri,
punch-in kamera, koddan uretilen whoosh / vurus sesleri.

  python Tools/trailer_kurgu.py --log Saved/Logs/ham_run.log --out Build/Teslim/DisasterBoard_Trailer.mp4
  (--onizle 12.5: tek kare PNG; --bas/--son: yalniz bir aralik)
"""
import argparse
import math
import os
import re
import subprocess
import sys
import wave

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H, FPS = 1920, 1080, 30
INK = (40, 28, 20)
GOLD = (246, 196, 64)
CREAM = (255, 246, 228)


# ---------------------------------------------------------------- yardimcilar

def clamp(x, a=0.0, b=1.0):
    return a if x < a else b if x > b else x


def smooth(x):
    x = clamp(x)
    return x * x * (3 - 2 * x)


def ease_out_cubic(x):
    x = clamp(x)
    return 1 - (1 - x) ** 3


def ease_out_back(x, s=1.9):
    x = clamp(x)
    return 1 + (s + 1) * (x - 1) ** 3 + s * (x - 1) ** 2


def ease_in_cubic(x):
    x = clamp(x)
    return x ** 3


FONT_PATH = os.path.join(ROOT, 'Content', 'Fonts', 'Arayuz_Kalin.ttf')
_fonts = {}


def font(px):
    if px not in _fonts:
        _fonts[px] = ImageFont.truetype(FONT_PATH, px)
    return _fonts[px]


def text_layer(text, px, fill, stroke=0, stroke_fill=INK, shadow=0):
    """Metni siki kirpilmis RGBA katman olarak verir (golge istege bagli)."""
    f = font(px)
    l, t, r, b = f.getbbox(text, stroke_width=stroke)
    pad = 6 + shadow
    im = Image.new('RGBA', (r - l + pad * 2, b - t + pad * 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    if shadow:
        sh = Image.new('RGBA', im.size, (0, 0, 0, 0))
        ImageDraw.Draw(sh).text((pad - l + shadow, pad - t + shadow), text, font=f, fill=(0, 0, 0, 150),
                                stroke_width=stroke, stroke_fill=(0, 0, 0, 150))
        im.alpha_composite(sh.filter(ImageFilter.GaussianBlur(shadow * 0.5)))
    d.text((pad - l, pad - t), text, font=f, fill=fill, stroke_width=stroke, stroke_fill=stroke_fill)
    return im


def rounded(size, radius, fill, outline=None, width=0):
    im = Image.new('RGBA', size, (0, 0, 0, 0))
    ImageDraw.Draw(im).rounded_rectangle((0, 0, size[0] - 1, size[1] - 1), radius, fill=fill, outline=outline,
                                         width=width)
    return im


def paste_center(base, layer, cx, cy, scale=1.0, alpha=1.0, angle=0.0):
    """RGBA katmani (cx, cy) merkezine olcek / aci / saydamlikla yapistirir."""
    if alpha <= 0.003 or scale <= 0.01:
        return
    lay = layer
    if abs(scale - 1.0) > 1e-3:
        lay = lay.resize((max(1, int(lay.width * scale)), max(1, int(lay.height * scale))), Image.BICUBIC)
    if abs(angle) > 0.01:
        lay = lay.rotate(angle, Image.BICUBIC, expand=True)
    if alpha < 0.999:
        a = lay.getchannel('A').point(lambda v: int(v * alpha))
        lay = lay.copy()
        lay.putalpha(a)
    base.alpha_composite(lay, (int(cx - lay.width / 2), int(cy - lay.height / 2)))


def zoom_view(img, z, fx, fy):
    """Kaynaktan z kat yakin, (fx, fy) odakli 1920x1080 goruntu (Ken Burns / punch-in)."""
    sw, sh = img.size
    cw, ch = sw / z, sh / z
    cx = clamp(fx * sw, cw / 2, sw - cw / 2)
    cy = clamp(fy * sh, ch / 2, sh - ch / 2)
    box = (cx - cw / 2, cy - ch / 2, cx + cw / 2, cy + ch / 2)
    if abs(z - 1) < 1e-3 and img.size == (W, H):
        return img.copy()
    return img.resize((W, H), Image.BICUBIC, box=box)


def vignette_mask():
    y, x = np.mgrid[0:H, 0:W].astype(np.float32)
    r = np.sqrt(((x - W / 2) / (W / 2)) ** 2 + ((y - H / 2) / (H / 2)) ** 2)
    return (1.0 - 0.28 * np.clip((r - 0.55) / 0.9, 0, 1) ** 1.6)[..., None]


VIG = None


def hblur(arr, amount):
    """Yatay hareket bulaniklastirmasi (whip gecisi)."""
    n = int(clamp(amount, 0, 1) * 14)
    if n < 1:
        return arr
    k = max(1, int(amount * 90 / 14))
    acc = arr.astype(np.float32)
    for i in range(1, n + 1):
        acc += np.roll(arr, i * k, axis=1)
    return (acc / (n + 1)).astype(np.uint8)


# ---------------------------------------------------------------- kaynaklar

class Rec:
    def __init__(self, folder):
        self.folder = folder
        self.n = len([f for f in os.listdir(folder) if f.startswith('REC_f_')])
        self.cache = {}

    def frame(self, i):
        i = int(clamp(i, 0, self.n - 1))
        if i not in self.cache:
            if len(self.cache) > 12:
                self.cache.pop(next(iter(self.cache)))
            self.cache[i] = Image.open(os.path.join(self.folder, 'REC_f_%05d.png' % i)).convert('RGB')
        return self.cache[i]


def read_marks(log):
    marks = {}
    with open(log, encoding='utf-8', errors='ignore') as f:
        for line in f:
            m = re.search(r'DB_SAHNE (\w+) kare=(\d+)', line)
            if m:
                marks[m.group(1)] = int(m.group(2))
    return marks


def make_logo(src):
    """Asil'in 2. sahnesindeki kutu (TETKiK! + silüet): disari seffaf, murekkep sicak koyu, kagit krem."""
    g = Image.open(src).convert('L')
    x0, y0, x1, y1 = 340, 285, 1225, 905
    c = np.asarray(g.crop((x0, y0, x1, y1))).astype(np.float32)
    ink = c < 170
    lab, _ = ndimage.label(~ndimage.binary_dilation(ink, iterations=2))
    border = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    inside = ndimage.binary_fill_holes(~np.isin(lab, list(border)))
    lab2, n2 = ndimage.label(inside)
    sizes = ndimage.sum(inside, lab2, range(1, n2 + 1))
    box = ndimage.binary_dilation(lab2 == (1 + int(np.argmax(sizes))), iterations=2)
    alpha = ndimage.gaussian_filter(box.astype(np.float32), 0.8)
    t = c[..., None] / 255.0
    rgb = np.array(INK, np.float32) * (1 - t) + np.array(CREAM, np.float32) * t
    return Image.fromarray(np.dstack([rgb, alpha * 255]).clip(0, 255).astype(np.uint8), 'RGBA')


# ---------------------------------------------------------------- arka planlar ve kartlar

def rays_bg(t, base=(247, 186, 92), ray=(255, 226, 150), n=14, cx=W / 2, cy=H * 0.43):
    """Donen gunes isinlari (karton sicakligi)."""
    y, x = np.mgrid[0:H:4, 0:W:4].astype(np.float32)
    ang = np.arctan2(y - cy, x - cx) + t * 0.35
    m = (np.sin(ang * n) > 0).astype(np.float32)
    r = np.sqrt((x - cx) ** 2 + (y - cy) ** 2) / (W / 2)
    m *= np.clip(1.0 - r * 0.5, 0.3, 1)
    rgb = np.array(base, np.float32) * (1 - m[..., None]) + np.array(ray, np.float32) * m[..., None]
    glow = np.exp(-r * r * 3.0)[..., None] * 40
    rgb = rgb + glow
    shade = (1 - 0.30 * np.clip(r - 0.45, 0, 1.2))[..., None]
    im = Image.fromarray((rgb * shade).clip(0, 255).astype(np.uint8), 'RGB').resize((W, H), Image.BILINEAR)
    return im.convert('RGBA')


def logo_with_shadow(logo, scale):
    lw = int(logo.width * scale)
    lg = logo.resize((lw, int(logo.height * scale)), Image.LANCZOS)
    sh = Image.new('RGBA', (lg.width + 80, lg.height + 80), (0, 0, 0, 0))
    a = lg.getchannel('A').point(lambda v: int(v * 0.45))
    blk = Image.new('RGBA', lg.size, (60, 30, 10, 255))
    blk.putalpha(a)
    sh.alpha_composite(blk, (40 + 14, 40 + 22))
    sh = sh.filter(ImageFilter.GaussianBlur(14))
    sh.alpha_composite(lg, (40, 40))
    return sh


# ---------------------------------------------------------------- klipler

class Clip:
    def __init__(self, kind, dur, **kw):
        self.kind, self.dur, self.kw = kind, dur, kw
        self.trans = kw.get('trans', ('cut', 0.0))   # bir sonraki klibe gecis


def render_clip(c, tl, S):
    k = c.kw
    p = tl / c.dur if c.dur > 0 else 0
    if c.kind in ('rec', 'img'):
        z0, z1 = k.get('zoom', (1.0, 1.08))
        f0 = k.get('focus', (0.5, 0.5))
        f1 = k.get('focus2', f0)
        e = smooth(p) if k.get('ease', True) else clamp(p)
        z = z0 + (z1 - z0) * e
        # kesimde punch-in: ilk 0,25 sn biraz daha yakindan yerine oturur
        if k.get('punch', True):
            z *= 1 + 0.10 * (1 - ease_out_cubic(tl / 0.28)) if tl >= 0 else 1.10
        fx = f0[0] + (f1[0] - f0[0]) * e
        fy = f0[1] + (f1[1] - f0[1]) * e
        if c.kind == 'rec':
            src = S['rec'].frame(k['start'] + tl * FPS * k.get('speed', 1.0))
        else:
            src = k['image']
        im = zoom_view(src, z, fx, fy)
        if k.get('shake_at') is not None:
            ds = tl - k['shake_at']
            if 0 <= ds < 0.35:
                a = (1 - ds / 0.35) * 16
                im = im.transform(im.size, Image.AFFINE, (1, 0, math.sin(ds * 90) * a, 0, 1, math.cos(ds * 77) * a),
                                  Image.BICUBIC)
        return im.convert('RGB')
    if c.kind == 'logo':
        return render_logo(tl, S, c)
    if c.kind == 'end':
        return render_end(tl, S)
    raise ValueError(c.kind)


def render_logo(tl, S, c):
    bg = rays_bg(tl + 2)
    slam = clamp(tl / 0.22)
    sc = 2.6 - 1.6 * ease_in_cubic(slam) if slam < 1 else 1.0 + 0.035 * math.sin((tl - 0.22) * 6) * math.exp(-(tl - 0.22) * 3) + 0.02 * (tl - 0.22)
    lay = S['logo_big']
    shake = 0
    if tl > 0.22:
        shake = max(0.0, 1 - (tl - 0.22) / 0.3) * 18
    cx = W / 2 + math.sin(tl * 83) * shake
    cy = H * 0.45 + math.cos(tl * 71) * shake
    paste_center(bg, lay, cx, cy, sc, clamp(tl / 0.08), -3 + 1.5 * (1 - slam))
    # alt satir yukari kayar
    st = clamp((tl - 0.45) / 0.35)
    if st > 0:
        paste_center(bg, S['logo_sub'], W / 2, H * 0.88 + (1 - ease_out_back(st)) * 60, 1.0, st)
    return bg.convert('RGB')


def render_end(tl, S):
    bg = rays_bg(tl + 5)
    a = ease_out_back(clamp(tl / 0.5), 1.4)
    paste_center(bg, S['logo_end'], W / 2, H * 0.34, 0.6 + 0.4 * a + 0.012 * tl, clamp(tl / 0.15), -3)
    t2 = clamp((tl - 0.35) / 0.35)
    if t2 > 0:
        paste_center(bg, S['end_tag'], W / 2, H * 0.70 + (1 - ease_out_back(t2)) * 50, 1.0, t2)
    t3 = clamp((tl - 0.7) / 0.35)
    if t3 > 0:
        puls = 1 + 0.045 * math.sin((tl - 0.7) * 7.5)
        paste_center(bg, S['end_btn'], W / 2, H * 0.825, ease_out_back(t3) * puls, t3)
    t4 = clamp((tl - 1.0) / 0.4)
    if t4 > 0:
        paste_center(bg, S['end_team'], W / 2, H * 0.94, 1.0, t4)
    return bg.convert('RGB')


# ---------------------------------------------------------------- yazilar

def caption_layer(big, small=None, num=None, color=GOLD, px=150):
    """Buyuk baslik: koyu konturlu beyaz harfler, solda numara rozeti, altinda koyu serit ustunde aciklama."""
    tl = text_layer(big, px, (255, 255, 255, 255), stroke=max(8, px // 13), shadow=10)
    parts = [tl]
    badge = None
    if num is not None:
        d = int(px * 0.95)
        badge = Image.new('RGBA', (d + 16, d + 16), (0, 0, 0, 0))
        bd = ImageDraw.Draw(badge)
        bd.ellipse((8, 12, d + 8, d + 12), fill=(0, 0, 0, 90))
        bd.ellipse((4, 4, d + 4, d + 4), fill=color + (255,), outline=INK + (255,), width=max(6, px // 22))
        nl = text_layer(str(num), int(px * 0.62), INK + (255,))
        badge.alpha_composite(nl, (4 + (d - nl.width) // 2, 4 + (d - nl.height) // 2 + 2))
    sl = None
    if small:
        st = text_layer(small, int(px * 0.40), CREAM + (255,))
        sl = rounded((st.width + 56, st.height + 26), (st.height + 26) // 2, INK + (232,), color + (255,), 5)
        sl.alpha_composite(st, (28, 13))
    bw = (badge.width + 18 if badge else 0) + tl.width
    w = max(bw, (sl.width + (badge.width + 18 if badge else 0)) if sl else 0)
    h = max(tl.height, badge.height if badge else 0) + (sl.height - 8 if sl else 0)
    im = Image.new('RGBA', (w + 20, h + 20), (0, 0, 0, 0))
    x = 10
    top = 10
    if badge:
        im.alpha_composite(badge, (x, top + (tl.height - badge.height) // 2))
        x += badge.width + 18
    im.alpha_composite(tl, (x, top))
    if sl:
        im.alpha_composite(sl, (x + 6, top + tl.height - 8))
    return im


class Caption:
    def __init__(self, t0, t1, layer, pos='left', style='slide'):
        self.t0, self.t1, self.layer, self.pos, self.style = t0, t1, layer, pos, style

    def draw(self, base, t):
        if t < self.t0 or t > self.t1:
            return
        tin = t - self.t0
        tout = self.t1 - t
        a_in = clamp(tin / 0.30)
        a_out = clamp(tout / 0.22)
        L = self.layer
        if self.pos == 'left':
            cx, cy = 90 + L.width / 2, H * 0.30
        elif self.pos == 'leftlow':
            cx, cy = 90 + L.width / 2, H * 0.60
        elif self.pos == 'right':
            cx, cy = W - 90 - L.width / 2, H * 0.30
        elif self.pos == 'low':
            cx, cy = W / 2, H * 0.80
        else:
            cx, cy = W / 2, H * 0.5
        if self.style == 'slide':
            dx = -(1 - ease_out_back(a_in, 1.4)) * (cx + L.width / 2 + 40)
            dx += (1 - a_out) * 160
            sc = 0.92 + 0.08 * ease_out_back(a_in) + 0.02 * tin
            paste_center(base, L, cx + dx, cy, sc, min(1.0, a_in * 2, a_out), -3.0)
        else:   # pop: ortada buyukten oturur, titrer
            sc = 1.45 - 0.45 * ease_out_back(clamp(tin / 0.22), 2.2) + 0.03 * tin
            j = (1 - clamp(tin / 0.25)) * 10
            paste_center(base, L, cx + math.sin(tin * 90) * j, cy + math.cos(tin * 70) * j, sc,
                         min(1.0, clamp(tin / 0.08), clamp(tout / 0.12)), -4.0)


# ---------------------------------------------------------------- ses

def synth_audio(music_path, offset, total, events, out):
    with wave.open(music_path) as w:
        sr, ch = w.getframerate(), w.getnchannels()
        raw = np.frombuffer(w.readframes(w.getnframes()), np.int16).astype(np.float32) / 32768
    mus = raw.reshape(-1, ch).mean(1)
    n = int(total * sr)
    m = mus[int(offset * sr):int(offset * sr) + n]
    m = np.pad(m, (0, max(0, n - len(m))))
    env = np.ones(n, np.float32)
    fi, fo = int(0.4 * sr), int(1.8 * sr)
    env[:fi] = np.linspace(0, 1, fi)
    env[-fo:] = np.linspace(1, 0, fo)
    mix = m * env * 0.9
    rng = np.random.default_rng(7)
    for kind, t in events:
        i = int(t * sr)
        if kind == 'whoosh':
            d = int(0.42 * sr)
            x = rng.standard_normal(d).astype(np.float32)
            tt = np.linspace(0, 1, d)
            # bant gecirgen tarama: basit iki kutuplu filtre, frekans yukari kayar
            out_s = np.zeros(d, np.float32)
            y1 = y2 = 0.0
            for j in range(d):
                f = 300 + 3200 * tt[j] ** 1.5
                r = 0.985
                cth = math.cos(2 * math.pi * f / sr)
                y = x[j] * (1 - r) + 2 * r * cth * y1 - r * r * y2
                y2, y1 = y1, y
                out_s[j] = y
            out_s *= np.sin(np.pi * tt) ** 1.5
            out_s /= (np.abs(out_s).max() + 1e-6)
            s, a = out_s * 0.32, i - d // 2
        elif kind == 'thump':
            d = int(0.45 * sr)
            tt = np.arange(d) / sr
            s = (np.sin(2 * np.pi * (95 * tt - 60 * tt * tt)) * np.exp(-tt * 9) * 0.75
                 + rng.standard_normal(d) * np.exp(-tt * 60) * 0.18).astype(np.float32)
            a = i
        elif kind == 'pop':
            d = int(0.16 * sr)
            tt = np.arange(d) / sr
            s = (np.sin(2 * np.pi * (520 + 900 * tt / 0.16) * tt) * np.exp(-tt * 28) * 0.28).astype(np.float32)
            a = i
        elif kind == 'chime':
            d = int(1.2 * sr)
            tt = np.arange(d) / sr
            s = sum(np.sin(2 * np.pi * f * tt) * np.exp(-tt * k) * g
                    for f, k, g in ((1046.5, 3.5, 0.18), (1568, 4.5, 0.12), (2093, 6, 0.08))).astype(np.float32)
            a = i
        else:
            continue
        a0, a1 = max(0, a), min(n, a + len(s))
        if a1 > a0:
            mix[a0:a1] += s[a0 - a:a1 - a]
    peak = np.abs(mix).max()
    if peak > 0.89:
        mix *= 0.89 / peak
    pcm = (mix * 32767).astype(np.int16)
    with wave.open(out, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())


# ---------------------------------------------------------------- kurgu

def build(S, marks):
    M = lambda name, sec=0.0: marks[name] + int(sec * FPS)
    cz = [Image.open(os.path.join(ROOT, 'Content', 'Cizgiroman', 'sahne%d.png' % i)).convert('RGB') for i in range(1, 5)]
    clips = [
        # Hikaye: bes arkadas -> tozlu kutu -> LOGO -> "afet gelmeden..." -> cizilmis oda = oyundaki oda
        Clip('img', 1.0, image=cz[0], zoom=(1.0, 1.12), focus=(0.5, 0.42), punch=False, trans=('cut', 0)),
        Clip('img', 1.25, image=cz[1], zoom=(1.0, 2.1), focus=(0.49, 0.54), ease=False, trans=('flash', 0.2)),
        Clip('logo', 2.3, trans=('whip', 0.3)),
        Clip('img', 1.0, image=cz[2], zoom=(1.05, 1.25), focus=(0.5, 0.45), trans=('cut', 0)),
        Clip('img', 1.3, image=cz[3], zoom=(1.0, 1.06), focus=(0.5, 0.5), trans=('dissolve', 0.55)),
        # Oyun: cizimden gercege, kamera takvimden tahtaya
        Clip('rec', 0.9, start=M('menu', 1.0), zoom=(1.06, 1.0), punch=False, trans=('cut', 0)),
        Clip('rec', 1.9, start=M('oyna', 0.0), speed=1.25, zoom=(1.0, 1.0), punch=False, trans=('whip', 0.3)),
        # 1 TARA
        Clip('rec', 3.4, start=M('tara', 1.9), zoom=(1.0, 1.12), focus=(0.5, 0.55), trans=('whip', 0.3)),
        # 2 HAZIRLA: fabrika paneline yaklas
        Clip('rec', 4.4, start=M('fabrika', 0.0), speed=1.9, zoom=(1.12, 1.25), focus=(0.80, 0.50), focus2=(0.80, 0.56), trans=('zoom', 0.3)),
        # Komsu
        Clip('rec', 2.2, start=M('komsu', 0.3), speed=1.2, zoom=(1.0, 1.1), focus=(0.5, 0.5), trans=('whip', 0.3)),
        # 3 KORU: rogar, somun, musluk, guclendirme hizli kesitler
        Clip('rec', 2.6, start=M('rogar', 0.9), speed=1.8, zoom=(1.08, 1.15), focus=(0.40, 0.5), trans=('cut', 0)),
        Clip('rec', 1.9, start=M('somun', 0.1), speed=1.1, zoom=(1.12, 1.18), focus=(0.36, 0.5), trans=('cut', 0)),
        Clip('rec', 1.6, start=M('musluk', 0.1), speed=1.0, zoom=(1.0, 1.12), focus=(0.5, 0.5), trans=('cut', 0)),
        Clip('rec', 1.8, start=M('guclendir', 0.2), speed=1.3, zoom=(1.0, 1.12), focus=(0.5, 0.5), trans=('whip', 0.3)),
        # Tahtayi cevir
        Clip('rec', 2.4, start=M('dondur', 0.0), speed=1.5, zoom=(1.0, 1.06), focus=(0.5, 0.5), trans=('whip', 0.3)),
        # Tahliye -> album
        Clip('rec', 1.4, start=M('tahliye', 0.1), speed=0.95, zoom=(1.0, 1.12), focus=(0.55, 0.55), trans=('zoom', 0.3)),
        Clip('rec', 3.2, start=M('tahliye', 2.6), speed=1.3, zoom=(1.0, 1.08), focus=(0.5, 0.5), trans=('flash', 0.3)),
        Clip('end', 4.0),
    ]
    return clips


def captions(starts):
    s = starts
    C = []
    C.append(Caption(s[3] + 0.1, s[4] - 0.05, caption_layer('AFET GELMEDEN...', px=150), 'low', 'pop'))
    C.append(Caption(s[4] + 0.1, s[5] + 0.6, caption_layer('KASABANI HAZIRLA!', px=150), 'low', 'pop'))
    words = [('DEPREM', (214, 140, 72)), ('SEL', (72, 150, 226)), ('YANGIN', (236, 92, 60)), ('TSUNAMİ', (52, 178, 186))]
    for i, (wd, col) in enumerate(words):
        t0 = s[6] + 0.15 + i * 0.42
        C.append(Caption(t0, t0 + 0.40, text_layer(wd, 200, col + (255,), stroke=16, shadow=12), 'center', 'pop'))
    C.append(Caption(s[7] + 0.05, s[8] - 0.1, caption_layer('TARA', 'riskli evleri bul', 1), 'left'))
    C.append(Caption(s[8] + 0.05, s[9] - 0.1, caption_layer('HAZIRLA', 'doğru afet çantasını yap', 2, px=140), 'left'))
    C.append(Caption(s[9] + 0.05, s[10] - 0.1, caption_layer('DİNLE', 'komşularının derdi ne?', color=(120, 200, 120)), 'left'))
    C.append(Caption(s[10] + 0.05, s[11] - 0.05, caption_layer('KORU', 'rögarı kapat', 3), 'left'))
    C.append(Caption(s[11], s[12] - 0.05, caption_layer('KORU', 'musluğu onar', 3), 'left', 'pop'))
    C.append(Caption(s[12], s[13] - 0.05, caption_layer('KORU', 'yangın musluğu kur', 3), 'left', 'pop'))
    C.append(Caption(s[13], s[14] - 0.1, caption_layer('KORU', 'binayı güçlendir', 3), 'left', 'pop'))
    C.append(Caption(s[14] + 0.05, s[15] - 0.1, caption_layer('ÇEVİR', 'her köşeye bak', color=(120, 180, 240)), 'left'))
    C.append(Caption(s[15] + 0.05, s[16] - 0.1, caption_layer('TAHLİYE', 'sahili boşalt', color=(52, 178, 186)), 'left'))
    C.append(Caption(s[16] + 0.15, s[17] - 0.05, caption_layer('KASABA HAZIR!', px=150), 'center', 'pop'))
    return C


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--log', default=os.path.join(ROOT, 'Saved', 'Logs', 'ham_run.log'))
    ap.add_argument('--kareler', default=os.path.join(ROOT, 'Saved', 'Screenshots', 'WindowsEditor'))
    ap.add_argument('--muzik', default=os.path.join(ROOT, 'Saved', 'DB_muzik_trailer.wav'))
    ap.add_argument('--muzik-bas', type=float, default=18.0, help='muzigin kacinci saniyesinden baslar (yogunluk artar)')
    ap.add_argument('--logo', default=os.path.join(ROOT, 'Art', 'Kaynak', 'Cizgiroman', 'sahne2_logo.jpg'))
    ap.add_argument('--out', default=os.path.join(ROOT, 'Build', 'Teslim', 'DisasterBoard_Trailer.mp4'))
    ap.add_argument('--onizle', type=float, nargs='*', help='yalniz bu saniyelerin karelerini PNG yazar')
    ap.add_argument('--bas', type=float, default=0.0)
    ap.add_argument('--son', type=float, default=None)
    a = ap.parse_args()

    global VIG
    VIG = vignette_mask()
    marks = read_marks(a.log)
    S = {'rec': Rec(a.kareler)}
    logo = make_logo(a.logo)
    S['logo_big'] = logo_with_shadow(logo, 1.34)
    st = text_layer('afetlere hazır bir kasaba kur', 74, CREAM + (255,))
    sub = rounded((st.width + 80, st.height + 34), (st.height + 34) // 2, INK + (235,), GOLD + (255,), 6)
    sub.alpha_composite(st, (40, 17))
    S['logo_sub'] = sub
    S['logo_end'] = logo_with_shadow(logo, 0.95)
    S['end_tag'] = text_layer('Afet gelmeden kasabanı hazırla.', 76, INK + (255,))
    bt = text_layer('ŞİMDİ OYNA', 64, (255, 255, 255, 255), stroke=5)
    btn = rounded((bt.width + 90, bt.height + 34), (bt.height + 34) // 2, (226, 110, 72, 255), INK + (255,), 6)
    btn.alpha_composite(bt, (45, 17))
    S['end_btn'] = btn
    S['end_team'] = text_layer('FreakLabs  ·  CIVIL GAME JAM 2026', 40, (90, 60, 40, 255))

    clips = build(S, marks)
    starts = [0.0]
    for c in clips:
        starts.append(starts[-1] + c.dur)
    total = starts[-1]
    caps = captions(starts)
    print('sure %.2f sn, %d klip' % (total, len(clips)))
    if total > 60:
        sys.exit('60 sn siniri asildi')

    def clip_at(t):
        for i in range(len(clips)):
            if t < starts[i + 1] or i == len(clips) - 1:
                return i
        return len(clips) - 1

    def compose(t):
        i = clip_at(t)
        im = None
        flash = 0.0
        # gecis: siniri ortalar (onceki klip biraz uzar, sonraki biraz once baslar)
        for b in (i, i + 1):
            if b < 1 or b >= len(clips):
                continue
            kind, d = clips[b - 1].trans
            Tb = starts[b]
            if kind in ('whip', 'zoom', 'dissolve', 'flash') and abs(t - Tb) < d / 2:
                A, B = clips[b - 1], clips[b]
                p = (t - (Tb - d / 2)) / d
                ia = render_clip(A, t - starts[b - 1], S)
                ib = render_clip(B, t - Tb, S)
                if kind == 'whip':
                    e = smooth(p)
                    na, nb = np.asarray(ia), np.asarray(ib)
                    off = int(e * W)
                    o = np.concatenate([na[:, off:], nb[:, :off]], axis=1) if off > 0 else na
                    o = hblur(o, math.sin(math.pi * p))
                    im = Image.fromarray(o)
                elif kind == 'zoom':
                    e = smooth(p)
                    if p < 0.5:
                        im = zoom_view(ia, 1 + e * 0.9, 0.5, 0.5)
                    else:
                        im = zoom_view(ib, 1 + (1 - e) * 0.9, 0.5, 0.5)
                    im = im.filter(ImageFilter.GaussianBlur(math.sin(math.pi * p) * 6))
                elif kind == 'dissolve':
                    im = Image.blend(ia, ib, smooth(p))
                else:
                    im = ia if p < 0.5 else ib
                    flash = max(flash, 1 - abs(p - 0.5) * 2)
                break
        if im is None:
            im = render_clip(clips[i], t - starts[i], S)
        # kesimden hemen sonra kisa beyaz parlama
        for b in range(1, len(clips)):
            if clips[b - 1].trans[0] == 'cut' and 0 <= t - starts[b] < 0.12 and clips[b].kind == 'rec':
                flash = max(flash, 0.35 * (1 - (t - starts[b]) / 0.12))
        arr = np.asarray(im).astype(np.float32)
        if clips[i].kind in ('rec', 'img'):
            arr *= VIG
        if flash > 0:
            arr = arr * (1 - flash) + 255 * flash
        fade = clamp(t / 0.25) * clamp((total - t) / 0.5)
        if fade < 1:
            arr *= fade
        base = Image.fromarray(arr.clip(0, 255).astype(np.uint8)).convert('RGBA')
        for c in caps:
            c.draw(base, t)
        return base.convert('RGB')

    if a.onizle:
        os.makedirs(os.path.join(ROOT, 'Saved', 'TrailerOnizleme'), exist_ok=True)
        for t in a.onizle:
            p = os.path.join(ROOT, 'Saved', 'TrailerOnizleme', 'o_%05.2f.png' % t)
            compose(t).save(p)
            print(p)
        return

    # ses olaylari: whip / zoom gecislerinde whoosh, logoda vurus, kesimlerde ve yazilarda pop, sonda can
    ev = [('thump', starts[2] + 0.2), ('chime', starts[len(clips) - 1] + 0.1)]
    for b in range(1, len(clips)):
        k = clips[b - 1].trans[0]
        if k in ('whip', 'zoom'):
            ev.append(('whoosh', starts[b]))
        elif k == 'flash' and b != 2:
            ev.append(('whoosh', starts[b] - 0.1))
    for c in caps:
        if c.style == 'pop':
            ev.append(('pop', c.t0))
    aud = os.path.join(ROOT, 'Saved', 'trailer_ses.wav')
    synth_audio(a.muzik, a.muzik_bas, total, ev, aud)

    t0 = a.bas
    t1 = min(total, a.son) if a.son else total
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    cmd = ['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-s', '%dx%d' % (W, H),
           '-r', str(FPS), '-i', '-', '-ss', str(t0), '-i', aud, '-t', str(t1 - t0),
           '-c:v', 'libx264', '-preset', 'slow', '-crf', '17', '-pix_fmt', 'yuv420p',
           '-c:a', 'aac', '-b:a', '192k', '-movflags', '+faststart', a.out]
    pr = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    n0, n1 = int(round(t0 * FPS)), int(round(t1 * FPS))
    for n in range(n0, n1):
        pr.stdin.write(compose(n / FPS).tobytes())
        if n % 150 == 0:
            print('kare %d / %d' % (n, n1), flush=True)
    pr.stdin.close()
    pr.wait()
    print('tamam', a.out)


if __name__ == '__main__':
    main()
