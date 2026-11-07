"""Gōkakyū novo (07/11): liga o visual anime no DA_Katon_Goukakyuu e ajusta o BP do projétil.

- DA: GokakyuCoreMaterial = MI_FireToon_Nucleo, GokakyuShellMaterial = MI_FireToon_Casca,
      saída = NS_Katon_Gokakyu_Saida, voo = NS_Katon_Gokakyu, rastro antigo = nenhum.
- BP_Katon_Goukakyuu: a bola cresce rápido nos primeiros 0,2 s (GrowTime).
Os sistemas antigos (NS_Katon_Goukakyuu_Saida/Voo/Rastro) continuam no projeto: para voltar, rodar com
unreal._gokakyu_voltar = True antes (religa os antigos e tira os materiais novos).
07/11 (2ª especificação): padrão = JATO contínuo (NS_Katon_Gokakyu_Jato/_Saida/_Impacto, sem esferas de malha; a luz
segue a cabeça). unreal._gokakyu_modo = "Bola" volta para a bola de malhas + NS_Katon_Gokakyu (backup).
"""
import unreal

EAL = unreal.EditorAssetLibrary
DA = "/Game/HypeWorld/Jutsus/Katon/DA_Katon_Goukakyuu"
BP = "/Game/HypeWorld/Jutsus/Katon/BP_Katon_Goukakyuu"
NOVO = "/Game/HypeWorld/Jutsus/Katon/VFX/Gokakyu/"
ANTIGO = "/Game/HypeWorld/Jutsus/Katon/VFX/Goukakyuu/"
voltar = getattr(unreal, "_gokakyu_voltar", False)
ESTILO = getattr(unreal, "_gokakyu_estilo", "Soft")   # "Soft" = estilo Zenkai; "Toon" = contorno nítido
MODO = getattr(unreal, "_gokakyu_modo", "Jato")       # "Jato" = jato contínuo; "Bola" = bola de malhas (backup)
IMPACTO_PACK = "/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Floor_01_Smoke"   # impacto de antes

da = unreal.load_asset(DA)
if da is None:
    found = [a for a in EAL.list_assets("/Game/HypeWorld/Jutsus", True) if a.split(".")[0].endswith("DA_Katon_Goukakyuu")]
    da = unreal.load_asset(found[0])
assert da, DA
if voltar:
    da.set_editor_property("gokakyu_core_material", None)
    da.set_editor_property("gokakyu_shell_material", None)
    da.set_editor_property("goukakyuu_exit_system", unreal.load_asset(ANTIGO + "NS_Katon_Goukakyuu_Saida"))
    da.set_editor_property("goukakyuu_flight_system", unreal.load_asset(ANTIGO + "NS_Katon_Goukakyuu_Voo"))
    da.set_editor_property("goukakyuu_trail_system", unreal.load_asset(ANTIGO + "NS_Katon_Goukakyuu_Rastro"))
    da.set_editor_property("goukakyuu_impact_system", unreal.load_asset(IMPACTO_PACK))
elif MODO == "Jato":
    # o material do núcleo só liga o modo novo (luz da cabeça); as esferas não aparecem (GokakyuShowMeshes = false)
    da.set_editor_property("gokakyu_core_material", unreal.load_asset(NOVO + f"MI_Fire{ESTILO}_Nucleo"))
    da.set_editor_property("gokakyu_shell_material", None)
    da.set_editor_property("goukakyuu_exit_system", unreal.load_asset(NOVO + "NS_Katon_Gokakyu_Jato_Saida"))
    da.set_editor_property("goukakyuu_flight_system", unreal.load_asset(NOVO + "NS_Katon_Gokakyu_Jato"))
    da.set_editor_property("goukakyuu_trail_system", None)
    da.set_editor_property("goukakyuu_impact_system", unreal.load_asset(NOVO + "NS_Katon_Gokakyu_Jato_Impacto"))
else:
    da.set_editor_property("gokakyu_core_material", unreal.load_asset(NOVO + f"MI_Fire{ESTILO}_Nucleo"))
    da.set_editor_property("gokakyu_shell_material", unreal.load_asset(NOVO + f"MI_Fire{ESTILO}_Casca"))
    da.set_editor_property("goukakyuu_exit_system", unreal.load_asset(NOVO + "NS_Katon_Gokakyu_Saida"))
    da.set_editor_property("goukakyuu_flight_system", unreal.load_asset(NOVO + "NS_Katon_Gokakyu"))
    da.set_editor_property("goukakyuu_trail_system", None)
    da.set_editor_property("goukakyuu_impact_system", unreal.load_asset(IMPACTO_PACK))
EAL.save_loaded_asset(da, False)

bp = unreal.load_asset(BP)
cdo = unreal.get_default_object(bp.generated_class())
print("antes: GrowTime", cdo.get_editor_property("grow_time"), "StartRadius", cdo.get_editor_property("start_radius"),
      "FullRadius", cdo.get_editor_property("full_radius"), "CastShake(DA)", da.get_editor_property("cast_shake"),
      "Speed(DA)", da.get_editor_property("projectile_speed"))
cdo.set_editor_property("grow_time", 0.25 if voltar else 0.2)   # antes: 0,25
if not voltar:
    # 2 cascas mais finas (3 cobriam o núcleo branco pela câmera de trás) e línguas maiores
    cdo.set_editor_property("gokakyu_shell_scales", [1.12, 1.32])
    cdo.set_editor_property("gokakyu_shell_spins", [200.0, -150.0])
    # suave: línguas menores (deslocar muito a esfera simples mostrava as faces retas, cara de cristal)
    cdo.set_editor_property("gokakyu_tongue", 0.45 if ESTILO == "Toon" else 0.2)
    # luz FORTE como no Zenkai (pedido do Felipe 07/11): laranja #FF8A2A iluminando bem o chão, tremulação lenta
    # (sem piscar) e pico maior no lançamento. Valores originais do C++: voo 900 / raio 7 / tremor 0,25; saída 2600 / 1000
    print("luz antes: voo", cdo.get_editor_property("gokakyu_light_intensity"),
          "saída", cdo.get_editor_property("goukakyuu_exit_light_intensity"))
    # #FF8A2A puro (1, 0,254, 0,023) na grama verde virava amarelo-esverdeado: menos verde na luz = laranja no chão
    laranja = unreal.LinearColor(1.0, 0.16, 0.022, 1.0)
    # medido de cima: 1000-1400 (e até 150) estouravam o chão em creme-branco; 60 com raio 12x tinge de laranja largo
    cdo.set_editor_property("gokakyu_light_intensity", 70.0)
    cdo.set_editor_property("gokakyu_light_radius", 12.0)
    cdo.set_editor_property("gokakyu_light_flicker", 0.18)
    cdo.set_editor_property("gokakyu_light_flicker_rate", 3.0)
    cdo.set_editor_property("gokakyu_light_color", laranja)
    cdo.set_editor_property("goukakyuu_exit_light_intensity", 40.0)   # 90 estourava a árvore perto da boca em amarelo
    cdo.set_editor_property("goukakyuu_exit_light_radius", 1300.0)
    cdo.set_editor_property("goukakyuu_exit_light_color", laranja)    # antes (1, 0,1, 0,015) = vermelho
    cdo.set_editor_property("gokakyu_show_meshes", MODO != "Jato")
    cdo.set_editor_property("gokakyu_light_launch_peak", 0.7)
    cdo.set_editor_property("gokakyu_impact_light", 400.0)
    cdo.set_editor_property("gokakyu_impact_light_radius", 2200.0)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_loaded_asset(bp, False)
print("GOKAKYU_LIGADO" if not voltar else "GOKAKYU_VOLTOU_ANTIGO", da.get_path_name())
