"""Importa um FBX do DeepMotion (SayMotion/Animate 3D) com o boneco dele (malha + esqueleto + animação).
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/importar_deepmotion.py
(edite FBX e NAME abaixo para cada animação nova)
"""
import unreal

FBX = "/Users/luisfelipexavier/Desktop/Hype World/Mixamo/Captura/dash_estrela/A-ninja-sprints_pWMyN6Kn45PtHhNJFC4xN5_v1(includeTPose).fbx"
DEST = "/Game/Ninja/Characters/DeepMotion"
NAME = "DM_DashEstrela"

pipe = unreal.InterchangeGenericAssetsPipeline()
pipe.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
# DeepMotion exporta a 30 quadros/s; sem isso o Interchange tenta 12 fps e recusa a animação
anim_pipe = pipe.get_editor_property("animation_pipeline")
anim_pipe.set_editor_property("use30_hz_to_bake_bone_animation", True)
# duração 2,667 s não cai exatamente num quadro: encaixa no quadro mais próximo
anim_pipe.set_editor_property("frame_alignment", unreal.InterchangeFrameAlignment.SNAP_TO_CLOSEST)
t = unreal.AssetImportTask()
t.filename = FBX
t.destination_path = DEST
t.destination_name = NAME
t.automated = True
t.replace_existing = True
t.save = True
stack = unreal.InterchangePipelineStackOverride()
stack.add_pipeline(pipe)
t.options = stack
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
for p in t.imported_object_paths:
    a = unreal.load_asset(p)
    extra = ""
    if isinstance(a, unreal.AnimSequence):
        extra = "%.2fs %d quadros" % (a.get_play_length(), a.get_editor_property("number_of_sampled_frames"))
    print(a.get_class().get_name(), p, extra)

# Segunda passada: só a animação, no esqueleto criado acima (com o mesmo nome do modelo ela era descartada)
skeleton = unreal.load_asset(f"{DEST}/{NAME}_Skeleton")
pipe = unreal.InterchangeGenericAssetsPipeline()
common = pipe.get_editor_property("common_skeletal_meshes_and_animations_properties")
common.set_editor_property("skeleton", skeleton)
common.set_editor_property("import_only_animations", True)
pipe.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
anim_pipe = pipe.get_editor_property("animation_pipeline")
anim_pipe.set_editor_property("use30_hz_to_bake_bone_animation", True)
anim_pipe.set_editor_property("frame_alignment", unreal.InterchangeFrameAlignment.SNAP_TO_CLOSEST)
t = unreal.AssetImportTask()
t.filename = FBX
t.destination_path = DEST
t.destination_name = "A_" + NAME
t.automated = True
t.replace_existing = True
t.save = True
stack = unreal.InterchangePipelineStackOverride()
stack.add_pipeline(pipe)
t.options = stack
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
for p in t.imported_object_paths:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.AnimSequence):
        print("animação:", p, "%.2fs %d quadros" % (a.get_play_length(), a.get_editor_property("number_of_sampled_frames")))
