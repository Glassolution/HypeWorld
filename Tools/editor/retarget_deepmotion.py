"""Converte animações do DeepMotion (esqueleto hips/spine/leftarm..., sem prefixo) para o manequim do jogo.
Mesma receita do retarget_mixamo.py: root motion gerado do quadril e colado no chão, Pin Bones para ik_foot/ik_hand, dedos não mapeados.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/retarget_deepmotion.py
"""
import unreal

FOLDER = "/Game/Ninja/Characters/Retarget"
SRC_MESH = unreal.load_asset("/Game/Ninja/Characters/DeepMotion/DM_DashEstrela")
QUINN = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
TGT_RIG = unreal.load_asset(f"{FOLDER}/IK_Manequim")
ANIMS = ["/Game/Ninja/Characters/DeepMotion/A_DM_DashEstrela"]
OUT = "/Game/Ninja/Characters/Animations/DeepMotion"
tools = unreal.AssetToolsHelpers.get_asset_tools()
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET


def get_or_create(name, cls, factory):
    existing = unreal.load_asset(f"{FOLDER}/{name}")
    return existing if existing else tools.create_asset(name, FOLDER, cls, factory)


# IK Rig do boneco do DeepMotion (cadeias detectadas sozinhas)
rig = get_or_create("IK_DeepMotion", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
c = unreal.IKRigController.get_controller(rig)
c.set_skeletal_mesh(SRC_MESH)
c.apply_auto_generated_retarget_definition()
c.apply_auto_fbik()
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/IK_DeepMotion", only_if_is_dirty=False)
print("IK_DeepMotion cadeias:", [(str(ch.get_editor_property("chain_name")), str(ch.get_editor_property("start_bone").get_editor_property("bone_name")), str(ch.get_editor_property("end_bone").get_editor_property("bone_name"))) for ch in c.get_retarget_chains()])
print("raiz do retarget:", c.get_retarget_root())

rtg = get_or_create("RTG_DeepMotion_Para_Manequim", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(S, rig)
rc.set_ik_rig(T, TGT_RIG)
rc.set_preview_mesh(S, SRC_MESH)
rc.set_preview_mesh(T, QUINN)
rc.remove_all_ops()
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(T, TGT_RIG)
rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
rc.auto_align_all_bones(T)
for side in ("Left", "Right"):
    for finger in ("Thumb", "Index", "Middle", "Ring", "Pinky"):
        rc.set_source_chain("None", side + finger)
print("mapa:", [(str(t), str(rc.get_source_chain(t))) for t in ("Spine", "Head", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "LeftClavicle", "RightClavicle", "Neck")])

root_op = rc.get_op_controller(rc.get_index_of_op_by_name("Root Motion"))
settings = root_op.get_settings()
txt = settings.export_text().replace("RootMotionSource=CopyFromSourceRoot", "RootMotionSource=GenerateFromTargetPelvis").replace("RootHeightSource=CopyHeightFromSource", "RootHeightSource=SnapToGround")
settings.import_text(txt)
root_op.set_settings(settings)

pin = rc.get_op_controller(rc.add_retarget_op("/Script/IKRig.IKRetargetPinBoneOp"))
for copy_from, copy_to in [("root", "ik_foot_root"), ("foot_l", "ik_foot_l"), ("foot_r", "ik_foot_r"),
                           ("root", "ik_hand_root"), ("hand_r", "ik_hand_gun"), ("hand_l", "ik_hand_l"), ("hand_r", "ik_hand_r")]:
    pin.set_bone_pair(copy_from, copy_to)
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/RTG_DeepMotion_Para_Manequim", only_if_is_dirty=False)

inputs = unreal.IKRetargetBatchOperationInputs()
inputs.set_editor_property("assets_to_retarget", [unreal.AssetRegistryHelpers.create_asset_data(unreal.load_asset(p)) for p in ANIMS])
inputs.set_editor_property("source_mesh", SRC_MESH)
inputs.set_editor_property("target_mesh", QUINN)
inputs.set_editor_property("ik_retarget_asset", rtg)
inputs.set_editor_property("search", "A_DM_")
inputs.set_editor_property("replace", "A_Ninja_DM_")
inputs.set_editor_property("target_path", OUT)
inputs.set_editor_property("include_referenced_assets", False)
inputs.set_editor_property("overwrite_existing_files", True)
for a in unreal.IKRetargetBatchOperation.run_batch_retarget(inputs):
    seq = unreal.load_asset(str(a.package_name))
    seq.set_editor_property("enable_root_motion", True)
    unreal.EditorAssetLibrary.save_asset(str(a.package_name), only_if_is_dirty=False)
    print("convertida:", a.package_name, "%.2fs %d quadros" % (seq.get_play_length(), seq.get_editor_property("number_of_sampled_frames")))
