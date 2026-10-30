import unreal
# Couro cabeludo: importa a máscara (gerar_mascara_couro, no Blender) e acrescenta ao M_Character_Master
# "Scalp Color" e "Scalp Strength" (0 = desligado). Escurece o topo da cabeça com a cor do cabelo, para não
# aparecer pele clara entre as mechas. O HypeAppearanceComponent liga quando há cabelo. Pode rodar de novo.
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
M=unreal.MaterialEditingLibrary;E=unreal.EditorAssetLibrary;A=unreal.AssetToolsHelpers.get_asset_tools()
FOLDER='/Game/HypeWorld/Characters/Base'
task=unreal.AssetImportTask();task.filename=str(ROOT/'SourceAssets/Customization/Masks/T_Base_Body_ScalpMask.png');task.destination_path=FOLDER+'/Textures'
task.destination_name='T_Base_Body_ScalpMask';task.automated=True;task.replace_existing=True;task.save=True;A.import_asset_tasks([task])
tex=unreal.load_asset(FOLDER+'/Textures/T_Base_Body_ScalpMask')
tex.set_editor_property('srgb',False);tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_GRAYSCALE);E.save_loaded_asset(tex,False)
mat=unreal.load_asset(FOLDER+'/M_Character_Master')
# .rgb: o parâmetro de cor chega como float4; misturar float3 com float4 não compila no Metal
# Strength 0,85 (padrão do HypeAppearanceComponent) já cobre tudo: a pele que sobrava (15–30%) virava uma touca marrom no topo.
# couro = 40% da cor: o M_CreatorHair pinta as mechas com 25–100% da cor (textura), então o couro some entre elas
CODE='float m = saturate(Mask.r * Strength * 1.25); return lerp(Base.rgb, Hair.rgb * 0.4, m);'
if 'Scalp Strength' in [str(n) for n in M.get_scalar_parameter_names(mat)]:
 # já existe: só garante a textura
 for n in unreal.ObjectIterator(unreal.MaterialExpressionTextureSampleParameter2D):
  if n.get_outer()==mat and str(n.get_editor_property("parameter_name"))=="ScalpMask":n.set_editor_property("texture",tex);n.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)  # máscara sem sRGB
 for n in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
  if n.get_outer()==mat and str(n.get_editor_property('description'))=='Scalp':n.set_editor_property('code',CODE)
else:
 base=M.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
 pos=(base.get_editor_property('material_expression_editor_x'),base.get_editor_property('material_expression_editor_y'))
 def node(cls,dx,dy,**props):
  n=M.create_material_expression(mat,cls,pos[0]+dx,pos[1]+dy)
  for k,v in props.items():n.set_editor_property(k,v)
  return n
 sample=node(unreal.MaterialExpressionTextureSampleParameter2D,0,250,parameter_name='ScalpMask',texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
 color=node(unreal.MaterialExpressionVectorParameter,0,450,parameter_name='Scalp Color',default_value=unreal.LinearColor(.025,.02,.02,1))
 amount=node(unreal.MaterialExpressionScalarParameter,0,550,parameter_name='Scalp Strength',default_value=0.0)
 mix=node(unreal.MaterialExpressionCustom,300,0,description='Scalp',output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
  code=CODE)
 inputs=[]
 for name in ('Base','Mask','Hair','Strength'):
  i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
 mix.set_editor_property('inputs',inputs)
 M.connect_material_expressions(base,'',mix,'Base');M.connect_material_expressions(sample,'R',mix,'Mask')
 M.connect_material_expressions(color,'',mix,'Hair');M.connect_material_expressions(amount,'',mix,'Strength')
 M.connect_material_property(mix,'',unreal.MaterialProperty.MP_BASE_COLOR)
M.recompile_material(mat);E.save_loaded_asset(mat,False)
unreal.log('COURO_CABELUDO ok params=%s'%[str(n) for n in M.get_scalar_parameter_names(mat) if 'Scalp' in str(n)])
