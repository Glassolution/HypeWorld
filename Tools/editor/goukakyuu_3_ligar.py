"""Gōkakyū, etapa 3: liga a saída, o voo e o rastro (goukakyuu_2_niagara.py) no DA_Katon_Goukakyuu.
Com esses dois campos preenchidos, o projétil esconde as esferas do protótipo durante o voo.
O impacto continua o mesmo por enquanto (próxima etapa do relatório). Pode rodar de novo.
"""
import unreal

EAL = unreal.EditorAssetLibrary
VFX = "/Game/HypeWorld/Jutsus/Katon/VFX/Goukakyuu"
DA = "/Game/HypeWorld/Jutsus/Katon/DA_Katon_Goukakyuu"

jutsu = unreal.load_asset(DA)
jutsu.set_editor_property("goukakyuu_exit_system", unreal.load_asset(f"{VFX}/NS_Katon_Goukakyuu_Saida"))
jutsu.set_editor_property("goukakyuu_flight_system", unreal.load_asset(f"{VFX}/NS_Katon_Goukakyuu_Voo"))
# 06/11: 2º sistema preso à bola (casca vermelha + rastro com vento + fumaça)
jutsu.set_editor_property("goukakyuu_trail_system", unreal.load_asset(f"{VFX}/NS_Katon_Goukakyuu_Rastro"))
assert EAL.save_loaded_asset(jutsu, False)
print("GOUKAKYUU_LIGADO", jutsu.get_editor_property("goukakyuu_exit_system").get_name(),
      jutsu.get_editor_property("goukakyuu_flight_system").get_name(),
      jutsu.get_editor_property("goukakyuu_trail_system").get_name())
