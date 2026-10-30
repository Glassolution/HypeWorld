import unreal
# M_CreatorHair não compilava (cabelo cinza = material padrão da Unreal): o importar_cosmeticos_indra ligava
# textura -> Desaturation e Add -> Clamp pelo pino "Input", que não existe nesses nós (o pino deles não tem nome).
# Liga pelo pino sem nome. Pode rodar de novo.
M=unreal.MaterialEditingLibrary;E=unreal.EditorAssetLibrary
mat=unreal.load_asset('/Game/Ninja/Characters/Customization/Materials/M_CreatorHair')
nodes={}
for n in unreal.ObjectIterator(unreal.MaterialExpression):
 if n.get_outer()==mat:nodes[n.get_class().get_name()]=n
sample=nodes['MaterialExpressionTextureSampleParameter2D'];gray=nodes['MaterialExpressionDesaturation']
add=nodes['MaterialExpressionAdd'];clamp=nodes['MaterialExpressionClamp']
assert M.connect_material_expressions(sample,'RGB',gray,''),'textura -> Desaturation'
assert M.connect_material_expressions(add,'',clamp,''),'Add -> Clamp'
M.recompile_material(mat);E.save_loaded_asset(mat,False)
unreal.log('CABELO_MATERIAL %s'%[[i.get_name() if i else None for i in M.get_inputs_for_material_expression(mat,n)] for n in (gray,clamp)])
# O reimport (reimportar_cabelos) deixou um 2º espaço de material vazio nos SK_Hair_*, e é ele que a malha usa:
# vazio = material padrão cinza. Todos os espaços recebem o MI_<id> da pasta.
for d in sorted(E.list_assets('/Game/Ninja/Characters/Customization',recursive=False,include_folder=True)):
 id=d.rstrip('/').split('/')[-1]
 if not id.startswith('Hair_'):continue
 mesh=unreal.load_asset('%s/SK_%s'%(d.rstrip('/'),id));mi=unreal.load_asset('%s/MI_%s'%(d.rstrip('/'),id))
 if not mesh or not mi:continue
 slots=mesh.get_editor_property('materials')
 if all(s.get_editor_property('material_interface')==mi for s in slots):continue
 for k,s in enumerate(slots):s.set_editor_property('material_interface',mi);slots[k]=s
 mesh.call_method('SetMaterials',(slots,));E.save_loaded_asset(mesh,False)
 unreal.log('CABELO_SLOTS %s %d espacos'%(id,len(slots)))
