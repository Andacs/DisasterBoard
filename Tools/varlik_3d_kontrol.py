# Modelcinin FBX'ini Blender'da açar, şablondaki kutuya (hitbox) göre ölçer, önizleme resmi çizer.
#   blender -b --factory-startup --python Tools/varlik_3d_kontrol.py -- <fbx> <sonuc.json> <onizleme.png> <G> <D> <Y> <kategori> <ad>
# Tools/varlik_kontrol.py çağırır. Ölçüler cm (FBX içe alınınca Blender metre kullanır: 1 m = 100 cm).
import bpy, json, math, sys
from mathutils import Vector

a = sys.argv[sys.argv.index("--") + 1:]
fbx, out, onizleme = a[0], a[1], a[2]
G, D, Y = float(a[3]), float(a[4]), float(a[5])
kategori, ad = a[6], a[7]
hata, uyari = [], []


def bitir(bilgi=""):
    with open(out, "w", encoding="utf-8") as f:
        json.dump({"hatalar": hata, "uyarilar": uyari, "bilgi": bilgi}, f, ensure_ascii=False)
    sys.exit(0)


def cm(v):
    return f"{v:.1f}".replace(".", ",")


bpy.ops.wm.read_factory_settings(use_empty=True)
try:
    bpy.ops.import_scene.fbx(filepath=fbx)
except Exception as e:
    hata.append(f"FBX açılamadı: {e}")
    bitir()

meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
if not meshes:
    hata.append("Dosyada mesh yok (yalnızca boş nesne / kamera / ışık).")
    bitir()

dg = bpy.context.evaluated_depsgraph_get()
lo = Vector((1e9, 1e9, 1e9))
hi = Vector((-1e9, -1e9, -1e9))
tris = 0
mats = set()
for o in meshes:
    ev = o.evaluated_get(dg)
    me = ev.to_mesh()
    mw = o.matrix_world
    for v in me.vertices:
        p = mw @ v.co
        lo = Vector(map(min, lo, p))
        hi = Vector(map(max, hi, p))
    tris += sum(max(0, len(p.vertices) - 2) for p in me.polygons)
    for s in o.material_slots:
        if s.material:
            mats.add(s.material.name)
    ev.to_mesh_clear()

lo_cm, hi_cm = lo * 100.0, hi * 100.0
dim = hi_cm - lo_cm
merkez = (lo_cm + hi_cm) / 2.0
bilgi = f"Ölçü {cm(dim.x)} x {cm(dim.y)} x {cm(dim.z)} cm (kutu {cm(G)} x {cm(D)} x {cm(Y)}), {tris} üçgen, {len(meshes)} parça, {len(mats)} malzeme"

# Birim: 100 kat büyük/küçük → cm / m karışmış
oran = max(dim.x / G, dim.y / D, dim.z / Y)
if oran > 20 or oran < 0.05:
    hata.append(f"Birim yanlış: model {cm(dim.x)} x {cm(dim.y)} x {cm(dim.z)} cm çıkıyor, kutu {cm(G)} x {cm(D)} x {cm(Y)} cm. "
                "Blender'da Unit Scale 0.01 / Length Centimeters olmalı ve FBX'te Apply Scalings: FBX All.")
    bitir(bilgi)

# Kutuya sığıyor mu (her eksende %3 + 1,5 mm pay)
pay = lambda k: k * 1.03 + 0.15
eksen = [("genişlik (X)", dim.x, G), ("derinlik (Y)", dim.y, D), ("yükseklik (Z)", dim.z, Y)]
tasan = [f"{n} {cm(v)} cm > {cm(k)} cm" for n, v, k in eksen if v > pay(k)]
if tasan:
    if abs(G - D) > 0.5 and dim.x <= pay(D) and dim.y <= pay(G) and dim.z <= pay(Y):
        hata.append("Model 90° dönük: X ile Y yer değiştirmiş. Uzun kenar şablondaki gibi olmalı; ÖN yazan yüze göre çevir.")
    else:
        hata.append("Kutudan taşıyor: " + ", ".join(tasan) + ". Model şablon kutusunun içinde kalmalı.")
if oran < 0.6:
    uyari.append(f"Model kutunun çok küçük bir kısmını dolduruyor (en çok %{oran * 100:.0f}); ölçek yanlış olabilir.")
if kategori == "Bina" and (dim.x < G * 0.9 or dim.y < D * 0.9):
    uyari.append("Taban plakası 2x2 karoyu (14,6 x 14,6) tam doldurmuyor; tahtada boşluk kalır.")

# Pivot: taban ortası, en alt nokta Z = 0
if abs(lo_cm.z) > max(0.1, Y * 0.01):
    hata.append(f"Taban Z = 0'da değil (en alt nokta {cm(lo_cm.z)} cm). Modeli origin'e oturt: en alt nokta 0 olmalı.")
if abs(merkez.x) > max(0.3, G * 0.04) or abs(merkez.y) > max(0.3, D * 0.04):
    hata.append(f"Pivot taban ortasında değil: model merkezi X {cm(merkez.x)}, Y {cm(merkez.y)} cm kaymış. "
                "Modeli 0,0'a ortala, sonra Ctrl+A > All Transforms.")

# Üçgen sayısı (lowpoly masa oyunu)
sinir = {"Bina": 15000, "Nesne": 4000, "Figür": 4000, "Sahne": 30000}.get(kategori, 8000)
if tris > sinir * 3:
    hata.append(f"Çok ağır: {tris} üçgen (bu tür için en çok {sinir}). Lowpoly yap / decimate.")
elif tris > sinir:
    uyari.append(f"{tris} üçgen; bu tür için önerilen en çok {sinir}.")
if len(mats) > 6:
    uyari.append(f"{len(mats)} malzeme var; 1-3 düz renk yeterli.")

# Önizleme: model + kırmızı tel kutu (hitbox) + taban ortasında yeşil pivot, önden-yandan-üstten
try:
    sc = bpy.context.scene
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, Y / 200.0))
    kutu = bpy.context.active_object
    kutu.scale = (G / 100.0, D / 100.0, Y / 100.0)
    mod = kutu.modifiers.new("tel", "WIREFRAME")
    mod.thickness = max(G, D, Y) / 100.0 * 0.025
    mk = bpy.data.materials.new("hitbox")
    mk.diffuse_color = (0.85, 0.15, 0.1, 1)
    kutu.data.materials.append(mk)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=max(G, D, Y) / 100.0 * 0.03, location=(0, 0, 0))
    pv = bpy.context.active_object
    mp = bpy.data.materials.new("pivot")
    mp.diffuse_color = (0.2, 0.7, 0.3, 1)
    pv.data.materials.append(mp)
    b = max(G, D, Y) / 100.0
    # Ön yüzün önünde yerde "ÖN" (araba: +X ucunda): model ters dönükse resimden anlaşılır
    cu = bpy.data.curves.new("on", type="FONT")
    cu.body = "ÖN"
    cu.size = b * 0.22
    cu.align_x = "CENTER"
    on = bpy.data.objects.new("ON", cu)
    on.location = (G / 200.0 + b * 0.08, 0, 0) if ad == "SM_Araba" else (0, -D / 200.0 - b * 0.12, 0)
    if ad == "SM_Araba":
        on.rotation_euler = (0, 0, 1.5708)
    on.data.materials.append(mk)
    sc.collection.objects.link(on)
    hedef = Vector((0, -b * 0.05, Y / 260.0))
    bpy.ops.object.camera_add(location=hedef + Vector((0.55, -1.0, 0.6)).normalized() * b * 2.9)
    cam = bpy.context.active_object
    yon = hedef - cam.location
    cam.rotation_euler = yon.to_track_quat("-Z", "Y").to_euler()
    sc.camera = cam
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.light = "STUDIO"
    sc.display.shading.color_type = "MATERIAL"
    sc.display.shading.show_shadows = True
    sc.render.resolution_x, sc.render.resolution_y = 900, 700
    sc.render.film_transparent = False
    sc.world = bpy.data.worlds.new("w") if not sc.world else sc.world
    sc.world.color = (0.96, 0.93, 0.87)
    sc.render.filepath = onizleme
    bpy.ops.render.render(write_still=True)
except Exception as e:
    uyari.append(f"Önizleme çizilemedi ({e}).")

bitir(bilgi)
