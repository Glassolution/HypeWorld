from pathlib import Path
source=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/AA/coletar_aa.py').read_text().split('CASES=')[0]
exec(source.replace("Alto100/AA')","Alto100/AA1b')"))
NAMES.extend(['r.TemporalAACurrentFrameWeight','r.TemporalAA.Quality'])
cmd('r.AntiAliasingMethod 2')
inventory=[]
for c in P.get_components_by_class(unreal.ActorComponent):
    row={'name':c.get_name(),'class':c.get_class().get_name(),'path':c.get_path_name()}
    if isinstance(c,unreal.MeshComponent):
        row['materials']=[str(m.get_path_name()) if m else None for m in c.get_materials()]
        for field in ['skeletal_mesh_asset','static_mesh','visible','hidden_in_game']:
            try:row[field]=str(c.get_editor_property(field))
            except Exception:pass
    inventory.append(row)
(OUT/'personagem_componentes.json').write_text(json.dumps(inventory,indent=2))
PP=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.PostProcessVolume) if 'MI_PP_Personagem' in str(a.get_editor_property('settings').get_editor_property('weighted_blendables')))
bl=PP.get_editor_property('settings').get_editor_property('weighted_blendables').get_editor_property('array')
TOON=next(b.get_editor_property('object') for b in bl if 'MI_PP_Personagem' in b.get_editor_property('object').get_path_name())
print('WEIGHT',unreal.SystemLibrary.get_console_variable_string_value('r.TemporalAACurrentFrameWeight'))
print(json.dumps(inventory,indent=2))
print(unreal.SkinnedMeshComponent.show_material_section.__doc__)
