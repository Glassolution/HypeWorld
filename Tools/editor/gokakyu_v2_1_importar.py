"""Gōkakyū v2 (08/11), passo 1: backup da versão atual + importação das malhas feitas no Blender.

- Backup (cópias, nada é apagado) em /Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Backup_v1_0811/:
  NS_Katon_Gokakyu e os sistemas que o DA usa hoje, DA_Katon_Goukakyuu e BP_Katon_Goukakyuu.
  Os valores atuais do DA/BP também vão para Docs/Jutsus/Katon/GokakyuV2/backup_valores.json (para voltar).
- Malhas: SourceAssets/VFX/Katon/FBX/*.fbx (exportadas de SourceAssets/VFX/Katon/Katon_Meshes.blend) ->
  /Game/HypeWorld/VFX/Katon/Meshes/. Sem colisão; cor de vértice importada (preto = borda, branco = centro).
Pode rodar de novo (o backup não é refeito se já existir; as malhas são reimportadas).
"""
import json
import os
import unreal

EAL = unreal.EditorAssetLibrary
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FBX_DIR = os.path.join(PROJ, "SourceAssets/VFX/Katon/FBX")
MESHES = "/Game/HypeWorld/VFX/Katon/Meshes"
KATON = "/Game/HypeWorld/Jutsus/Katon"
BACKUP = KATON + "/VFX/Gokakyu/Backup_v1_0811"
DA = KATON + "/DA_Katon_Goukakyuu"
BP = KATON + "/BP_Katon_Goukakyuu"

# ------------------------------------------------------------------------------------------------ backup
da = unreal.load_asset(DA)
bp = unreal.load_asset(BP)
cdo = unreal.get_default_object(bp.generated_class())
fontes = {KATON + "/VFX/Gokakyu/NS_Katon_Gokakyu", DA, BP}
for prop in ("goukakyuu_exit_system", "goukakyuu_flight_system", "goukakyuu_trail_system", "goukakyuu_impact_system"):
    a = da.get_editor_property(prop)
    if a:
        fontes.add(a.get_path_name().split(".")[0])
for src in sorted(fontes):
    dst = BACKUP + "/" + src.split("/")[-1] + "_v1"
    if not EAL.does_asset_exist(dst):
        assert EAL.duplicate_asset(src, dst), src
        print("backup:", src, "->", dst)
EAL.save_directory(BACKUP, only_if_is_dirty=False, recursive=True)   # a cópia nasce só na memória


def val(v):
    if v is None:
        return None
    if isinstance(v, unreal.Object):
        return v.get_path_name()
    if isinstance(v, (unreal.LinearColor, unreal.Vector, unreal.Vector2D)):
        return str(v)
    if isinstance(v, unreal.Array):
        return [val(x) for x in v]
    return v


DA_PROPS = ["gokakyu_core_material", "gokakyu_shell_material", "gokakyu_charge_system", "goukakyuu_exit_system",
            "goukakyuu_flight_system", "goukakyuu_trail_system", "goukakyuu_impact_system", "goukakyuu_material",
            "projectile_speed", "projectile_range", "cast_shake", "explosion_shake"]
BP_PROPS = ["grow_time", "start_radius", "full_radius", "speed", "range", "accel_time", "launch_speed_scale",
            "gokakyu_size", "gokakyu_core_scale", "gokakyu_pulse", "gokakyu_pulse_rate", "gokakyu_shell_scales",
            "gokakyu_shell_spins", "gokakyu_shell_stretch", "gokakyu_tongue", "gokakyu_wobble", "gokakyu_wobble_rate",
            "gokakyu_tint", "gokakyu_intensity", "gokakyu_light_intensity", "gokakyu_light_radius",
            "gokakyu_light_flicker", "gokakyu_light_color", "gokakyu_light_flicker_rate", "gokakyu_show_meshes",
            "gokakyu_light_launch_peak", "gokakyu_hold_time", "gokakyu_hold_forward", "gokakyu_impact_light",
            "gokakyu_impact_light_radius", "goukakyuu_exit_light_intensity", "goukakyuu_exit_light_radius",
            "anticipation_light"]
valores = {"DA": {}, "BP": {}}
for p in DA_PROPS:
    try:
        valores["DA"][p] = val(da.get_editor_property(p))
    except Exception as e:
        valores["DA"][p] = "ERRO %s" % e
for p in BP_PROPS:
    try:
        valores["BP"][p] = val(cdo.get_editor_property(p))
    except Exception as e:
        valores["BP"][p] = "ERRO %s" % e
out = os.path.join(PROJ, "Docs/Jutsus/Katon/GokakyuV2/backup_valores.json")
if not os.path.exists(out):
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as f:
        json.dump(valores, f, indent=1, ensure_ascii=False)
print(json.dumps(valores, ensure_ascii=False))

# ------------------------------------------------------------------------------------------------ malhas
tasks = []
for fn in sorted(os.listdir(FBX_DIR)):
    if not fn.lower().endswith(".fbx"):
        continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(FBX_DIR, fn))
    t.set_editor_property("destination_path", MESHES)
    t.set_editor_property("destination_name", fn[:-4])
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    print("importado:", list(t.get_editor_property("imported_object_paths")))

for name in ["SM_Katon_Core", "SM_Katon_Shell", "SM_Katon_Ribbon_01", "SM_Katon_Ribbon_02", "SM_Katon_Ribbon_03",
             "SM_Katon_Ribbon_04", "SM_Katon_Shockwave"]:
    m = unreal.load_asset(MESHES + "/" + name)
    assert m, name
    # sem colisão (só visual) e sem material importado: o material vem do Niagara/C++
    body = m.get_editor_property("body_setup")
    if body:
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        unreal.EditorStaticMeshLibrary.remove_collisions(m)
    # Nanite vem ligado na importação e material aditivo/translúcido não usa Nanite: a malha cairia no "fallback"
    # reduzido (núcleo 332 triângulos em vez de 960)
    ns = m.get_editor_property("nanite_settings")
    if ns.enabled:
        ns.enabled = False
        m.set_editor_property("nanite_settings", ns)
    b = m.get_bounding_box()
    sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    print("%s: min %s max %s | verts %d | cor de vértice %s" % (name, b.min, b.max, sub.get_number_verts(m, 0),
                                                              sub.has_vertex_colors(m)))
    EAL.save_loaded_asset(m, False)
print("GOKAKYU_V2_IMPORT_OK")
