from pathlib import Path
source=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/AA/coletar_aa.py').read_text().split('CASES=')[0]
exec(source.replace("Alto100/AA')","Alto100/GIAO')"))
pose(0)
for s in ['sg.AntiAliasingQuality 1','r.AntiAliasingMethod 4','r.TSR.ShadingRejection.Flickering 0','r.TSR.History.ScreenPercentage 100','r.Shadow.CSM.MaxCascades 3','r.Shadow.MaxCSMResolution 1536','r.Shadow.RadiusThreshold 0.06']:
    cmd(s)
LIGHT=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.DirectionalLight)[0].get_component_by_class(unreal.DirectionalLightComponent)
LIGHT.set_dynamic_shadow_distance_movable_light(5000.)
previous=json.loads((OUT.parent/'Sombras/csm3_1536_config.json').read_text())
disabled=[]
for path in previous['disabled']:
    c=unreal.find_object(None,path)
    if c:c.set_cast_shadow(False);disabled.append(path)
assert len(disabled)==len(previous['disabled']), (len(disabled),len(previous['disabled']))
PP=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.PostProcessVolume) if 'MI_PP_Personagem' in str(a.get_editor_property('settings').get_editor_property('weighted_blendables')))
settings=PP.get_editor_property('settings')
info={'base':state(),'camera':str(CAM.get_world_location()),'rotation':str(CAM.get_world_rotation()),'disabled':disabled,'postprocess':{},'skylights':[]}
for field in ['dynamic_global_illumination_method','override_dynamic_global_illumination_method','reflection_method','override_reflection_method','ambient_occlusion_intensity']:
    info['postprocess'][field]=str(settings.get_editor_property(field))
for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.SkyLight):
    c=a.get_component_by_class(unreal.SkyLightComponent)
    info['skylights'].append({k:str(c.get_editor_property(k)) for k in ['mobility','intensity','cast_shadows','occlusion_max_distance','contrast','occlusion_tint']})
(OUT/'preparacao.json').write_text(json.dumps(info,indent=2))
print(json.dumps(info,indent=2))
