import unreal,json,traceback,hashlib
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Integrado')
D='/Game/HypeWorld/VisualLab/Integrado'
AL=unreal.EditorAssetLibrary;ML=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools()
made={};report={'bush_materials':{},'trees':{}}
def node(m,cls):return ML.create_material_expression(m,cls)
def custom_input(name):
    i=unreal.CustomInput();i.set_editor_property('input_name',name);return i
def wire(a,out,b,inp):
    assert ML.connect_material_expressions(a,out,b,inp),(str(a),out,str(b),inp)
def gradient(m):
    assert not m.get_editor_property('use_material_attributes')
    src=ML.get_material_property_input_node(m,unreal.MaterialProperty.MP_BASE_COLOR)
    pin=ML.get_material_property_input_node_output_name(m,unreal.MaterialProperty.MP_BASE_COLOR)
    local=node(m,unreal.MaterialExpressionPreSkinnedPosition)
    bounds=node(m,unreal.MaterialExpressionObjectLocalBounds)
    code=node(m,unreal.MaterialExpressionCustom)
    code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    code.set_editor_property('inputs',[custom_input(n) for n in ['Local','BMin','BMax']])
    code.set_editor_property('code','float3 p=saturate((Local-BMin)/max(BMax-BMin,1.0)); float h=smoothstep(0.05,0.9,p.z); float core=1-saturate(length((p.xy-0.5)*2)); return (0.62+0.38*h)*(1-0.22*core*(1-0.65*p.z));')
    wire(local,'',code,'Local');wire(bounds,'Min',code,'BMin');wire(bounds,'Max',code,'BMax')
    mult=node(m,unreal.MaterialExpressionMultiply);wire(src,pin,mult,'A');wire(code,'',mult,'B')
    assert ML.connect_material_property(mult,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ML.recompile_material(m)
def clone_chain(src):
    path=src.get_path_name()
    if path in made:return made[path]
    dst=D+'/Arbustos/'+src.get_name()+'_'+hashlib.sha1(path.encode()).hexdigest()[:6]
    if AL.does_asset_exist(dst):
        made[path]=AL.load_asset(dst);return made[path]
    obj=AL.duplicate_asset(path.split('.')[0],dst);assert obj
    made[path]=obj
    if isinstance(obj,unreal.Material):gradient(obj)
    else:ML.set_material_instance_parent(obj,clone_chain(src.get_editor_property('parent')));ML.update_material_instance(obj)
    assert AL.save_loaded_asset(obj)
    return obj
try:
    rows=json.loads((OUT.parent/'GIAO/vegetacao.json').read_text())
    for row in rows:
        if 'bush' not in row['mesh'].lower():continue
        for m in row['materials']:
            if 'MASKED' in m['blend'] and 'flores' not in m['path'].lower():
                copied=clone_chain(AL.load_asset(m['path']))
                report['bush_materials'][m['path']]=copied.get_path_name()
    (OUT/'assets_criados.json').write_text(json.dumps(report,indent=2))
    # New opaque canopy material, with soft large-scale pigment variation.
    mp=D+'/Copas/M_Copa_Opaca'
    canopy=AL.load_asset(mp) if AL.does_asset_exist(mp) else AT.create_asset('M_Copa_Opaca',D+'/Copas',unreal.Material,unreal.MaterialFactoryNew())
    if not ML.get_material_property_input_node(canopy,unreal.MaterialProperty.MP_BASE_COLOR):
        local=node(canopy,unreal.MaterialExpressionPreSkinnedPosition)
        code=node(canopy,unreal.MaterialExpressionCustom)
        code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        code.set_editor_property('inputs',[custom_input('P')])
        code.set_editor_property('code','float h=saturate((P.z-380)/500); float n=sin(P.x*0.045+sin(P.y*0.027))*sin(P.z*0.041+P.y*0.034); return lerp(float3(0.055,0.16,0.025),float3(0.22,0.39,0.075),smoothstep(0,1,h))*(0.94+0.06*n);')
        wire(local,'',code,'P');ML.connect_material_property(code,'',unreal.MaterialProperty.MP_BASE_COLOR)
        for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,1),(unreal.MaterialProperty.MP_SPECULAR,0)]:
            c=node(canopy,unreal.MaterialExpressionConstant);c.set_editor_property('r',value);ML.connect_material_property(c,'',prop)
        ML.recompile_material(canopy);AL.save_loaded_asset(canopy)
    for name in ['CommonTree_3']:
        src=AL.load_asset('/Game/HypeWorld/Environment/MegaKit/Meshes/'+name)
        dst=D+'/Copas/SM_'+name+'_Opaca'
        mesh=AL.load_asset(dst) if AL.does_asset_exist(dst) else AL.duplicate_asset(src.get_path_name().split('.')[0],dst)
        dyn=unreal.DynamicMesh()
        result=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(src,dyn,unreal.GeometryScriptCopyMeshFromAssetOptions(),unreal.GeometryScriptMeshReadLOD())
        removed=unreal.GeometryScript_Materials.delete_triangles_by_material_id(dyn,1)
        opts=unreal.GeometryScriptPrimitiveOptions(material_id=1)
        # Interlocking, rounded crown lobes; preserve the original trunk geometry.
        lobes=[(0,0,735,170,180,170),(-105,-45,675,115,125,125),(105,25,700,115,125,135),(-45,120,730,120,110,140),(65,-120,740,120,110,135),(-45,-30,840,120,125,100),(60,70,810,120,125,110)]
        for x,y,z,sx,sy,sz in lobes:
            tr=unreal.Transform(location=unreal.Vector(x,y,z),scale=unreal.Vector(sx,sy,sz))
            unreal.GeometryScript_Primitives.append_sphere_lat_long(dyn,opts,tr,1.,16,24)
        opt=unreal.GeometryScriptCopyMeshToAssetOptions()
        opt.set_editor_property('replace_materials',True)
        opt.set_editor_property('new_materials',[src.get_material(0),canopy])
        unreal.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(dyn,mesh,opt,unreal.GeometryScriptMeshWriteLOD())
        AL.save_loaded_asset(mesh)
        report['trees'][src.get_path_name()]=mesh.get_path_name()
        report['removed_leaf_triangles']=str(removed)
    (OUT/'assets_criados.json').write_text(json.dumps(report,indent=2))
except Exception:(OUT/'assets_ERRO.txt').write_text(traceback.format_exc())

