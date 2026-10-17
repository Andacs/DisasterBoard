# DisasterBoard: modelci ve tasarımcıların yapacağı her şeyin listesi ve ölçüleri.
# Tek kaynak: Tools/sablon_3d_blender.py (3D hitbox kutuları), Tools/sablon_2d.py (2D PNG şablonları) ve
# Docs/Sablonlar/OKU_BENI.md bu listeden üretilir. Ölçü değişirse burayı değiştirip ikisini yeniden çalıştır.
#
# 3D ölçüler OYUNDAKİ GERÇEK BOYUT, santim (Unreal: 1 birim = 1 cm). Sahne minyatür bir masa oyunu:
# tahta 128 cm, bir karo 8 cm, bir bina 2x2 karoya (14.6 x 14.6 cm) oturur.
# G = genişlik (X), D = derinlik (Y), Y = yükseklik (Z). Pivot her zaman TABANIN ORTASI (Z=0 en alt nokta).

# (dosya adı, G, D, Y, kategori, açıklama)
ASSETS_3D = [
    # --- Binalar: taban (plaka) 14.6 x 14.6, gövde bunun içinde; kutu = binanın en büyük hâli (çatı dahil)
    ("SM_Bina_AyseTeyze",    14.6, 14.6, 18.5, "Bina", "Ayşe Teyze'nin evi. Küçük ev, beşik çatı. Gövde ~10.9 x 10.9."),
    ("SM_Bina_Okul",         14.6, 14.6, 21.9, "Bina", "Okul. Düz çatı. Gövde ~12.2 x 12.2."),
    ("SM_Bina_Siginak",      14.6, 14.6, 11.6, "Bina", "Sığınak: yarım küre / bunker, kapı, yeşil bayrak. Tsunamide sakinler buraya yürür."),
    ("SM_Bina_Yilmazlar",    14.6, 14.6, 20.9, "Bina", "Yılmazların evi (bebekli aile). Beşik çatı. Gövde ~12.2 x 10.9."),
    ("SM_Bina_EskiApartman", 14.6, 14.6, 43.8, "Bina", "Eski apartman, en yüksek bina, kolonlarda çatlak. Düz çatı + su deposu. Gövde ~12.2 x 12.2."),
    ("SM_Bina_Kitapci",      14.6, 14.6, 20.3, "Bina", "Kitapçı (Elif). Beşik çatı. Gövde ~12.2 x 12.2."),
    ("SM_Bina_Firin",        14.6, 14.6, 17.3, "Bina", "Fırın (Ali Usta). Baca olabilir. Gövde ~12.2 x 10.9."),
    ("SM_Bina_Market",       14.6, 14.6, 17.3, "Bina", "Bakkal / market (Rıza Abi). Gövde ~10.9 x 10.9."),
    ("SM_Bina_OgrenciEvi",   14.6, 14.6, 24.3, "Bina", "Öğrenci evi (Can ve kedisi Pamuk). Düz çatı. Gövde ~10.9 x 12.2."),
    ("SM_Bina_Park",         14.6, 14.6,  8.5, "Bina", "Park: çim, 2 ağaç, bank. Toplanma alanı."),
    ("SM_Bina_SahilEvi",     14.6, 14.6, 18.5, "Bina", "Sahil evi (balıkçı Murat). Denize bakar. Gövde ~10.9 x 10.9."),
    ("SM_Bina_AhsapEvler",   14.6, 14.6, 18.5, "Bina", "Bitişik ahşap evler (Nermin Hanım). Gövde ~12.2 x 10.9."),
    # --- Küçük nesneler
    ("SM_Agac",              6.7,  6.7,  9.1, "Nesne", "Ağaç (gövde + yuvarlak tepe). Boş arsalarda ve parkta."),
    ("SM_Araba",             6.0,  3.0,  3.4, "Nesne", "Araba. ÖN yüzü işaretli tarafa (hareket yönü). 3 renk varyantı kodla boyanır."),
    ("SM_YanginMuslugu",     3.2,  3.2,  5.5, "Nesne", "Yangın musluğu. Bozukken kod eğik gösterir, tamirde doğrultur; tek mesh yeter."),
    ("SM_RogarKapagi",       5.4,  5.4,  0.6, "Nesne", "Rögar kapağı (ızgaraya konur). Yassı daire, üstünde çubuklar."),
    ("SM_SuBirikintisi",     7.2,  7.2,  0.3, "Nesne", "Su birikintisi (yolda, sel). Yassı, düzensiz kenar olabilir."),
    ("SM_Kasabali",          3.4,  3.4,  8.5, "Figür", "Kasabalı figürü (binanın önünde durur, tıklanınca konuşur). 12 kişi: AyseTeyze, SelmaOgretmen, HasanAmca, Zeynep, KemalBey, Elif, AliUsta, RizaAbi, Can, Deniz, MuratAbi, NerminHanim. İsim: SM_Kasabali_<Ad>."),
    ("SM_TahliyeKisi",       2.6,  2.6,  6.9, "Figür", "Tahliyede sığınağa yürüyen sakin (4 tane, kod renklendirir)."),
    # --- Sahne
    ("SM_Takvim",           96.5,  2.0, 74.3, "Sahne", "Duvar takvimi (çerçeve + spiral). Sayfa alanı 88.5 x 64.3, ÖN odaya bakar. Yazılar koddan, sayfaya çizilir."),
    ("SM_Tahta",           128.0, 128.0, 15.2, "Sahne", "Oyun tahtası: 13 x 13 karo (8 cm aralık, karo 6.6), pahlı kenar. Karo üstü Z = 13.8. Mevcut taslak: Art/Sahne/AnaMenu_OyunAlani.blend (Cube.002)."),
]

# 2D: piksel. (dosya yolu Content/ altında, genişlik, yükseklik, kategori, açıklama, ek işaret)
# ek işaret: "ayak" = pivot alt orta (ayaklar), "omuz" = pivot sol orta (kol döner), "altyazi" = sol alt boş bırak, "daire" = ikon daire içinde
ITEMS = [
    ("sargi_bezi", "Sargı Bezi"), ("antiseptik", "Antiseptik"), ("yara_bandi", "Yara Bandı"), ("su", "Su"),
    ("el_feneri", "El Feneri"), ("duduk", "Düdük"), ("yangin_tupu", "Yangın Tüpü"), ("duman_maskesi", "Duman Maskesi"),
    ("kalp_ilaci", "Kalp İlacı"), ("astim_spreyi", "Astım Spreyi"), ("gozluk", "Gözlük"), ("bebek_mamasi", "Bebek Maması"),
    ("bebek_bezi", "Bebek Bezi"), ("kedi_mamasi", "Kedi Maması"), ("battaniye", "Battaniye"), ("powerbank", "Powerbank"),
    ("onemli_belgeler", "Önemli Belgeler"),
]
TOOLS = [("rogar_kapagi", "Rögar Kapağı"), ("tahliye", "Tahliye"), ("ingiliz_anahtari", "İngiliz Anahtarı"), ("bina_guclendirme", "Bina Güçlendirme")]
KITS = [("ilk_yardim", "İlk Yardım Çantası"), ("deprem", "Deprem Çantası"), ("yangin", "Yangın Çantası")]

ASSETS_2D = []
for who, name in (("A", "Mert"), ("B", "Asil"), ("C", "Yiğit")):
    ASSETS_2D.append((f"Karakterler/{who}_govde.png", 320, 512, "Karakter", f"{name}: gösteren kol HARİÇ bütün figür, ayakta, kameraya bakıyor. Ayaklar alt kenarda, yatayda ortada.", "ayak"))
    ASSETS_2D.append((f"Karakterler/{who}_kol.png", 256, 96, "Karakter", f"{name}: gösteren kol, yatay, SAĞA uzanmış, işaret parmağı sağ uçta. Omuz sol kenarın ortasında (oyun buradan döndürür).", "omuz"))
for i, cap in enumerate(["Beş arkadaş kasabada geziyor", "Oyuncakçının vitrininde tozlu kutu: DisasterBoard",
                         "Kutudan minik kasaba çıkıyor, kartta: Afet gelmeden kasabayı hazırla", "Masaya oturuyorlar: Sıra sende!"], 1):
    ASSETS_2D.append((f"Cizgiroman/sahne{i}.png", 1280, 720, "Hikaye", f"Sahne {i}: {cap}. Altyazıyı oyun yazar, resme yazı koyma.", "altyazi"))
for key, name in ITEMS:
    ASSETS_2D.append((f"Ikonlar/esya_{key}.png", 128, 128, "Eşya ikonu", f"{name} (fabrikada düğme, çantada küçük ikon). Şeffaf arka plan, daire içine sığsın.", "daire"))
for key, name in TOOLS:
    ASSETS_2D.append((f"Ikonlar/onlem_{key}.png", 128, 128, "Önlem ikonu", f"{name} (alttaki şehir önlemleri paneli). Şeffaf arka plan.", "daire"))
for key, name in KITS:
    ASSETS_2D.append((f"Ikonlar/kit_{key}.png", 128, 128, "Kit ikonu", f"{name} (fabrikada temel kit düğmesi, sipariş kartı).", "daire"))
ASSETS_2D += [
    ("Ikonlar/isaret_uyari.png", 96, 96, "İşaret", "Riskli yerin üstündeki uyarı (!) işareti. Alt ucu sivri (binayı gösterir).", "daire"),
    ("Ikonlar/isaret_tamam.png", 96, 96, "İşaret", "Hazır olan yerin üstündeki yeşil tik işareti.", "daire"),
    ("Ikonlar/butce.png", 64, 64, "Arayüz", "Bütçe (para) simgesi, üstteki bütçe kutusunda.", "daire"),
    ("Arayuz/tarayici.png", 168, 300, "Arayüz", "Tarayıcı cihazı (sol altta, Mert'in yanında; basınca tahta taranır). Antenler dahil.", ""),
    ("Arayuz/canta.png", 168, 176, "Arayüz", "Fabrikada bantta inen sırt çantası (boş hâli). Kit seçilince kod rengini değiştirir: açık/nötr renk yap.", ""),
    ("Arayuz/logo.png", 1024, 256, "Arayüz", "DisasterBoard logosu (takvim menüsünde başlık). Şeffaf arka plan.", ""),
]
