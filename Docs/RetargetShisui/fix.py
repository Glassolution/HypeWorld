exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
r=unreal.load_asset(RPATH);c=unreal.IKRetargeterController.get_controller(r)
source_rig=unreal.IKRigController.get_controller(c.get_ik_rig(SRC));target_rig=unreal.IKRigController.get_controller(c.get_ik_rig(T))
ratio=target_rig.get_ref_pose_transform_of_bone('pelvis').translation.z/source_rig.get_ref_pose_transform_of_bone('pelvis').translation.z
pelvis=c.get_op_controller(0).get_settings()
# Pelvis Motion already multiplies by target/source mesh height. 1 preserves this ratio; another ratio here would double-scale it.
for key,value in [('scale_vertical',1.),('scale_horizontal',1.),('blend_to_source_translation',0.),('blend_to_absolute_offset',unreal.Vector()),('translation_offset_global',unreal.Vector()),('translation_offset_local',unreal.Vector())]:pelvis.set_editor_property(key,value)
c.get_op_controller(0).set_settings(pelvis)
c.reset_retarget_pose(c.get_current_retarget_pose_name(T),[],T)
c.set_root_offset_in_retarget_pose(unreal.Vector(),T)
c.auto_align_all_bones(T,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
fk=c.get_op_controller(1).get_settings();chains=list(fk.chains_to_retarget)
for chain in chains:
    chain.set_editor_property('translation_mode',unreal.FKChainTranslationMode.NONE)
    chain.set_editor_property('rotation_mode',unreal.FKChainRotationMode.ONE_TO_ONE)
fk.set_editor_property('chains_to_retarget',chains);c.get_op_controller(1).set_settings(fk)
for index,field in [(2,'chains_to_retarget'),(3,'chains')]:
    settings=c.get_op_controller(index).get_settings()
    settings.set_editor_property(field,[x for x in settings.get_editor_property(field) if str(x.target_chain_name) in ['LeftLeg','RightLeg']]);c.get_op_controller(index).set_settings(settings)
solvers=[]
for i in range(target_rig.get_num_solvers()):
    end=str(target_rig.get_solver_controller(i).get_solver_settings().end_bone);enabled='foot' in end
    target_rig.set_solver_enabled(i,enabled);solvers.append({'index':i,'end':end,'enabled':enabled})
# First measure FK alone, then re-enable feet only if their output remains correct.
c.set_retarget_op_enabled(2,False);c.set_retarget_op_enabled(3,False)
assert unreal.EditorAssetLibrary.save_loaded_asset(c.get_ik_rig(T))
assert unreal.EditorAssetLibrary.save_loaded_asset(r)
(OUT/'fix_config.json').write_text(json.dumps({'effective_pelvis_scale':ratio,'pelvis_scale_control':1.,'translation_chains':'Pelvis only; all FK chains None','auto_align':'CHAIN_TO_CHAIN','solvers':solvers,'foot_ik':'temporarily disabled for measurement','hands_ik':False},indent=2))
print('FIXED_CONFIG',ratio,solvers)
