import unreal,json
from pathlib import Path
out=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/Visual_Auditoria_2026-10-26')
ml=unreal.MaterialEditingLibrary; d={}
for name in ['MI_Tut_Grama','MI_Tut_GramaBase','MI_Tut_TerrenoBase','MI_Tut_Terreno','MI_PP_Personagem']:
    m=unreal.load_asset('/Game/Maps/Tutorial/Tutorial_Assets/Materiais/'+name)
    e={}
    for method,getter in [('get_scalar_parameter_names','get_material_instance_scalar_parameter_value'),('get_static_switch_parameter_names','get_material_instance_static_switch_parameter_value'),('get_texture_parameter_names','get_material_instance_texture_parameter_value')]:
        try:
            for n in getattr(ml,method)(m):
                v=getattr(ml,getter)(m,n);e[str(n)]=str(v)
                if isinstance(v,unreal.Texture):
                    tx={}
                    for p in ['do_scale_mips_for_alpha_coverage','alpha_coverage_thresholds','mip_gen_settings','lod_bias','max_texture_size']:
                        try:tx[p]=str(v.get_editor_property(p))
                        except:pass
                    d[v.get_path_name()]=tx
        except Exception as err:e[method+'_error']=str(err)
    d[name]=e
(out/'parametros_efetivos.json').write_text(json.dumps(d,indent=2),encoding='utf-8')
