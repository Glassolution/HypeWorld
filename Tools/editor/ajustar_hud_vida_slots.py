"""HUD (25/10/2026, pedido do Felipe): bloco de vida menor e mais para dentro da tela (direita e para cima), nome
"Felipe Utirra" e slots de jutsu menores e todos vazios (ainda não existe nenhuma habilidade).
Rodar com ue_exec -f, editor fora do Play. Para voltar a arte nos slots: Tools/editor/montar_slots_jutsu.py.
"""
import unreal

bp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("display_name", "Felipe Utirra")
cdo.set_editor_property("life_block_scale", 0.17)                        # era 0,18; 0,15 ficou pequeno demais
cdo.set_editor_property("life_block_position", unreal.Vector2D(60.0, -60.0))   # era (24, -22): canto de baixo à esquerda
cdo.set_editor_property("jutsu_slot_data", [])                           # todos vazios
cdo.set_editor_property("jutsu_slot_size", 42.0)                         # era 50
cdo.set_editor_property("jutsu_slot_gap", 20.0)                          # centro a centro 62 (era 74)
cdo.set_editor_property("jutsu_hidden_offset", 92.0)                     # era 110 (acompanha o tamanho)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
print("HUD:", cdo.get_editor_property("display_name"), cdo.get_editor_property("life_block_scale"),
      cdo.get_editor_property("life_block_position"), len(cdo.get_editor_property("jutsu_slot_data")), cdo.get_editor_property("jutsu_slot_size"))
