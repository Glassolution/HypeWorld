import unreal, json
from pathlib import Path
out=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/Visual_Auditoria_2026-10-26')
data={}
root='/Game/Maps/Tutorial/Tutorial_Assets/'
for suffix in ['Materiais/MI_Tut_Grama','Materiais/MI_Tut_GramaBase','Materiais/MI_Tut_TerrenoBase','Materiais/M_Tut_Grama_Master','Foliage/LGT_Tut_Grama_1','Foliage/LGT_Tut_Grama_2','Materiais/MPC_Wind']:
    obj=unreal.load_asset(root+suffix); props={}
    for p in ['scalar_parameter_values','vector_parameter_values','static_parameters','parent','shading_model','two_sided','grass_varieties','scalar_parameters','vector_parameters']:
        try: props[p]=str(obj.get_editor_property(p))
        except Exception: pass
    data[suffix]=props
    if 'Grama' in suffix and obj and isinstance(obj,unreal.MaterialInterface):
        try:
            for tex in unreal.MaterialEditingLibrary.get_used_textures(obj):
                t={}
                for p in ['do_scale_mips_for_alpha_coverage','alpha_coverage_thresholds','mip_gen_settings','lod_bias','max_texture_size','compression_settings']:
                    try: t[p]=str(tex.get_editor_property(p))
                    except Exception: pass
                data[tex.get_path_name()]=t
        except Exception as e: props['textures_error']=str(e)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Maps/Tutorial/L_Tutorial')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    cls=a.get_class().get_name()
    if cls in ['PostProcessVolume','DirectionalLight','SkyAtmosphere','ExponentialHeightFog']:
        entry={'class':cls}
        for p in ['settings','light_component','component','unbound','blend_weight']:
            try: entry[p]=str(a.get_editor_property(p))
            except Exception: pass
        for c in a.get_components_by_class(unreal.ActorComponent):
            cp={}
            for p in ['light_source_angle','intensity','light_color','aerial_perspective_view_distance_scale','fog_density','fog_height_falloff','start_distance','fog_inscattering_color']:
                try: cp[p]=str(c.get_editor_property(p))
                except Exception: pass
            if cp: entry[c.get_name()]=cp
        data[a.get_actor_label()]=entry
(out/'assets_atuais.json').write_text(json.dumps(data,indent=2,ensure_ascii=False),encoding='utf-8')
print('AUDITORIA_CONCLUIDA')
