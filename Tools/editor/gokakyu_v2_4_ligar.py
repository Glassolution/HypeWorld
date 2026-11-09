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
    for k in ("gokakyu_drag", "gokakyu_flight_grow", "gokakyu_wander", "launch_bend"):
        cdo.set_editor_property(k, 0.0)
    for k in ("full_radius", "gokakyu_core_scale", "gokakyu_light_radius", "bend_time"):
        if k in v["BP"]:
            cdo.set_editor_property(k, v["BP"][k])
    cdo.set_editor_property("tail_time", v["BP"].get("tail_time", 0.45))
    print("VOLTOU PARA O V1")
else:
    # valores do v2 aprovado (cores/luz) antes do passe "fogo vivo" de 08/11, para comparar/voltar à mão
    proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
    out2 = os.path.join(proj, "Docs/Jutsus/Katon/GokakyuV2/backup_valores_v2.json")
    if not os.path.exists(out2):
        keys = ("full_radius", "gokakyu_core_scale", "gokakyu_shell_scales", "gokakyu_shell_spins",
                "gokakyu_shell_stretch", "accel_time", "launch_speed_scale", "gokakyu_light_radius", "launch_bend",
                "bend_time", "collision_radius_scale")
        with open(out2, "w", encoding="utf-8") as f:
            json.dump({k: (list(v) if isinstance(v, unreal.Array) else v) for k, v in
                       ((k, cdo.get_editor_property(k)) for k in keys)}, f, indent=1)
    da.set_editor_property("gokakyu_core_material", unreal.load_asset(V2 + "Materiais/MI_Katon_Nucleo"))
    # 08/11 "fogo vivo": as cascas do C++ viram LÍNGUAS DE FOGO em malha (SM_Katon_Linguas, apontando para trás)
    da.set_editor_property("gokakyu_shell_material", unreal.load_asset(V2 + "Materiais/MI_Katon_LinguaMalha"))
    da.set_editor_property("gokakyu_charge_system", None)          # nada nos selos: tudo nasce no cuspe
    da.set_editor_property("goukakyuu_exit_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2_Carga"))
    da.set_editor_property("goukakyuu_flight_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2_Voo"))
    da.set_editor_property("goukakyuu_trail_system", None)
    da.set_editor_property("goukakyuu_impact_system", unreal.load_asset(V2 + "Niagara/NS_Katon_Gokakyu_v2_Impacto"))
    cdo.set_editor_property("gokakyu_core_mesh", unreal.load_asset(V2 + "Meshes/SM_Katon_Core"))
    cdo.set_editor_property("gokakyu_shell_mesh", unreal.load_asset(V2 + "Meshes/SM_Katon_Linguas"))
    cdo.set_editor_property("gokakyu_show_meshes", True)
    cdo.set_editor_property("grow_time", 0.3)
    cdo.set_editor_property("gokakyu_hold_time", 0.32)    # timing da carga (aprovado): solta em 0,32 s
    # 09/11 trava: no quadro da soltura a bola já sai com a velocidade máxima (Speed 3000), sem rampa
    cdo.set_editor_property("accel_time", 0.01)
    cdo.set_editor_property("launch_speed_scale", 1.0)
    # pico de luz no instante da soltura (C++ 09/11: curto, ~0,1 s; antes o pico era no cuspe)
    cdo.set_editor_property("gokakyu_light_launch_peak", 1.5)
    # física do voo (C++ 08/11): perde ~30% da velocidade em 1 s, cresce 10% no voo, arco suave + desvio lento
    cdo.set_editor_property("gokakyu_drag", 0.35)
    cdo.set_editor_property("gokakyu_flight_grow", 0.1)
    cdo.set_editor_property("gokakyu_flight_grow_time", 1.0)
    cdo.set_editor_property("gokakyu_wander", 35.0)
    cdo.set_editor_property("gokakyu_wander_rate", 0.6)
    cdo.set_editor_property("launch_bend", 70.0)
    cdo.set_editor_property("bend_time", 1.6)
    # tamanho: fogo com ~2,5-3x a altura do personagem; colisão/dano acompanham o raio (155 -> 210)
    cdo.set_editor_property("full_radius", 210.0)
    cdo.set_editor_property("gokakyu_size", 0.6)
    cdo.set_editor_property("gokakyu_core_scale", 0.55)   # núcleo menor, escondido dentro do fogo (as labaredas fazem a silhueta)
    # luz igual à aprovada: o raio da luz é proporcional ao raio da bola (155/210)
    cdo.set_editor_property("gokakyu_light_radius", 12.0 * 155.0 / 210.0)
    # 09/11: SEM as línguas rígidas de malha (pareciam coroa/ouriço): a silhueta fica com as labaredas em flipbook,
    # que mudam de forma, se soltam e apagam como no Zenkai. Lista vazia = o C++ não cria cascas
    cdo.set_editor_property("gokakyu_shell_scales", [])
    cdo.set_editor_property("gokakyu_shell_spins", [70.0, -55.0])     # graus/s: línguas girando devagar, sentidos opostos
    cdo.set_editor_property("gokakyu_shell_stretch", 1.0)
    cdo.set_editor_property("gokakyu_tongue", 0.0)
    cdo.set_editor_property("gokakyu_pulse", 0.03)
    cdo.set_editor_property("gokakyu_wobble", 8.0)
    # 09/11: sem o cone antigo boca -> bola (na câmera do jogo virava um risco fino da boca até a bola até 0,45 s)
    cdo.set_editor_property("tail_time", 0.0)
    print("V2 LIGADO")
da.modify(True)
EAL.save_loaded_asset(da, False)
# O BP só copia para o ator, no spawn, as propriedades que estavam DIFERENTES do padrão do C++ na última compilação
# (lista otimizada do Blueprint). Sem compilar, um valor novo num campo que antes era igual ao padrão (ex.:
# GokakyuSize 1 -> 0,6) é ignorado no jogo. Compila e confere se a compilação manteve os valores.
antes = {k: cdo.get_editor_property(k) for k in ("gokakyu_drag", "gokakyu_flight_grow", "gokakyu_flight_grow_time",
                                                   "gokakyu_wander", "gokakyu_wander_rate", "launch_bend", "bend_time",
                                                   "full_radius", "gokakyu_core_scale", "gokakyu_light_radius",
                                                   "gokakyu_shell_spins", "gokakyu_shell_stretch",
                                                   "gokakyu_size", "gokakyu_hold_time", "grow_time", "gokakyu_core_mesh",
                                                   "gokakyu_shell_mesh", "gokakyu_shell_scales", "gokakyu_show_meshes",
                                                   "accel_time", "launch_speed_scale", "gokakyu_tongue", "gokakyu_light_launch_peak", "tail_time")}
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
