"""Gōkakyū, etapa 1: clarão da saída (luz parada na boca + bloom), valores no BP_Katon_Goukakyuu.
Felipe (06/11): o brilho tem que ser forte SÓ quando a bola sai; no teste 13 o chão ficava aceso até a bola chegar no
alvo (~0,8 s) -> pico mais forte e curto: forte até 0,1 s, apagado em 0,4 s.
Pode rodar de novo. Rodar com o editor aberto, fora do Play.
"""
import unreal

BP = "/Game/HypeWorld/Jutsus/Katon/BP_Katon_Goukakyuu"
VALORES = {
    "goukakyuu_exit_light_intensity": 2600.0,
    "goukakyuu_exit_light_radius": 1000.0,
    "goukakyuu_exit_hold_time": 0.1,
    "goukakyuu_exit_fade_time": 0.4,
    "goukakyuu_exit_bloom": 1.4,
    "goukakyuu_exit_bloom_threshold": 1.0,
    "goukakyuu_exit_exposure": 0.25,
}

bp = unreal.load_asset(BP)
cdo = unreal.get_default_object(bp.generated_class())
for nome, valor in VALORES.items():
    cdo.set_editor_property(nome, valor)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
cdo = unreal.get_default_object(bp.generated_class())
print("GOUKAKYUU_CLARAO_OK", {n: round(cdo.get_editor_property(n), 3) for n in VALORES})
