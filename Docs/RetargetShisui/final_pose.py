exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
c=unreal.IKRetargeterController.get_controller(unreal.load_asset(RPATH));rig=unreal.IKRigController.get_controller(c.get_ik_rig(T))
previous=json.loads((OUT/'head_fix.json').read_text())
delta=previous['mannequin']['spine_deg']-previous['shisui']['spine_deg']
def inv(q):return unreal.Quat(-q.x,-q.y,-q.z,q.w)
parent_pose=unreal.Quat(0,0,0,1);parent_ref=parent_pose
for name in ['root','pelvis','spine_01']:
    ref=rig.get_ref_pose_transform_of_bone(name).rotation;local=inv(parent_ref)*ref
    offset=c.get_rotation_offset_for_retarget_pose_bone(name,T);global_pose=parent_pose*local*offset
    if name=='spine_01':
        a=math.radians(delta)/2.;world_delta=unreal.Quat(math.sin(a),0,0,math.cos(a))
        new_offset=inv(local)*inv(parent_pose)*world_delta*global_pose
        c.set_rotation_offset_for_retarget_pose_bone(name,new_offset,T)
    parent_ref=ref;parent_pose=global_pose
exec(open(str(OUT/'align_head.py'),encoding='utf-8').read())
c.set_retarget_op_enabled(2,True);c.set_retarget_op_enabled(3,True)
assert unreal.EditorAssetLibrary.save_loaded_asset(unreal.load_asset(RPATH))
print('SPINE_NEUTRAL_CORRECTION_DEG',delta,'FOOT_IK_ON_HANDS_IK_OFF')
