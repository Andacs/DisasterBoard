# DisasterBoard: modelci ve tasarımcıdan gelen dosyaları kontrol eder, oyunda dener, uygunsa onaylar.
#
#   python Tools/varlik_kontrol.py <dosya veya klasör> ...   kontrol et; sorun yoksa Art/Onayli/ altına koy
#   python Tools/varlik_kontrol.py --durum                   73 dosyanın listesi: onaylı / bekleyen
#   python Tools/varlik_kontrol.py --teslim [--kismi]        onaylıları commit + push (hepsi bitmeden: --kismi)
#   python Tools/varlik_kontrol.py --aktar                   (Andaç) onaylı 2D'leri Content/ altına kopyalar
#   python Tools/varlik_kontrol.py --geri <dosya adı>        bir onayı geri alır (gözle bakınca sorun görüldüyse)
#   python Tools/varlik_kontrol.py --karakter A <duruş> <gösterme>
#                                       rehber karakterin iki pozunu (büyük tuval olabilir) aynı kesimle 320x512'ye
#                                       oturtur (ayaklar alt kenarda), sonra normal kontrolden geçirir
#
# Seçenekler: --oyun <DisasterBoard.exe>  test paketi başka yerdeyse;  --oyunsuz  oyun testini atla (onaylamaz)
#
# 2D (PNG): ad listede mi, piksel ölçüsü, şeffaf arka plan, şablon kalıntısı (kırmızı çerçeve, dama zemin),
#           karakterde ayak/omuz yeri; sonra test paketiyle oyun açılır, görsel yerinde gösterilir, oyunun dosyayı
#           okuduğu rapordan doğrulanır, ekran görüntüleri Art/Onayli/_ekran/ altına konur.
# 3D (FBX): Blender'da açılır: kutuya (hitbox) sığıyor mu, pivot taban ortasında mı, birim cm mi, üçgen sayısı;
#           önizleme resmi Art/Onayli/3D/onizleme/ altına.
# Ölçülerin tek kaynağı Tools/sablon_listesi.py. Talimat (Claude için): .claude/skills/varlik-onay/SKILL.md
import difflib, glob, hashlib, json, os, shutil, subprocess, sys, tempfile, time
from datetime import datetime

sys.stdout.reconfigure(encoding="utf-8")
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from sablon_listesi import teslim_listesi  # noqa: E402

LISTE = teslim_listesi()
ONAYLI = os.path.join(REPO, "Art", "Onayli")
RED = os.path.join(REPO, "Art", "Reddedildi")
DENEME = os.path.join(REPO, "Art", "_deneme")
HAZIR = os.path.join(REPO, "Art", "_hazir")   # --karakter çıktısı (git'e girmez)
DURUM = os.path.join(ONAYLI, "durum.json")
BILGI = "bilgi: "   # uyarı değil, ölçü özeti

# Oyundaki test turunun ekran görüntüleri (DBPlayerController::DBVarlikTest): hangi görsel hangi karede görünür
def ekranlar(yol):
    if yol.startswith("Cizgiroman/sahne"):
        n = yol[len("Cizgiroman/sahne")]
        return [f"0{n}_sahne{n}"]
    tablo = [("Arayuz/logo", "05_menu_logo"), ("Karakterler/A_goster", "06_mert_tarayici"), ("Arayuz/tarayici", "06_mert_tarayici"),
             ("Ikonlar/isaret_uyari", "07_uyari_isaretleri"), ("Karakterler/B_durus", "08a_asil_durus"),
             ("Karakterler/A_durus", "08_asil_fabrika"), ("Karakterler/B_goster", "08_asil_fabrika"), ("Ikonlar/esya_", "08_asil_fabrika"),
             ("Ikonlar/kit_", "08_asil_fabrika"), ("Ikonlar/butce", "08_asil_fabrika"), ("Arayuz/canta", "09_canta_bantta"),
             ("Karakterler/C_durus", "10_yigit_onlemler"), ("Karakterler/C_goster", "10b_yigit_gosteriyor"),
             ("Ikonlar/onlem_", "10_yigit_onlemler"), ("Ikonlar/isaret_tamam", "11_tamam_isareti")]
    return [shot for pre, shot in tablo if yol.startswith(pre)]


def durum_oku():
    if os.path.exists(DURUM):
        with open(DURUM, encoding="utf-8") as f:
            return json.load(f)
    return {}


def durum_yaz(d):
    os.makedirs(ONAYLI, exist_ok=True)
    with open(DURUM, "w", encoding="utf-8") as f:
        json.dump(d, f, ensure_ascii=False, indent=1, sort_keys=True)
    rapor_yaz(d)


def rapor_yaz(d):
    n2 = sum(1 for k, v in LISTE.items() if v["tur"] == "2D")
    o2 = sum(1 for k, v in LISTE.items() if v["tur"] == "2D" and k in d)
    o3 = sum(1 for k, v in LISTE.items() if v["tur"] == "3D" and k in d)
    satir = [f"# Onaylı varlıklar ({o2 + o3} / {len(LISTE)})", "",
             f"2D: {o2} / {n2} · 3D: {o3} / {len(LISTE) - n2}. Bu dosyayı `Tools/varlik_kontrol.py` yazar, elle değiştirme.", "",
             "| Dosya | Tür | Durum | Tarih | Not |", "|---|---|---|---|---|"]
    for k, v in LISTE.items():
        if k in d:
            u = "; ".join(x for x in d[k].get("uyarilar", []) if not x.startswith(BILGI)) or "-"
            satir.append(f"| `{k}` | {v['tur']} | ONAYLI | {d[k]['tarih']} | {u} |")
        else:
            satir.append(f"| `{k}` | {v['tur']} | bekliyor | | |")
    with open(os.path.join(ONAYLI, "RAPOR.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(satir) + "\n")


def eslestir(dosya):
    """Dosya adından listedeki anahtar; yoksa (None, öneri)"""
    ad = os.path.basename(dosya)
    kok, uz = os.path.splitext(ad)
    uz = uz.lower()
    if uz == ".png":
        for k, v in LISTE.items():
            if v["tur"] == "2D" and os.path.basename(k).lower() == ad.lower():
                return k, None
        adaylar = [os.path.basename(k) for k, v in LISTE.items() if v["tur"] == "2D"]
    elif uz == ".fbx":
        for k, v in LISTE.items():
            if v["tur"] == "3D" and k.lower() == kok.lower():
                return k, None
        adaylar = [k + ".fbx" for k, v in LISTE.items() if v["tur"] == "3D"]
    else:
        return None, "Yalnızca .png (2D) ve .fbx (3D) kabul ediliyor."
    yakin = difflib.get_close_matches(ad, adaylar, n=1, cutoff=0.5)
    oneri = f" En yakın ad: {yakin[0]} (doğruysa dosyayı bu adla kaydet)." if yakin else ""
    return None, f"'{ad}' adı teslim listesinde yok.{oneri} Adlar Docs/Sablonlar/OKU_BENI.md'de."


# ---------------------------------------------------------------- 2D

def kontrol_2d(dosya, bilgi):
    from PIL import Image
    hata, uyari = [], []
    try:
        im = Image.open(dosya)
        im.load()
    except Exception as e:
        return [f"Resim açılamadı ({e})."], []
    if im.format != "PNG":
        hata.append(f"Dosya gerçekte {im.format}; PNG olarak kaydet (yalnızca uzantıyı değiştirmek yetmez).")
    W, H = bilgi["G"], bilgi["Y"]
    if im.size != (W, H):
        hata.append(f"Boyut {im.size[0]}x{im.size[1]} px, olması gereken {W}x{H} px (şablonla aynı).")
    if im.mode in ("I;16", "I", "F") or getattr(im, "bits", 8) > 8:
        hata.append("16 bit renk; 8 bit PNG olarak kaydet.")
    hikaye = bilgi["kategori"] == "Hikaye"
    rgba = im.convert("RGBA")
    px = list(rgba.get_flattened_data() if hasattr(rgba, "get_flattened_data") else rgba.getdata())
    n = len(px)
    alpha = [p[3] for p in px]
    if not hikaye:
        if "A" not in im.getbands() and not (im.mode == "P" and "transparency" in im.info):
            if siyah_zemin(rgba):
                hata.append("Arka plan siyah: şeffaflık gönderirken kaybolmuş (mesaj uygulaması PNG'yi JPG'ye çevirir). "
                            "Orijinal PNG'yi \"fotoğraf\" değil \"dosya/belge\" olarak gönder.")
            else:
                hata.append("Arka plan şeffaf değil (alfa kanalı yok). Şeffaf arka planlı PNG kaydet.")
        else:
            seffaf = sum(1 for a in alpha if a < 16) / n
            if seffaf < 0.03:
                hata.append("Neredeyse her yer opak: arka plan silinmemiş. Çizimin dışı şeffaf olmalı.")
        dolu = sum(1 for a in alpha if a > 32) / n
        if dolu < 0.02:
            hata.append("Görsel boş görünüyor (neredeyse hepsi şeffaf).")
    # Şablon kalıntısı: kırmızı hitbox çerçevesi, dama zemin
    if im.size == (W, H):
        kenar = [rgba.getpixel((x, y)) for x in range(W) for y in (0, 1, H - 2, H - 1)] + \
                [rgba.getpixel((x, y)) for y in range(H) for x in (0, 1, W - 2, W - 1)]
        kirmizi = sum(1 for r, g, b, a in kenar if a > 200 and abs(r - 214) < 12 and abs(g - 60) < 14 and abs(b - 50) < 14) / len(kenar)
        if kirmizi > 0.3:
            hata.append("Şablonun kırmızı çerçevesi duruyor: şablon katmanını silip öyle kaydet.")
        dama = sum(1 for r, g, b, a in px if a > 200 and (r, g, b) in ((236, 236, 240), (248, 248, 250))) / n
        if dama > 0.15:
            hata.append("Şablonun dama (gri kareli) zemini duruyor: şablon katmanını silip öyle kaydet.")
    if im.size == (W, H) and not hikaye and "A" in rgba.getbands():
        a = rgba.getchannel("A")
        if bilgi["isaret"] in ("ayak", "el"):
            alt = a.crop((0, int(H * 0.94), W, H))
            if alt.getbbox() is None:
                uyari.append("Ayaklar alt kenara değmiyor; oyunda karakter havada durur.")
            else:
                x0, _, x1, _ = alt.getbbox()
                if abs((x0 + x1) / 2 - W / 2) > W * 0.18:
                    uyari.append("Ayaklar yatayda ortada değil.")
        if bilgi["isaret"] == "el":
            el = a.crop((int(W * 0.72), int(H * 0.28), W, int(H * 0.62)))
            if el.getbbox() is None:
                uyari.append("Kalkık el sağ üstte görünmüyor (şablondaki yeşil daire); oyun noktalı izi oradan başlatır.")
        if bilgi["isaret"] == "daire":
            r = min(W, H) / 2 * 1.04
            disari = sum(1 for i, al in enumerate(alpha) if al > 64 and ((i % W + 0.5 - W / 2) ** 2 + (i // W + 0.5 - H / 2) ** 2) > r * r)
            if disari > 0.03 * max(1, sum(1 for al in alpha if al > 64)):
                uyari.append("İkon daireden taşıyor; oyunda yuvarlak düğmenin kenarından çıkar.")
    if os.path.getsize(dosya) > 3 * 1024 * 1024:
        uyari.append(f"Dosya büyük ({os.path.getsize(dosya) // 1024} KB); oyun açılışını yavaşlatabilir.")
    return hata, uyari


def siyah_zemin(rgba):
    W, H = rgba.size
    kose = [rgba.getpixel(p) for p in ((0, 0), (W - 1, 0), (0, H - 1), (W - 1, H - 1), (W // 2, 0), (0, H // 2))]
    return all(r < 12 and g < 12 and b < 12 for r, g, b, _ in kose)


def ayak_hizasi(yol):
    """(ayakların alt satırı, ayakların yatay ortası) piksel; alfa yoksa None"""
    from PIL import Image
    a = Image.open(yol).convert("RGBA").getchannel("A").point(lambda v: 255 if v > 32 else 0)
    bb = a.getbbox()
    if not bb:
        return None
    alt = bb[3]
    satir = a.crop((0, max(0, alt - 12), a.size[0], alt)).getbbox()
    return alt, (satir[0] + satir[2]) / 2 if satir else (bb[0] + bb[2]) / 2


def karakter_esi(k):
    return k.replace("_durus", "_goster") if "_durus" in k else k.replace("_goster", "_durus")


def poz_farki(p1, p2):
    """İki poz üst üste konunca gövde kıpırdıyor mu"""
    h1, h2 = ayak_hizasi(p1), ayak_hizasi(p2)
    if not h1 or not h2:
        return None
    if abs(h1[0] - h2[0]) > 8 or abs(h1[1] - h2[1]) > 12:
        return (f"İki poz aynı hizada değil (ayaklar {abs(h1[0] - h2[0]):.0f} px dikey, {abs(h1[1] - h2[1]):.0f} px yatay kayık); "
                "poz değişince karakter zıplar. Aynı tuvalde, aynı yerde çiz ya da --karakter ile birlikte hazırla.")
    return None


def karakter_hazirla(harf, durus, goster):
    """İki pozu aynı kesimle kırpar, 320x512'ye sığdırır (ayaklar alt kenarda, ortada), Art/_hazir/ altına yazar"""
    from PIL import Image
    harf = harf.upper()
    if harf not in ("A", "B", "C"):
        print("Karakter harfi A (Mert), B (Asil) ya da C (Yiğit) olmalı.")
        return None
    W, H, kenar = 320, 512, 4
    resimler = []
    for p in (durus, goster):
        im = Image.open(p)
        im.load()
        if "A" not in im.getbands() and not (im.mode == "P" and "transparency" in im.info):
            rgba = im.convert("RGBA")
            neden = ("arka plan siyah: şeffaflık gönderirken kaybolmuş (mesaj uygulaması PNG'yi JPG'ye çevirir). "
                     "Orijinal PNG'yi \"fotoğraf\" değil \"dosya/belge\" olarak iste.") if siyah_zemin(rgba) else "şeffaf arka plan yok."
            print(f"RED    {os.path.basename(p)}: {neden}")
            return None
        resimler.append(im.convert("RGBA"))
    if resimler[0].size != resimler[1].size:
        print(f"UYARI  iki pozun tuvali farklı boyda ({resimler[0].size} / {resimler[1].size}); aynı tuvalde çizilmediyse hiza kayabilir.")
    kutu = None
    for im in resimler:
        bb = im.getchannel("A").point(lambda v: 255 if v > 16 else 0).getbbox()
        if not bb:
            print("RED    pozlardan biri boş (her yer şeffaf).")
            return None
        kutu = bb if kutu is None else (min(kutu[0], bb[0]), min(kutu[1], bb[1]), max(kutu[2], bb[2]), max(kutu[3], bb[3]))
    kw, kh = kutu[2] - kutu[0], kutu[3] - kutu[1]
    olcek = min((W - 2 * kenar) / kw, (H - kenar - 2) / kh)
    yw, yh = max(1, round(kw * olcek)), max(1, round(kh * olcek))
    # Ayakların yatay ortası duruş pozundan (kalkık kol kesimi sağa genişletse de karakter ortada dursun)
    alt = resimler[0].getchannel("A").point(lambda v: 255 if v > 32 else 0)
    satir = alt.crop((0, kutu[3] - max(4, kh // 20), alt.size[0], kutu[3])).getbbox()
    ayak_x = ((satir[0] + satir[2]) / 2 - kutu[0]) * olcek if satir else yw / 2
    x0 = round(min(max(W / 2 - ayak_x, 0), W - yw))
    y0 = H - 2 - yh
    os.makedirs(HAZIR, exist_ok=True)
    cikti = []
    for im, ad in zip(resimler, ("durus", "goster")):
        kes = im.crop(kutu).resize((yw, yh), Image.LANCZOS)
        tuval = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        tuval.alpha_composite(kes, (x0, y0))
        hedef = os.path.join(HAZIR, f"{harf}_{ad}.png")
        tuval.save(hedef)
        cikti.append(hedef)
    print(f"Hazırlandı: {harf}_durus.png, {harf}_goster.png (320x512, ölçek %{olcek * 100:.0f}) -> Art/_hazir/")
    return cikti


def oyun_bul(arg):
    """(komut listesi, ekran görüntüsü klasörleri) ya da (None, sebep)"""
    if arg:
        adaylar = [arg]
    else:
        adaylar = [os.environ.get("DB_OYUN", "")]
        # en kısa yol: paketin üst klasöründeki başlatıcı (Windows/DisasterBoard.exe)
        adaylar += sorted(glob.glob(os.path.join(REPO, "Build", "TestKiti", "**", "DisasterBoard.exe"), recursive=True), key=len)
    for exe in adaylar:
        if exe and os.path.isfile(exe):
            kok = os.path.dirname(os.path.abspath(exe))
            parca = kok.split(os.sep)
            if "Binaries" in parca:   # içteki exe verildiyse paketin köküne çık (Saved/ orada)
                kok = os.sep.join(parca[:parca.index("Binaries") - 1])
            return [exe], [kok]
    # Andaç'ın bilgisayarı: paket yoksa derlenmiş projeyle editörden "-game"
    ed = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
    dll = os.path.join(REPO, "Binaries", "Win64", "UnrealEditor-DisasterBoard.dll")
    if os.path.isfile(ed) and os.path.isfile(dll):
        return [ed, os.path.join(REPO, "DisasterBoard.uproject"), "/Game/DisasterBoard/Maps/L_Ana", "-game"], [os.path.join(REPO, "Saved")]
    return None, ("Test paketi bulunamadı. DisasterBoard_TestKiti.zip'i repo içinde Build/TestKiti/ klasörüne aç "
                  "(içinde Windows/DisasterBoard.exe olmalı) ya da --oyun ile exe yolunu ver.")


def oyunda_dene(adaylar, oyun_arg):
    """adaylar: {anahtar: dosya}. Dönüş: ({anahtar: hata listesi}, ekran görüntüleri listesi, genel hata)"""
    komut, kokler = oyun_bul(oyun_arg)
    if komut is None:
        return {}, [], kokler
    # Deneme klasörü: şimdiye kadar onaylananlar + yeni adaylar (ekran görüntüsü bütün seti gösterir)
    shutil.rmtree(DENEME, ignore_errors=True)
    if os.path.isdir(os.path.join(ONAYLI, "2D")):
        shutil.copytree(os.path.join(ONAYLI, "2D"), DENEME)
    for k, f in adaylar.items():
        hedef = os.path.join(DENEME, k)
        os.makedirs(os.path.dirname(hedef), exist_ok=True)
        shutil.copy2(f, hedef)
    rapor = os.path.join(tempfile.gettempdir(), "DB_varlik_rapor.txt")
    if os.path.exists(rapor):
        os.remove(rapor)
    for kok in kokler:
        for eski in glob.glob(os.path.join(kok, "**", "V_*.png"), recursive=True):
            os.remove(eski)
    t0 = time.time()
    print(f"  Oyun açılıyor (yaklaşık 40 sn, pencereye dokunma)...", flush=True)
    try:
        subprocess.run(komut + ["-DBVarlikTest", f"-DBVarlik={DENEME}", f"-DBRapor={rapor}", "-windowed", "-ResX=1600",
                                "-ResY=900", "-nosplash"], timeout=240, cwd=os.path.dirname(komut[0]))
    except subprocess.TimeoutExpired:
        return {}, [], "Oyun 4 dakikada kapanmadı (takıldı ya da pencere arkada bekliyor)."
    if not os.path.exists(rapor):
        return {}, [], "Oyun raporu yazmadan kapandı (çöktü olabilir). Dosyaları tek tek dene, hangisinin çökerttiğini bul."
    with open(rapor, encoding="utf-8") as f:
        satirlar = f.read().splitlines()
    if "TUR TAMAM" not in satirlar:
        return {}, [], "Oyun testi yarıda kaldı (rapor eksik)."
    durum = dict(s.split("\t", 1) for s in satirlar if "\t" in s)
    sonuc = {}
    for k in adaylar:
        d = durum.get(k)
        b = LISTE[k]
        if d is None:
            sonuc[k] = ["Oyun bu görseli test turunda istemedi (araçta eşleme hatası; Andaç'a haber ver)."]
        elif d.startswith("OK"):
            sonuc[k] = [] if f"{b['G']}x{b['Y']} aday" in d else [f"Oyun dosyayı farklı okudu: {d}"]
        elif d.startswith("HATA"):
            sonuc[k] = ["Oyun dosyayı okuyamadı (bozuk ya da desteklenmeyen PNG). Başka bir programla PNG olarak yeniden kaydet."]
        else:
            sonuc[k] = ["Oyun dosyayı bulamadı (araçta yol hatası; Andaç'a haber ver)."]
    resimler = []
    for kok in kokler:
        resimler += [p for p in glob.glob(os.path.join(kok, "**", "V_*.png"), recursive=True) if os.path.getmtime(p) >= t0 - 1]
    return sonuc, sorted(resimler, key=os.path.basename), None


# ---------------------------------------------------------------- 3D

def blender_bul():
    b = os.environ.get("BLENDER", "")
    if b and os.path.isfile(b):
        return b
    bul = sorted(glob.glob(r"C:\Program Files\Blender Foundation\Blender*\blender.exe"))
    return bul[-1] if bul else (shutil.which("blender") or None)


def kontrol_3d(dosya, bilgi, onizleme):
    bl = blender_bul()
    if not bl:
        return ["Blender bulunamadı. Blender kur ya da BLENDER ortam değişkenine blender.exe yolunu yaz."], []
    out = os.path.join(tempfile.gettempdir(), "DB_3d_sonuc.json")
    if os.path.exists(out):
        os.remove(out)
    os.makedirs(os.path.dirname(onizleme), exist_ok=True)
    betik = os.path.join(HERE, "varlik_3d_kontrol.py")
    subprocess.run([bl, "-b", "--factory-startup", "--python", betik, "--", os.path.abspath(dosya), out, onizleme,
                    str(bilgi["G"]), str(bilgi["D"]), str(bilgi["Y"]), bilgi["kategori"], bilgi["ad"]],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    if not os.path.exists(out):
        return ["Blender dosyayı açarken kapandı (FBX bozuk olabilir)."], []
    with open(out, encoding="utf-8") as f:
        r = json.load(f)
    return r["hatalar"], r["uyarilar"] + ([BILGI + r["bilgi"]] if r.get("bilgi") else [])


# ---------------------------------------------------------------- ana akış

def reddet(dosya, sebepler):
    os.makedirs(RED, exist_ok=True)
    hedef = os.path.join(RED, os.path.basename(dosya))
    if os.path.abspath(dosya) != os.path.abspath(hedef):
        shutil.copy2(dosya, hedef)
    with open(hedef + ".neden.txt", "w", encoding="utf-8") as f:
        f.write("\n".join(sebepler) + "\n")


def sha1(dosya):
    with open(dosya, "rb") as f:
        return hashlib.sha1(f.read()).hexdigest()[:12]


def kontrol(dosyalar, oyun_arg, oyunsuz):
    d = durum_oku()
    girdiler = []
    for g in dosyalar:
        if os.path.isdir(g):
            girdiler += sorted(glob.glob(os.path.join(g, "**", "*.png"), recursive=True) + glob.glob(os.path.join(g, "**", "*.fbx"), recursive=True))
        elif os.path.isfile(g):
            girdiler.append(g)
        else:
            print(f"YOK  {g}: dosya bulunamadı")
    adaylar_2d, sonuc = {}, {}
    for f in girdiler:
        k, sebep = eslestir(f)
        if k is None:
            sonuc[f] = ("RED", None, [sebep], [])
            continue
        b = LISTE[k]
        if b["tur"] == "2D":
            h, u = kontrol_2d(f, b)
            if h:
                sonuc[f] = ("RED", k, h, u)
            else:
                adaylar_2d[k] = f
                sonuc[f] = ("OYUN", k, [], u)
        else:
            on = os.path.join(ONAYLI, "3D", "onizleme", k + ".png")
            h, u = kontrol_3d(f, b, on)
            sonuc[f] = ("RED" if h else "ONAY", k, h, u)
    resimler, genel = [], None
    if adaylar_2d:
        if oyunsuz:
            genel = "Oyun testi atlandı (--oyunsuz): 2D dosyalar onaylanmadı, yalnızca ölçü/format kontrolü yapıldı."
        else:
            oy, resimler, genel = oyunda_dene(adaylar_2d, oyun_arg)
            for f, (dur, k, h, u) in list(sonuc.items()):
                if dur == "OYUN" and k in oy:
                    sonuc[f] = ("RED" if oy[k] else "ONAY", k, oy[k], u)
        for f, (dur, k, h, u) in list(sonuc.items()):
            if dur == "OYUN":
                sonuc[f] = ("BEKLE", k, [genel], u)
    # Sonuç: onaylananlar yerine konur, reddedilenler Art/Reddedildi/ altına (sebebiyle)
    onaylanan = {k for f, (dur, k, h, u) in sonuc.items() if dur == "ONAY"}
    for f, (dur, k, h, u) in sonuc.items():
        ad = os.path.basename(f)
        if dur == "ONAY":
            b = LISTE[k]
            hedef = os.path.join(ONAYLI, "2D", k) if b["tur"] == "2D" else os.path.join(ONAYLI, "3D", k + ".fbx")
            os.makedirs(os.path.dirname(hedef), exist_ok=True)
            shutil.copy2(f, hedef)
            d[k] = {"tarih": datetime.now().strftime("%d.%m %H:%M"), "kaynak": ad, "sha1": sha1(f), "uyarilar": u}
            # Rehber karakter iki pozla çizilir: eşi bu seferde onaylanmadıysa ve önceden de yoksa söyle
            if k.startswith("Karakterler/"):
                es = karakter_esi(k)
                if es not in d and es not in onaylanan:
                    u = u + [f"Eşi {os.path.basename(es)} henüz onaylı değil; gelene kadar oyunda hep bu poz görünür."]
            print(f"ONAY   {ad}  ->  Art/Onayli/{os.path.relpath(hedef, ONAYLI).replace(os.sep, '/')}")
        elif dur == "RED":
            reddet(f, h + [x if x.startswith(BILGI) else f"(uyarı) {x}" for x in u])
            print(f"RED    {ad}")
            for x in h:
                print(f"         - {x}")
        else:
            print(f"BEKLE  {ad}: {h[0]}")
        for x in u if dur != "RED" else []:
            print(f"         {x}" if x.startswith(BILGI) else f"         uyarı: {x}")
    # İki poz da yerindeyse hizası: poz değişince karakter zıplamasın
    for k in sorted(onaylanan):
        es = karakter_esi(k) if k.startswith("Karakterler/") else None
        if es and es in d and k < es:
            fark = poz_farki(os.path.join(ONAYLI, "2D", k), os.path.join(ONAYLI, "2D", es))
            if fark:
                d[k]["uyarilar"].append(fark)
                print(f"UYARI  {os.path.basename(k)} / {os.path.basename(es)}: {fark}")
    if resimler:
        ek = os.path.join(ONAYLI, "_ekran")
        os.makedirs(ek, exist_ok=True)
        for p in resimler:
            shutil.copy2(p, os.path.join(ek, os.path.basename(p)))
        ilgili = sorted({s for f, (dur, k, h, u) in sonuc.items() if k and LISTE[k]["tur"] == "2D" for s in ekranlar(k)})
        print("Oyundan ekran görüntüleri (gözle bak, yerinde ve düzgün mü):")
        for s in ilgili:
            print(f"  Art/Onayli/_ekran/V_{s}.png")
    elif genel and adaylar_2d and not oyunsuz:
        print(f"OYUN TESTİ YAPILAMADI: {genel}")
    for f, (dur, k, h, u) in sonuc.items():
        if dur == "ONAY" and LISTE[k]["tur"] == "3D":
            print(f"Önizleme: Art/Onayli/3D/onizleme/{k}.png")
    durum_yaz(d)
    eksik = [k for k in LISTE if k not in d]
    print(f"Toplam onaylı: {len(LISTE) - len(eksik)} / {len(LISTE)}" + ("  HEPSİ TAMAM: --teslim ile gönder." if not eksik else ""))


def durum_goster():
    d = durum_oku()
    rapor_yaz(d)
    for tur in ("2D", "3D"):
        ks = [k for k, v in LISTE.items() if v["tur"] == tur]
        ok = [k for k in ks if k in d]
        print(f"{tur}: {len(ok)} / {len(ks)} onaylı")
        for k in ks:
            print(f"  {'ONAYLI  ' if k in d else 'bekliyor'}  {k if tur == '2D' else k + '.fbx'}")
    print(f"Toplam: {len(d)} / {len(LISTE)}  (tablo: Art/Onayli/RAPOR.md)")


def git(*a):
    return subprocess.run(["git", *a], cwd=REPO, capture_output=True, text=True, encoding="utf-8")


def teslim(kismi):
    d = durum_oku()
    eksik = [k for k in LISTE if k not in d]
    if eksik and not kismi:
        print(f"{len(eksik)} dosya daha onay bekliyor (--durum). Yine de göndermek için: --teslim --kismi")
        return 1
    rapor_yaz(d)
    git("add", "Art/Onayli")
    if not git("diff", "--cached", "--quiet").returncode:
        print("Gönderilecek yeni onaylı dosya yok.")
        return 0
    o2 = sum(1 for k in d if LISTE.get(k, {}).get("tur") == "2D")
    msg = f"Onaylı varlıklar: {len(d)} / {len(LISTE)} ({o2} 2D, {len(d) - o2} 3D)\n\nTools/varlik_kontrol.py ile kontrol edildi ve oyunda denendi. Tablo: Art/Onayli/RAPOR.md"
    r = git("commit", "-m", msg)
    if r.returncode:
        print(r.stdout + r.stderr)
        return 1
    r = git("pull", "--rebase", "--autostash")
    if r.returncode:
        print("git pull başarısız:\n" + r.stdout + r.stderr)
        return 1
    r = git("push")
    if r.returncode:
        print("git push başarısız:\n" + r.stdout + r.stderr)
        return 1
    print(f"Gönderildi: {len(d)} / {len(LISTE)} onaylı dosya repoda (Art/Onayli/). Andaç pull edip --aktar ile oyuna alır.")
    return 0


def aktar():
    src = os.path.join(ONAYLI, "2D")
    n = 0
    for p in glob.glob(os.path.join(src, "**", "*.png"), recursive=True):
        rel = os.path.relpath(p, src)
        hedef = os.path.join(REPO, "Content", rel)
        os.makedirs(os.path.dirname(hedef), exist_ok=True)
        shutil.copy2(p, hedef)
        n += 1
        print(f"  Content/{rel.replace(os.sep, '/')}")
    print(f"{n} görsel Content/ altına kopyalandı (oyun dosyadan okur; derleme gerekmez, pakete de girer).")
    fbx = sorted(glob.glob(os.path.join(ONAYLI, "3D", "*.fbx")))
    if fbx:
        print(f"{len(fbx)} model Art/Onayli/3D/ altında; Unreal'a içe aktarılıp DBWorld'deki yer tutucuların yerine bağlanacak:")
        for p in fbx:
            print(f"  {os.path.basename(p)}")


def geri(ad):
    d = durum_oku()
    k, sebep = eslestir(ad if os.path.splitext(ad)[1] else ad + (".fbx" if ad.startswith("SM_") else ".png"))
    if k is None or k not in d:
        print(f"Onaylı listesinde yok: {ad}" + (f" ({sebep})" if sebep else ""))
        return 1
    for p in (os.path.join(ONAYLI, "2D", k), os.path.join(ONAYLI, "3D", k + ".fbx"), os.path.join(ONAYLI, "3D", "onizleme", k + ".png")):
        if os.path.exists(p):
            os.remove(p)
    del d[k]
    durum_yaz(d)
    print(f"Onay geri alındı: {k}. Toplam onaylı: {len(d)} / {len(LISTE)}")
    return 0


def main(argv):
    oyun_arg = None
    if "--oyun" in argv:
        i = argv.index("--oyun")
        oyun_arg = argv[i + 1]
        del argv[i:i + 2]
    if "--durum" in argv:
        return durum_goster()
    if "--teslim" in argv:
        return teslim("--kismi" in argv)
    if "--aktar" in argv:
        return aktar()
    if "--karakter" in argv:
        i = argv.index("--karakter")
        if i + 3 >= len(argv):
            print("--karakter <A|B|C> <duruş dosyası> <gösterme dosyası>")
            return 1
        hazir = karakter_hazirla(argv[i + 1], argv[i + 2], argv[i + 3])
        if not hazir:
            return 1
        kontrol(hazir, oyun_arg, "--oyunsuz" in argv)
        return 0
    if "--geri" in argv:
        i = argv.index("--geri")
        return geri(argv[i + 1]) if i + 1 < len(argv) else (print("--geri <dosya adı>") or 1)
    dosyalar = [a for a in argv if not a.startswith("--")]
    if not dosyalar:
        print(__doc__ if __doc__ else open(__file__, encoding="utf-8").read().split("import")[0])
        return 1
    try:
        import PIL  # noqa: F401
    except ImportError:
        print("Pillow kurulu değil: python -m pip install pillow")
        return 1
    kontrol(dosyalar, oyun_arg, "--oyunsuz" in argv)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]) or 0)
