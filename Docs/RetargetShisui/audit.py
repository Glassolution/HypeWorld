exec(open(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/RetargetShisui/common.py',encoding='utf-8').read())
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
c=unreal.IKRetargeterController.get_controller(unreal.load_asset(RPATH))
paths=[RPATH,c.get_ik_rig(T).get_path_name().split('.')[0]]
for path in paths:
    rel='Content/'+path.removeprefix('/Game/')+'.uasset';dest=OUT/'BackupAntes'/rel
    if not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dest)
protected=[]
for folder in ['Content/ANAP','Content/Ninja/Characters/Animations','Content/HypeWorld/Animacoes','Content/HypeWorld/Jutsus','Content/Maps','Source','Config']:
    for f in (ROOT/folder).rglob('*'):
        if f.is_file():protected.append({'path':str(f.relative_to(ROOT)),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
(OUT/'protected.json').write_text(json.dumps(protected,indent=2))
report={'ops':[{'index':i,'name':str(c.get_op_name(i)),'enabled':c.get_retarget_op_enabled(i),'settings':str(c.get_op_controller(i).get_settings())} for i in range(c.get_num_retarget_ops())],'refs':{},'fk':[str(s) for s in c.get_op_controller(1).get_settings().chains_to_retarget],'ik':[str(s) for s in c.get_op_controller(2).get_settings().chains_to_retarget]}
for label,path in [('mannequin',SPATH),('shisui',TPATH)]:
    mesh=unreal.load_asset(path);rp=AP.get_reference_pose(mesh.get_editor_property('skeleton'));names=[str(n) for n in AP.get_bone_names(rp)]
    report['refs'][label]={'bones':{n:{'p':vec(AP.get_bone_pose(rp,n,WORLD).translation),'local_rotation':str(AP.get_bone_pose(rp,n,LOCAL).rotation),'scale':vec(AP.get_bone_pose(rp,n,LOCAL).scale3d)} for n in names},'import':list(mesh.get_editor_property('asset_import_data').extract_filenames())}
report['pelvis_ratio']=report['refs']['shisui']['bones']['pelvis']['p'][2]/report['refs']['mannequin']['bones']['pelvis']['p'][2]
report['pose_offsets']={n:str(c.get_rotation_offset_for_retarget_pose_bone(n,T)) for n in report['refs']['shisui']['bones']}
report['root_offset']=vec(c.get_root_offset_in_retarget_pose(T))
(OUT/'audit.json').write_text(json.dumps(report,indent=2))
print('PELVIS_RATIO',report['pelvis_ratio'],'PROTECTED',len(protected))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Engine/Maps/Entry')
e=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
floor=e.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,50000));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(200,200,1))
for yaw,intensity in [(-45,6),(135,5),(45,4)]:
    light=e.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,50500),unreal.Rotator(pitch=-40,yaw=yaw));light.light_component.set_editor_property('intensity',intensity);light.light_component.set_editor_property('cast_shadows',False)
perf=unreal.get_default_object(unreal.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'));perf.set_editor_property('bThrottleCPUWhenNotForeground',False)
