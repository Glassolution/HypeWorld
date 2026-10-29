"""Importa as animações do Mixamo (Desktop/Hype World/Mixamo) no esqueleto do X Bot.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/importar_mixamo.py
"""
import os
import re
import unreal

ROOT = "/Users/luisfelipexavier/Desktop/Hype World/Mixamo"
DEST = "/Game/Ninja/Characters/Mixamo/Anims"
SKELETON = unreal.load_asset("/Game/Ninja/Characters/Mixamo/SK_XBot_Skeleton")


def asset_name(file_name, folder):
    base = os.path.splitext(file_name)[0]
    base = re.sub(r"\((\d+)\)", r"\1", base)                      # "idle (2)" -> "idle 2"
    words = re.sub(r"[^A-Za-z0-9 ]", "", base).split()
    prefix = "A_MX_" if folder == "Pack_ActionAdventure" else "A_MX_Avulsa_"
    return prefix + "_".join(w.capitalize() for w in words)


tasks = []
for folder in ("Pack_ActionAdventure", "Avulsas"):
    for f in sorted(os.listdir(os.path.join(ROOT, folder))):
        if not f.lower().endswith(".fbx") or f == "X Bot.fbx":
            continue
        pipe = unreal.InterchangeGenericAssetsPipeline()
        common = pipe.get_editor_property("common_skeletal_meshes_and_animations_properties")
        common.set_editor_property("skeleton", SKELETON)
        common.set_editor_property("import_only_animations", True)
        pipe.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
        t = unreal.AssetImportTask()
        t.filename = os.path.join(ROOT, folder, f)
        t.destination_path = DEST
        t.destination_name = asset_name(f, folder)
        t.automated = True
        t.replace_existing = True
        t.save = True
        stack = unreal.InterchangePipelineStackOverride()  # nesta versão as opções do Interchange vão dentro deste pacote
        stack.add_pipeline(pipe)
        t.options = stack
        tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for t in tasks:
    anims = [p for p in t.imported_object_paths if unreal.load_asset(p).get_class().get_name() == "AnimSequence"]
    others = [p.split(".")[-1] for p in t.imported_object_paths if p not in anims]
    seq = unreal.load_asset(anims[0]) if anims else None
    print("%-34s %s  %s" % (t.destination_name, ("%.2fs" % seq.get_play_length()) if seq else "FALHOU", ("extras: %s" % others) if others else ""))
