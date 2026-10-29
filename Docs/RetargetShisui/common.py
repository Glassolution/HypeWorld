import unreal,math,json,hashlib,shutil
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui')
OUT.mkdir(exist_ok=True)
ROOT=OUT.parents[1]
RPATH='/Game/HypeWorld/Personagens/Shisui/RTG_Manequim_Para_Shisui'
TPATH='/Game/HypeWorld/Personagens/Shisui/SK_Shisui'
SPATH='/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple'
IDLE='/Game/ANAP/Animations/Base/Base_Anim_Armature_CustomMan_CharaSelect_Idle'
AP=unreal.AnimPoseExtensions;WORLD=unreal.AnimPoseSpaces.WORLD;LOCAL=unreal.AnimPoseSpaces.LOCAL
T=unreal.RetargetSourceOrTarget.TARGET;SRC=unreal.RetargetSourceOrTarget.SOURCE
def vec(v):return [v.x,v.y,v.z]
def pitch(v):return math.degrees(math.atan2(-v.y,v.z))
def metrics(component):
    def tr(n):return component.get_socket_transform(n,unreal.RelativeTransformSpace.RTS_COMPONENT)
    def pos(n):return tr(n).translation
    # Both meshes use the same skeletal landmark, so the wrist-to-middle_03 measure is directly comparable.
    controller=unreal.IKRetargeterController.get_controller(unreal.load_asset(RPATH))
    side=T if component.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Shisui' else SRC
    rig=unreal.IKRigController.get_controller(controller.get_ik_rig(side))
    reference_head=rig.get_ref_pose_transform_of_bone('head').rotation
    head_delta=tr('head').rotation*unreal.Quat(-reference_head.x,-reference_head.y,-reference_head.z,reference_head.w)
    head_up=head_delta.rotate_vector(unreal.Vector(0,0,1))
    data={'pelvis_cm':pos('pelvis').z,'feet_width_cm':abs(pos('foot_l').x-pos('foot_r').x),'spine_deg':pitch(pos('neck_01')-pos('spine_01')),'head_axis':vec(head_up),'head_rotation':str(tr('head').rotation.rotator()),'head_neck_deg':pitch(pos('head')-pos('neck_01')),'hand_cm':(pos('middle_03_l')-pos('hand_l')).length(),'scale':vec(component.get_editor_property('relative_scale3d')),'bones':{n:vec(pos(n)) for n in ['root','pelvis','spine_01','spine_05','neck_01','neck_02','head','foot_l','foot_r','ball_l','ball_r','hand_l','middle_01_l','middle_02_l','middle_03_l']},'hand_scale':vec(tr('hand_l').scale3d)}
    data['head_deg']=pitch(head_up)
    data['reference_head_quat']=[reference_head.x,reference_head.y,reference_head.z,reference_head.w]
    q=tr('head').rotation
    data['head_quat']=[q.x,q.y,q.z,q.w]
    data['mesh_reference_pelvis_cm']=rig.get_ref_pose_transform_of_bone('pelvis').translation.z
    def knee(side):
        a=pos('thigh_'+side)-pos('calf_'+side);b=pos('foot_'+side)-pos('calf_'+side)
        dot=(a.x*b.x+a.y*b.y+a.z*b.z)/(a.length()*b.length())
        return 180-math.degrees(math.acos(max(-1.,min(1.,dot))))
    data['knees_deg']=[knee('l'),knee('r')]
    data['feet_z_cm']=[pos('foot_l').z,pos('foot_r').z]
    return data
