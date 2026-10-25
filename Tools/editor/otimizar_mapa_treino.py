"""Lvl_Treino mais leve (25/10/2026, pedido do Felipe: "otimize esse mapa"). Nada é apagado:
- Galerias de animação (pastas Galeria/*: 47 bonecos animando em loop + 49 textos): escondidas no JOGO (no editor continuam
  visíveis) e sem animar quando não aparecem na tela. Para voltar: GALERIA_NO_JOGO = True antes do código.
- Nuvens volumétricas (VolumetricCloud): menos amostras (mais baratas, quase o mesmo visual).
Também deixa o bloco de vida do HUD um pouco maior (pedido do Felipe). Rodar com ue_exec -f, editor fora do Play.
"""
import unreal

try:
    GALERIA_NO_JOGO
except NameError:
    GALERIA_NO_JOGO = False

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n_gal = 0
for a in eas.get_all_level_actors():
    pasta = str(a.get_folder_path())
    if pasta.startswith("Galeria"):
        a.set_actor_hidden_in_game(not GALERIA_NO_JOGO)
        for comp in a.get_components_by_class(unreal.SkeletalMeshComponent):
            comp.set_editor_property("visibility_based_anim_tick_option",
                                     unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE if GALERIA_NO_JOGO
                                     else unreal.VisibilityBasedAnimTickOption.ONLY_TICK_POSE_WHEN_RENDERED)
        n_gal += 1
    if isinstance(a, unreal.VolumetricCloud):
        nuvem = a.get_component_by_class(unreal.VolumetricCloudComponent)
        nuvem.set_editor_property("view_sample_count_scale", 0.35)
        nuvem.set_editor_property("reflection_view_sample_count_scale_value", 0.25)
        nuvem.set_editor_property("shadow_view_sample_count_scale", 0.35)
        nuvem.set_editor_property("shadow_reflection_view_sample_count_scale_value", 0.25)
print("galeria:", n_gal, "atores", "no jogo" if GALERIA_NO_JOGO else "escondidos no jogo")
print("salvo mapa:", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())

bp = unreal.load_asset("/Game/Ninja/UI/WBP_NinjaHUD")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("life_block_scale", 0.19)   # 0,15 pequeno demais, 0,17 ainda pequeno
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
print("vida escala:", cdo.get_editor_property("life_block_scale"))
