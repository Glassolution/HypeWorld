"""Cria a "tradução" de esqueleto X Bot (Mixamo) -> manequim da Unreal e converte as animações.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/retarget_mixamo.py
"""
import unreal

FOLDER = "/Game/Ninja/Characters/Retarget"
OUT = "/Game/Ninja/Characters/Animations/Mixamo"
SRC_ANIMS = "/Game/Ninja/Characters/Mixamo/Anims"
XBOT = unreal.load_asset("/Game/Ninja/Characters/Mixamo/SK_XBot")
QUINN = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
tools = unreal.AssetToolsHelpers.get_asset_tools()
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET


def get_or_create(name, cls, factory):
    existing = unreal.load_asset(f"{FOLDER}/{name}")   # load antes: logo após abrir o editor o registro ainda não achou os arquivos
    return existing if existing else tools.create_asset(name, FOLDER, cls, factory)


def make_rig(name, mesh):
    rig = get_or_create(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    c.apply_auto_generated_retarget_definition()   # cadeias (braços, pernas, coluna...) detectadas sozinhas
    c.apply_auto_fbik()                            # solver de IK para pés e mãos
    unreal.EditorAssetLibrary.save_asset(rig.get_path_name().split(".")[0], only_if_is_dirty=False)
    print(name, "cadeias:", [str(ch.get_editor_property("chain_name")) for ch in c.get_retarget_chains()])
    return rig


src_rig = make_rig("IK_XBot", XBOT)
tgt_rig = make_rig("IK_Manequim", QUINN)

rtg = get_or_create("RTG_XBot_Para_Manequim", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(S, src_rig)
rc.set_ik_rig(T, tgt_rig)
rc.set_preview_mesh(S, XBOT)
rc.set_preview_mesh(T, QUINN)
rc.remove_all_ops()                           # recria do zero (rodar de novo não pode duplicar as etapas)
rc.add_default_ops()                          # Pelvis Motion, FK Chains, IK Chains, IK Solve, Root Motion
rc.assign_ik_rig_to_all_ops(T, tgt_rig)
rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
rc.auto_align_all_bones(T)                    # põe o manequim na mesma pose (T) do X Bot antes de copiar
# Dedos: o Mixamo tem 3 ossos por dedo e o manequim 4 (com metacarpo); copiar deixa os dedos tortos.
# Sem mapear, os dedos do manequim ficam na pose natural dele.
for side in ("Left", "Right"):
    for finger in ("Thumb", "Index", "Middle", "Ring", "Pinky"):
        rc.set_source_chain("None", side + finger)
print("dedos mapeados:", [str(rc.get_source_chain(s + f)) for s in ("Left", "Right") for f in ("Thumb", "Index")])
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/RTG_XBot_Para_Manequim", only_if_is_dirty=False)
print("ops:", [str(rc.get_op_name(i)) for i in range(rc.get_num_retarget_ops())])

# Root Motion: só o deslocamento no chão vai para o root (altura fica no quadril); senão o boneco afunda com o root travado
root_op = rc.get_op_controller(rc.get_index_of_op_by_name("Root Motion"))
settings = root_op.get_settings()
txt = settings.export_text().replace("RootMotionSource=CopyFromSourceRoot", "RootMotionSource=GenerateFromTargetPelvis").replace("RootHeightSource=CopyHeightFromSource", "RootHeightSource=SnapToGround")
settings.import_text(txt)
root_op.set_settings(settings)
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/RTG_XBot_Para_Manequim", only_if_is_dirty=False)
print("root motion:", root_op.get_settings().export_text().split(",bRotateWithPelvis")[0].split("RootMotionSource=")[1], "|", root_op.get_settings().export_text().split("RootHeightSource=")[1].split(",")[0])

# Ossos auxiliares de IK do manequim (ik_foot_*, ik_hand_*) acompanham os pés e mãos de verdade.
# Sem isso eles ficam parados no centro e o Foot IK do ABP puxa os dois pés para baixo do boneco.
pin_index = rc.add_retarget_op("/Script/IKRig.IKRetargetPinBoneOp")
pin = rc.get_op_controller(pin_index)
for copy_from, copy_to in [("root", "ik_foot_root"), ("foot_l", "ik_foot_l"), ("foot_r", "ik_foot_r"),
                           ("root", "ik_hand_root"), ("hand_r", "ik_hand_gun"), ("hand_l", "ik_hand_l"), ("hand_r", "ik_hand_r")]:
    pin.set_bone_pair(copy_from, copy_to)
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/RTG_XBot_Para_Manequim", only_if_is_dirty=False)
print("pin bones:", pin.get_all_bone_pairs())

# Converte todas as animações
ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = [a for a in ar.get_assets_by_path(SRC_ANIMS, recursive=True) if str(a.asset_class_path.asset_name) == "AnimSequence"]
inputs = unreal.IKRetargetBatchOperationInputs()
inputs.set_editor_property("assets_to_retarget", assets)
inputs.set_editor_property("source_mesh", XBOT)
inputs.set_editor_property("target_mesh", QUINN)
inputs.set_editor_property("ik_retarget_asset", rtg)
inputs.set_editor_property("search", "A_MX_")
inputs.set_editor_property("replace", "A_Ninja_MX_")
inputs.set_editor_property("target_path", OUT)
inputs.set_editor_property("include_referenced_assets", False)
inputs.set_editor_property("overwrite_existing_files", True)
result = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
print("convertidas:", len(result))
for a in result:
    seq = unreal.load_asset(str(a.package_name))
    seq.set_editor_property("enable_root_motion", True)   # o deslocamento no root é extraído e o ABP o ignora: boneco no lugar
    unreal.EditorAssetLibrary.save_asset(str(a.package_name), only_if_is_dirty=False)

# Viradas: aqui o giro vai para o root (bRotateWithPelvis) e o jogo descarta o root; assim o corpo não gira
# duas vezes (o personagem já gira pelo código) e a animação mostra só o passo de virar.
TURNS = ["A_MX_Avulsa_Right_Turn", "A_MX_Avulsa_Walking_Turn_180"]
settings = root_op.get_settings()
settings.import_text(settings.export_text().replace("bRotateWithPelvis=False", "bRotateWithPelvis=True"))
root_op.set_settings(settings)
turn_assets = [a for a in assets if str(a.asset_name) in TURNS]
inputs.set_editor_property("assets_to_retarget", turn_assets)
result = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
for a in result:
    seq = unreal.load_asset(str(a.package_name))
    seq.set_editor_property("enable_root_motion", True)
    unreal.EditorAssetLibrary.save_asset(str(a.package_name), only_if_is_dirty=False)
print("viradas com giro no root:", [str(a.asset_name) for a in result])
settings.import_text(settings.export_text().replace("bRotateWithPelvis=True", "bRotateWithPelvis=False"))
root_op.set_settings(settings)
unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/RTG_XBot_Para_Manequim", only_if_is_dirty=False)
