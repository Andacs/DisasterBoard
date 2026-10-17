# Düz beyaz zeminli çizimleri şeffaf PNG'ye çevirir (görüntü üreticiler şeffaf zemin vermiyor).
# Kenara değen beyaz bölge silinir; çizimin içindeki beyazlar (bez, maske, göz akı) kalır. Kenarda beyaz hale
# kalmasın diye geçiş pikselleri beyazdan arındırılır.
# Kullanım: python Tools/zemin_temizle.py <girdi klasörü> <çıktı klasörü>
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage


def temizle(yol, cikti):
    rgb = np.asarray(Image.open(yol).convert("RGB")).astype(np.float32)
    mn = rgb.min(axis=2)
    mx = rgb.max(axis=2)
    beyazimsi = (mn > 226) & (mx - mn < 30)
    etiket, _ = ndimage.label(beyazimsi)
    kenar = np.unique(np.concatenate([etiket[0], etiket[-1], etiket[:, 0], etiket[:, -1]]))
    zemin = np.isin(etiket, kenar[kenar > 0])
    # Geçiş bandı: zemine 2 piksel yakın olanların alfası beyazlığa göre
    yakin = ndimage.binary_dilation(zemin, iterations=2) & ~zemin
    alfa = np.where(zemin, 0.0, 1.0)
    a_band = np.clip((255.0 - mn) / (255.0 - 150.0), 0.0, 1.0)
    alfa = np.where(yakin, a_band, alfa)
    # Beyazdan arındır: c = a*c' + (1-a)*255  ->  c' = (c - (1-a)*255) / a
    a3 = np.maximum(alfa[..., None], 1e-3)
    renk = np.clip((rgb - (1.0 - a3) * 255.0) / a3, 0, 255)
    out = np.dstack([renk, alfa[..., None] * 255.0]).astype(np.uint8)
    Image.fromarray(out, "RGBA").save(cikti)


if __name__ == "__main__":
    girdi, cikti_kl = sys.argv[1], sys.argv[2]
    os.makedirs(cikti_kl, exist_ok=True)
    for ad in sorted(os.listdir(girdi)):
        if ad.lower().endswith(".png"):
            temizle(os.path.join(girdi, ad), os.path.join(cikti_kl, ad))
            print("şeffaf:", ad)
