# Compara dois FBX de traje (vértices, faces, deslocamento máximo). Uso: blender -b --factory-startup --python Tools/customization/comparar_fbx_traje.py -- <a.fbx> <b.fbx>
import bpy,sys
import numpy as np
a_,b_=sys.argv[sys.argv.index('--')+1:]
def carrega(p):
    bpy.ops.wm.read_factory_settings(use_empty=True); bpy.ops.import_scene.fbx(filepath=p)
    o=next(x for x in bpy.context.scene.objects if x.type=='MESH')
    return np.array([o.matrix_world@v.co for v in o.data.vertices]),len(o.data.polygons)
a,fa=carrega(a_); b,fb=carrega(b_)
print('RES',a_.split('/')[-1],'v',len(a),len(b),'f',fa,fb,'desloc max mm',round(float(np.linalg.norm(a-b,axis=1).max()*1000),3) if len(a)==len(b) else 'n/a')
