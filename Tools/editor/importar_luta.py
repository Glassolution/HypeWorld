"""Importa as animações de luta (Desktop/Hype World/Mixamo/Luta) e converte para o manequim.
Mesmo caminho das outras do Mixamo: esqueleto do X Bot -> RTG_XBot_Para_Manequim -> A_Ninja_MX_Luta_*.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/importar_luta.py
"""
import os
import re
import unreal

ROOT = "/Users/luisfelipexavier/Desktop/Hype World/Mixamo/Luta"
DEST = "/Game/Ninja/Characters/Mixamo/Anims/Luta"
OUT = "/Game/Ninja/Characters/Animations/Mixamo/Luta"
SKELETON = unreal.load_asset("/Game/Ninja/Characters/Mixamo/SK_XBot_Skeleton")
XBOT = unreal.load_asset("/Game/Ninja/Characters/Mixamo/SK_XBot")
QUINN = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
RTG = unreal.load_asset("/Game/Ninja/Characters/Retarget/RTG_XBot_Para_Manequim")


def asset_name(file_name):
    base = os.path.splitext(file_name)[0]
    base = re.sub(r"\((\d+)\)", r"\1", base)                      # "Mma Kick (1)" -> "Mma Kick 1"
    words = re.sub(r"[^A-Za-z0-9 ]", "", base).split()
    return "A_MX_Luta_" + "_".join(w.capitalize() for w in words)


tasks = []
for f in sorted(os.listdir(ROOT)):
    if not f.lower().endswith(".fbx"):
        continue
    pipe = unreal.InterchangeGenericAssetsPipeline()
    common = pipe.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("skeleton", SKELETON)
    common.set_editor_property("import_only_animations", True)
    pipe.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, f)
    t.destination_path = DEST
    t.destination_name = asset_name(f)
    t.automated = True
    t.replace_existing = True
    t.save = True
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(pipe)
    t.options = stack
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

sources = []
for t in tasks:
    anims = [p for p in t.imported_object_paths if unreal.load_asset(p).get_class().get_name() == "AnimSequence"]
    others = [p.split(".")[-1] for p in t.imported_object_paths if p not in anims]
    seq = unreal.load_asset(anims[0]) if anims else None
    print("%-30s %s  %s" % (t.destination_name, ("%.2fs" % seq.get_play_length()) if seq else "FALHOU", ("extras: %s" % others) if others else ""))
    if seq:
        sources.append(unreal.AssetRegistryHelpers.create_asset_data(seq))

inputs = unreal.IKRetargetBatchOperationInputs()
inputs.set_editor_property("assets_to_retarget", sources)
inputs.set_editor_property("source_mesh", XBOT)
inputs.set_editor_property("target_mesh", QUINN)
inputs.set_editor_property("ik_retarget_asset", RTG)
inputs.set_editor_property("search", "A_MX_")
inputs.set_editor_property("replace", "A_Ninja_MX_")
inputs.set_editor_property("target_path", OUT)
inputs.set_editor_property("include_referenced_assets", False)
inputs.set_editor_property("overwrite_existing_files", True)
result = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
for a in result:
    seq = unreal.load_asset(str(a.package_name))
    seq.set_editor_property("enable_root_motion", True)   # o avanço do golpe vira movimento do personagem (montagens de ataque)
    unreal.EditorAssetLibrary.save_asset(str(a.package_name), only_if_is_dirty=False)
    print("convertida:", a.asset_name, "%.2fs" % seq.get_play_length(), "%d frames" % seq.get_editor_property("number_of_sampled_frames"))
