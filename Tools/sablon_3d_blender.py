# 3D hitbox şablonları: her model için oyundaki ölçüde kutu + isim etiketi + ÖN işareti.
#   blender -b --python Tools/sablon_3d_blender.py -- <çıktı klasörü>
# Çıktı: DisasterBoard_3D_Sablonlar.blend (hepsi bir arada, 1 birim = 1 cm), FBX/<ad>.fbx (her biri ayrı).
#        Ölçü tablosu (PNG) Tools/sablon_2d.py üretir. Ölçüler Tools/sablon_listesi.py'den.
import bpy, os, sys, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sablon_listesi import ASSETS_3D

out = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else os.path.join(os.getcwd(), "Sablonlar3D")
os.makedirs(os.path.join(out, "FBX"), exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.system = 'METRIC'
sc.unit_settings.scale_length = 0.01      # 1 Blender birimi = 1 cm (Unreal ile aynı)
sc.unit_settings.length_unit = 'CENTIMETERS'

COLORS = {"Bina": (0.92, 0.55, 0.45, 0.55), "Nesne": (0.40, 0.70, 0.68, 0.55), "Figür": (0.93, 0.74, 0.34, 0.55), "Sahne": (0.65, 0.60, 0.80, 0.45)}
FRONT = {"Bina", "Figür", "Sahne", "Nesne"}

def make_box(name, w, d, h, color):
    me = bpy.data.meshes.new(name)
    x, y = w / 2, d / 2
    v = [(-x, -y, 0), (x, -y, 0), (x, y, 0), (-x, y, 0), (-x, -y, h), (x, -y, h), (x, y, h), (-x, y, h)]
    f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    me.from_pydata(v, [], f)
    me.update()
    ob = bpy.data.objects.new(name, me)
    ob.color = color
    bpy.context.collection.objects.link(ob)
    return ob

def make_text(body, size, loc, rot=(math.radians(90), 0, 0), color=(0.1, 0.08, 0.06, 1)):
    cu = bpy.data.curves.new("txt", type='FONT')
    cu.body = body
    cu.size = size
    cu.align_x = 'CENTER'
    ob = bpy.data.objects.new("Etiket_" + body.split("\n")[0], cu)
    ob.location = loc
    ob.rotation_euler = rot
    ob.color = color
    bpy.context.collection.objects.link(ob)
    return ob

rows = {"Bina": 0.0, "Nesne": -45.0, "Figür": -45.0, "Sahne": -150.0}
cursor = {"Bina": 0.0, "Nesne": 0.0, "Figür": 0.0, "Sahne": 0.0}
cursor["Figür"] = 999  # nesnelerle aynı sıra, nesnelerden sonra
boxes = []
for name, w, d, h, cat, note in ASSETS_3D:
    key = "Nesne" if cat == "Figür" else cat
    x = cursor[key] + w / 2
    cursor[key] += w + (10.0 if cat != "Sahne" else 30.0)
    y = rows[cat]
    ob = make_box(name, w, d, h, COLORS[cat])
    ob.location = (x, y, 0)
    boxes.append((ob, w, d, h))
    ts = 4.0 if cat == "Sahne" else (1.6 if w < 10 else 2.0)
    make_text(f"{name}\n{w:g} x {d:g} x {h:g} cm", ts, (x, y - d / 2 - 0.5, h + ts * 1.6))
    if cat in FRONT:
        make_text("ÖN", max(1.0, min(w, h) * 0.18), (x, y - d / 2 - 0.05, h * 0.45), color=(0.75, 0.15, 0.1, 1))
    # tek tek FBX (yalnızca kutu): modelci referans olarak içe alır, modelini kutunun içine yapar
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    old = ob.location.copy()
    ob.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, "FBX", name + ".fbx"), use_selection=True, object_types={'MESH'},
                             apply_unit_scale=True, apply_scale_options='FBX_SCALE_ALL', axis_forward='-Y', axis_up='Z', bake_space_transform=False)
    ob.location = old

# Ölçek referansı: tek karo (8 cm) ve 2x2 bina tabanı ızgarası
make_text("1 karo = 8 cm  ·  bina tabanı 2x2 karo = 14.6 cm  ·  pivot: taban ortası", 3.0, (0, 30, 0), rot=(0, 0, 0))

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "DisasterBoard_3D_Sablonlar.blend"))

print("SABLON3D TAMAM", len(boxes))
