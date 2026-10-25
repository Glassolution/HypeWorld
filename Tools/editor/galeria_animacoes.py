"""Monta uma galeria no mapa de treino: um boneco por animação, em loop, com o nome em cima (para avaliar qualidade).
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/galeria_animacoes.py   (ajuste PASTA/PREFIXO/ORIGEM)
"""
import unreal

PASTA = "/Game/Ninja/Characters/Packs/Motifect_Locomotion/Anims"
PREFIXO = "A_MF_"
FOLDER = "Galeria/Motifect_Locomotion"
ORIGEM = unreal.Vector(-2600, -1100, 0)   # área livre atrás do ponto de início, de frente para ele
COLUNAS, PASSO_X, PASSO_Y = 9, 330, 275
MESH = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if str(a.get_folder_path()) == FOLDER:
        eas.destroy_actor(a)

ar = unreal.AssetRegistryHelpers.get_asset_registry()
anims = sorted([a for a in ar.get_assets_by_path(PASTA, recursive=False) if str(a.asset_class_path.asset_name) == "AnimSequence"], key=lambda a: str(a.asset_name))
for i, data in enumerate(anims):
    anim = unreal.load_asset(str(data.package_name))
    row, col = divmod(i, COLUNAS)
    loc = ORIGEM + unreal.Vector(row * PASSO_X, col * PASSO_Y, 0)
    actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    nome = str(data.asset_name).replace(PREFIXO, "")
    actor.set_actor_label("Galeria_" + nome)
    actor.set_folder_path(FOLDER)
    comp = actor.skeletal_mesh_component
    comp.set_skeletal_mesh_asset(MESH)
    comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    play = unreal.SingleAnimationPlayData()
    play.set_editor_property("anim_to_play", anim)
    play.set_editor_property("saved_looping", True)
    play.set_editor_property("saved_playing", True)
    comp.set_editor_property("animation_data", play)
    label = eas.spawn_actor_from_class(unreal.TextRenderActor, loc + unreal.Vector(0, 0, 215), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    label.set_actor_label("GaleriaNome_" + nome)
    label.set_folder_path(FOLDER)
    tr = label.text_render
    tr.set_text("%d. %s" % (i + 1, nome))
    tr.set_editor_property("world_size", 22.0)
    tr.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    tr.set_text_render_color(unreal.Color(r=255, g=230, b=120, a=255))
print("bonecos na galeria:", len(anims))
unreal.EditorLoadingAndSavingUtils.save_current_level()
print("mapa salvo")
