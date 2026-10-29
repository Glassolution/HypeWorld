import unreal,json,re
from pathlib import Path
out=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/Visual_Auditoria_2026-10-26')
def normalize(s): return re.sub(r'0x[0-9A-Fa-f]+','ADDR',str(s))
def inspect(path):
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(path)
    data={}
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        cls=a.get_class().get_name()
        if cls not in ['PostProcessVolume','DirectionalLight','SkyAtmosphere','ExponentialHeightFog','SkyLight']: continue
        e={'class':cls}
        for p in ['settings','unbound','blend_weight']:
            try:e[p]=normalize(a.get_editor_property(p))
            except:pass
        for c in a.get_components_by_class(unreal.ActorComponent):
            cp={}
            for p in ['light_source_angle','intensity','light_color','aerial_perspective_view_distance_scale','fog_density','fog_height_falloff','start_distance','fog_inscattering_color','volumetric_fog','indirect_lighting_intensity','lower_hemisphere_color']:
                try:cp[p]=normalize(c.get_editor_property(p))
                except:pass
            if cp:e[c.get_name()]=cp
        data[a.get_actor_label()]=e
    return data
old=inspect('/Game/HypeWorld/VisualLab/Auditoria/L_Aprovado_50a4980')
now=inspect('/Game/Maps/Tutorial/L_Tutorial')
diff={k:{'aprovado':old.get(k),'atual':now.get(k)} for k in set(old)|set(now) if old.get(k)!=now.get(k)}
textures={}
for p in ['/Game/Maps/Tutorial/Tutorial_Assets/Materiais/MI_Tut_Grama','/Game/Maps/Tutorial/Tutorial_Assets/Materiais/M_Tut_Grama_Master']:
    obj=unreal.load_asset(p)
    try:ts=unreal.MaterialEditingLibrary.get_material_used_textures(obj)
    except Exception as e: textures[p]=str(e);continue
    for t in ts:
        entry={}
        for prop in ['do_scale_mips_for_alpha_coverage','alpha_coverage_thresholds','mip_gen_settings','lod_bias','max_texture_size']:
            try:entry[prop]=str(t.get_editor_property(prop))
            except:pass
        textures[t.get_path_name()]=entry
(out/'comparacao_mapas.json').write_text(json.dumps({'old':old,'now':now,'differences':diff,'textures':textures},indent=2,ensure_ascii=False),encoding='utf-8')
print('COMPARACAO_OK',len(old),len(now),len(diff))
