exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
c=unreal.IKRetargeterController.get_controller(unreal.load_asset(RPATH));rig=unreal.IKRigController.get_controller(c.get_ik_rig(T))
def inv(q):return unreal.Quat(-q.x,-q.y,-q.z,q.w)
chain=['root','pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','neck_01','neck_02','head']
global_pose={};parent=None
for bone in chain:
    ref=rig.get_ref_pose_transform_of_bone(bone).rotation
    ref_parent=rig.get_ref_pose_transform_of_bone(parent).rotation if parent else unreal.Quat(0,0,0,1)
    local=inv(ref_parent)*ref
    parent_pose=global_pose[parent] if parent else unreal.Quat(0,0,0,1)
    offset=c.get_rotation_offset_for_retarget_pose_bone(bone,T)
    if bone=='head':
        # Align the physical skull's neutral direction, accounting for the different joint coordinate axes.
        offset=inv(local)*inv(parent_pose)*ref
        c.set_rotation_offset_for_retarget_pose_bone(bone,offset,T)
    global_pose[bone]=parent_pose*local*offset
    parent=bone
print('HEAD_OFFSET',c.get_rotation_offset_for_retarget_pose_bone('head',T))
unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset(RPATH))
