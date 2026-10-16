# L_Ana ve L_Test: gunes, gok isigi, atmosfer, sis. Basliksiz calisir, var olani ezmez.
#   UnrealEditor-Cmd.exe <tam yol>.uproject -run=pythonscript -script=<tam yol>/Tools/olustur_temel_assetler.py
import unreal
EAL = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
results = []

def fail(msg):
    unreal.log_error("JAM_SETUP FAIL: " + msg)
    raise RuntimeError(msg)

def make_level(full):
    if EAL.does_asset_exist(full):
        results.append("SKIP " + full); return
    if not les.new_level(full): fail("new_level " + full)
    if ues.get_editor_world() is None: fail("no editor world after new_level")
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, -30))
    if not sun: fail("spawn sun")
    sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture", True)
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -500))
    if not les.save_current_level(): fail("save " + full)
    results.append("OK " + full)

make_level("/Game/DisasterBoard/Maps/L_Ana")
make_level("/Game/DisasterBoard/Maps/L_Test")
for r in results: unreal.log("JAM_SETUP " + r)
unreal.log("JAM_SETUP DONE")
