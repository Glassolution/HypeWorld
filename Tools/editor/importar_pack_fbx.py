"""Importa um pack de animações FBX com esqueleto próprio (Fab/Motifect etc.) e converte para o manequim.
1) o primeiro FBX entra com a malha (cria o esqueleto do pack); 2) todos entram só como animação nesse esqueleto;
3) IK Rig automático do pack + IK Retargeter para o manequim (mesma receita do Mixamo/DeepMotion) e conversão em lote.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/importar_pack_fbx.py   (ajuste PACK abaixo)
"""
import os
import unreal

PACK = "Motifect_Locomotion"
SRC_DIR = "/Users/luisfelipexavier/Desktop/Hype World/Fab/motifect_locomotion/Animations"
FIRST = "run_jog.fbx"
RAW = f"/Game/Ninja/Characters/Packs/{PACK}/Raw"
OUT = f"/Game/Ninja/Characters/Packs/{PACK}/Anims"
RIGS = "/Game/Ninja/Characters/Retarget"
QUINN = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
TGT_RIG = unreal.load_asset(f"{RIGS}/IK_Manequim")
tools = unreal.AssetToolsHelpers.get_asset_tools()
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET


def task(fbx, name, skeleton=None):
    pipe = unreal.InterchangeGenericAssetsPipeline()
    pipe.get_editor_property("material_pipeline").set_editor_property("import_materials", False)
    anim = pipe.get_editor_property("animation_pipeline")
    anim.set_editor_property("use30_hz_to_bake_bone_animation", True)
    anim.set_editor_property("frame_alignment", unreal.InterchangeFrameAlignment.SNAP_TO_CLOSEST)
    if skeleton:
        common = pipe.get_editor_property("common_skeletal_meshes_and_animations_properties")
        common.set_editor_property("skeleton", skeleton)
        common.set_editor_property("import_only_animations", True)
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC_DIR, fbx)
    t.destination_path = RAW
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    t.save = True
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(pipe)
    t.options = stack
    return t


# 1) Malha + esqueleto do pack
first = task(FIRST, "SK_" + PACK)
tools.import_asset_tasks([first])
mesh = next(unreal.load_asset(p) for p in first.imported_object_paths if isinstance(unreal.load_asset(p), unreal.SkeletalMesh))
skeleton = mesh.get_editor_property("skeleton")
print("malha:", mesh.get_path_name(), "| esqueleto:", skeleton.get_name())

# 2) Todas as animações nesse esqueleto
tasks = [task(f, "A_" + os.path.splitext(f)[0], skeleton) for f in sorted(os.listdir(SRC_DIR)) if f.lower().endswith(".fbx")]
tools.import_asset_tasks(tasks)
anims = []
for t in tasks:
    a = next((unreal.load_asset(p) for p in t.imported_object_paths if isinstance(unreal.load_asset(p), unreal.AnimSequence)), None)
    if a:
        anims.append(a)
    else:
        print("FALHOU:", t.destination_name)
print("animações importadas:", len(anims))

# 3) IK Rig do pack + retargeter para o manequim
def get_or_create(name, cls, factory):
    existing = unreal.load_asset(f"{RIGS}/{name}")
    return existing if existing else tools.create_asset(name, RIGS, cls, factory)

rig = get_or_create("IK_" + PACK, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
c = unreal.IKRigController.get_controller(rig)
c.set_skeletal_mesh(mesh)
c.apply_auto_generated_retarget_definition()
c.apply_auto_fbik()
# Correções da detecção automática: coluna até o peito (Chest) e pescoço com cadeia própria (senão ia para a cabeça)
c.set_retarget_chain_end_bone("Spine", "Chest")
if not any(str(ch.get_editor_property("chain_name")) == "Neck" for ch in c.get_retarget_chains()):
    c.add_retarget_chain("Neck", "Neck1", "Neck2", "")
unreal.EditorAssetLibrary.save_asset(rig.get_path_name().split(".")[0], only_if_is_dirty=False)
print("cadeias:", [(str(ch.get_editor_property("chain_name")), str(ch.get_editor_property("start_bone").get_editor_property("bone_name")), str(ch.get_editor_property("end_bone").get_editor_property("bone_name"))) for ch in c.get_retarget_chains()])
print("raiz:", c.get_retarget_root())

rtg = get_or_create("RTG_" + PACK + "_Para_Manequim", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(S, rig)
rc.set_ik_rig(T, TGT_RIG)
rc.set_preview_mesh(S, mesh)
rc.set_preview_mesh(T, QUINN)
rc.remove_all_ops()
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(T, TGT_RIG)
rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
rc.set_source_chain("Neck", "Neck")
rc.auto_align_all_bones(T)
print("mapa:", [(t, str(rc.get_source_chain(t))) for t in ("Spine", "Neck", "Head", "LeftClavicle", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "LeftThumb", "LeftIndex")])
root_op = rc.get_op_controller(rc.get_index_of_op_by_name("Root Motion"))
settings = root_op.get_settings()
settings.import_text(settings.export_text().replace("RootMotionSource=CopyFromSourceRoot", "RootMotionSource=GenerateFromTargetPelvis").replace("RootHeightSource=CopyHeightFromSource", "RootHeightSource=SnapToGround"))
root_op.set_settings(settings)
pin = rc.get_op_controller(rc.add_retarget_op("/Script/IKRig.IKRetargetPinBoneOp"))
for a, b in [("root", "ik_foot_root"), ("foot_l", "ik_foot_l"), ("foot_r", "ik_foot_r"), ("root", "ik_hand_root"), ("hand_r", "ik_hand_gun"), ("hand_l", "ik_hand_l"), ("hand_r", "ik_hand_r")]:
    pin.set_bone_pair(a, b)
unreal.EditorAssetLibrary.save_asset(rtg.get_path_name().split(".")[0], only_if_is_dirty=False)

inputs = unreal.IKRetargetBatchOperationInputs()
inputs.set_editor_property("assets_to_retarget", [unreal.AssetRegistryHelpers.create_asset_data(a) for a in anims])
inputs.set_editor_property("source_mesh", mesh)
inputs.set_editor_property("target_mesh", QUINN)
inputs.set_editor_property("ik_retarget_asset", rtg)
inputs.set_editor_property("search", "A_")
inputs.set_editor_property("replace", "A_MF_")
inputs.set_editor_property("target_path", OUT)
inputs.set_editor_property("include_referenced_assets", False)
inputs.set_editor_property("overwrite_existing_files", True)
done = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
for a in done:
    seq = unreal.load_asset(str(a.package_name))
    seq.set_editor_property("enable_root_motion", True)
    unreal.EditorAssetLibrary.save_asset(str(a.package_name), only_if_is_dirty=False)
print("convertidas:", len(done))
