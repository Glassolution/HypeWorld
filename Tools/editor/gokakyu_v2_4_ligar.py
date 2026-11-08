"""Gōkakyū v2 (08/11), passo 4: liga o v2 no DA_Katon_Goukakyuu e no BP_Katon_Goukakyuu.

Backup do v1: /Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/Backup_v1_0811/ e Docs/Jutsus/Katon/GokakyuV2/backup_valores.json.
Para voltar ao v1: unreal._gokakyu_v2_voltar = True antes de rodar (religa os sistemas/valores do backup).
Timing (skill, seção 5): bola cresce em 0,3 s na frente da boca (GrowTime), segura até 0,32 s (HoldTime) e dispara
com aceleração forte (AccelTime 0,15, começa a 25% da velocidade).
"""
import json
import os
import unreal

EAL = unreal.EditorAssetLibrary
KATON = "/Game/HypeWorld/Jutsus/Katon"
V2 = "/Game/HypeWorld/VFX/Katon/"
da = unreal.load_asset(KATON + "/DA_Katon_Goukakyuu")
bp = unreal.load_asset(KATON + "/BP_Katon_Goukakyuu")
cdo = unreal.get_default_object(bp.generated_class())

if getattr(unreal, "_gokakyu_v2_voltar", False):
    proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    with open(os.path.join(proj, "Docs/Jutsus/Katon/GokakyuV2/backup_valores.json"), encoding="utf-8") as f:
        v = json.load(f)
    for k, val in v["DA"].items():
        if k.endswith(("_system", "_material")):
            da.set_editor_property(k, unreal.load_asset(val.split(".")[0]) if val else None)
    for k in ("grow_time", "gokakyu_hold_time", "accel_time", "launch_speed_scale", "gokakyu_show_meshes",
              "gokakyu_shell_scales", "gokakyu_shell_spins", "gokakyu_shell_stretch", "gokakyu_tongue",
              "gokakyu_pulse", "gokakyu_wobble", "gokakyu_size"):
        cdo.set_editor_property(k, v["BP"][k])
    cdo.set_editor_property("gokakyu_core_mesh", None)
    cdo.set_editor_property("gokakyu_shell_mesh", None)
    print("VOLTOU PARA O V1")
else:
    da.set_editor_property("gokakyu_core_material", unreal.load_asset(V2 + "Materiais/MI_Katon_Nucleo"))
    da.set_editor_property("gokakyu_shell_material", unreal.load_asset(V2 + "Materiais/MI_Katon_Casca"))
    da.set_editor_property("gokakyu_charge_system", None)          # nada nos selos: tudo nasce no cuspe
    da.set_editor_property("goukakyuu_exit_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2_Carga"))
    da.set_editor_property("goukakyuu_flight_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2"))
    da.set_editor_property("goukakyuu_trail_system", None)
    da.set_editor_property("goukakyuu_impact_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2_Impacto"))
    cdo.set_editor_property("gokakyu_core_mesh", unreal.load_asset(V2 + "Meshes/SM_Katon_Core"))
    cdo.set_editor_property("gokakyu_shell_mesh", unreal.load_asset(V2 + "Meshes/SM_Katon_Shell"))
    cdo.set_editor_property("gokakyu_show_meshes", True)
    cdo.set_editor_property("grow_time", 0.3)
    cdo.set_editor_property("gokakyu_hold_time", 0.32)
    cdo.set_editor_property("accel_time", 0.15)
    cdo.set_editor_property("launch_speed_scale", 0.25)
    # tamanho VISUAL = 60% do raio de colisão (155 cm): as cascas de 2 m engoliam o personagem
    cdo.set_editor_property("gokakyu_size", 0.6)
    cdo.set_editor_property("gokakyu_shell_scales", [1.08, 1.22])
    cdo.set_editor_property("gokakyu_shell_spins", [200.0, -150.0])   # graus/s: 0,55 e 0,42 volta/s, sentidos opostos
    cdo.set_editor_property("gokakyu_shell_stretch", 1.15)
    cdo.set_editor_property("gokakyu_tongue", 0.0)
    cdo.set_editor_property("gokakyu_pulse", 0.03)
    cdo.set_editor_property("gokakyu_wobble", 8.0)
    print("V2 LIGADO")
da.modify(True)
EAL.save_loaded_asset(da, False)
# O BP só copia para o ator, no spawn, as propriedades que estavam DIFERENTES do padrão do C++ na última compilação
# (lista otimizada do Blueprint). Sem compilar, um valor novo num campo que antes era igual ao padrão (ex.:
# GokakyuSize 1 -> 0,6) é ignorado no jogo. Compila e confere se a compilação manteve os valores.
antes = {k: cdo.get_editor_property(k) for k in ("gokakyu_size", "gokakyu_hold_time", "grow_time", "gokakyu_core_mesh",
                                                   "gokakyu_shell_mesh", "gokakyu_shell_scales", "gokakyu_show_meshes",
                                                   "accel_time", "launch_speed_scale", "gokakyu_tongue")}
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo = unreal.get_default_object(bp.generated_class())
for k, v in antes.items():
    if cdo.get_editor_property(k) != v:
        print("compilação mudou", k, "-> recolocando")
        cdo.set_editor_property(k, v)
bp.modify(True)
EAL.save_loaded_asset(bp, False)
for k in ("goukakyuu_exit_system", "goukakyuu_flight_system", "goukakyuu_impact_system", "gokakyu_core_material",
          "gokakyu_shell_material"):
    a = da.get_editor_property(k)
    print("DA", k, a.get_name() if a else None)
for k in ("gokakyu_core_mesh", "gokakyu_shell_mesh", "grow_time", "gokakyu_hold_time", "accel_time",
          "gokakyu_show_meshes"):
    a = cdo.get_editor_property(k)
    print("BP", k, a.get_name() if isinstance(a, unreal.Object) else a)
