import unreal,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Integrado')
AL=unreal.EditorAssetLibrary;ML=unreal.MaterialEditingLibrary
try:
    names=AL.list_assets('/Game/HypeWorld/VisualLab/Integrado',recursive=True)
    results=[]
    for path in names:
        m=AL.load_asset(path)
        if not isinstance(m,unreal.Material):continue
        expr=list(unreal.ObjectIterator(unreal.MaterialExpressionCustom))
        for c in expr:
            if c.get_outer()!=m:continue
            inputs=ML.get_inputs_for_material_expression(m,c)
            for i,src in enumerate(inputs):
                if not isinstance(src,unreal.MaterialExpressionPreSkinnedPosition):continue
                v=ML.create_material_expression(m,unreal.MaterialExpressionVertexInterpolator)
                assert ML.connect_material_expressions(src,'',v,'VS')
                inp=c.get_editor_property('inputs')[i].get_editor_property('input_name')
                assert ML.connect_material_expressions(v,'',c,str(inp))
        ML.recompile_material(m);AL.save_loaded_asset(m);results.append(path)
    (OUT/'materiais_reparados.json').write_text(json.dumps(results,indent=2))
except Exception:(OUT/'reparar_ERRO.txt').write_text(traceback.format_exc())

