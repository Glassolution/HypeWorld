# Registra um traje modular no DT_CustomizationParts (slot Traje). Uso no editor: PERSONAGEM, NOME_MENU e OCULTO definidos
# antes deste arquivo. Pode rodar de novo: atualiza a linha.
import unreal

dt = unreal.load_asset('/Game/HypeWorld/Characters/Parts/DT_CustomizationParts')
antes = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(dt))
mesh = unreal.load_asset('/Game/HypeWorld/Personagens/Indra/Modular/Trajes/%s/SK_Outfit_%s' % (PERSONAGEM, PERSONAGEM))
assert unreal.NinjaEditorTools.set_outfit_catalog_row(dt, 'Outfit_' + PERSONAGEM, NOME_MENU, mesh, OCULTO)
unreal.EditorAssetLibrary.save_loaded_asset(dt, False)
linhas = [str(n) for n in unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)]
unreal.log('CATALOGO %d -> %d linhas; trajes: %s' % (antes, len(linhas), [n for n in linhas if n.startswith('Outfit_')]))
