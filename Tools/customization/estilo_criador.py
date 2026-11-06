import bpy, bmesh, json, sys
from pathlib import Path
from mathutils import Vector
r=Path(sys.argv[-2])
out=Path(sys.argv[-1])
base=r/'SourceAssets/Customization'
mod=base/'IndraModular'
files=[(p,'head' if p.stem=='SK_Indra_Head' else 'hair' if 'Hair' in p.stem else 'outfit') for p in (
 mod/'SK_Indra_Head.fbx',mod/'SK_Hair_Indra.fbx',mod/'SK_Outfit_Indra.fbx',
 *(mod/n/('SK_Outfit_'+n+'.fbx') for n in ('ItachiAnbu','KakashiAnbu','Sasuke')),
)]
files += [(p,'hair') for p in sorted((base/'Fitted').glob('Hair_*/Hair_*.fbx'))]

def ease(x):
    t=max(0.0,min(1.0,x))
    return t*t*(3-2*t)

def head_form(p, role):
    x,y,z=p
    # O anel em 1,50 m permanece fixo. O queixo avança em direção à gola
    # e o crânio ganha largura, profundidade e altura sem mudar a bind pose.
    neck=ease((z-1.50)/.067)
    crown=ease((z-1.57)/.145)
    follow=1 if role=='head' else ease((z-1.43)/.135)
    return Vector((x*(1+.18*neck*follow),
                   y+(.015+y)*.11*neck*follow,
                   z+(.014*crown-.012*neck*(1-crown))*follow))

def outfit_form(p):
    x,y,z=p
    shoulder=ease((z-1.20)/.14)*(1-ease((z-1.44)/.06))
    center=1-ease((abs(x)-.23)/.17)
    chest=ease((z-1.00)/.22)*(1-ease((z-1.42)/.08))
    return Vector((x*(1-.12*shoulder*center),
                   y*(1-.15*chest*center),
                   z-.020*shoulder*ease((abs(x)-.07)/.16)*center))

def clean_anbu(mesh, world):
    bm=bmesh.new()
    bm.from_mesh(mesh.data)
    deform=bm.verts.layers.deform.active
    pelvis=next(g.index for g in mesh.vertex_groups if g.name.startswith('9ind00t0-pelvis_'))
    remaining=set(bm.verts)
    remove=[]
    while remaining:
        seed=remaining.pop()
        island=[seed]
        for v in island:
            for edge in v.link_edges:
                n=edge.other_vert(v)
                if n in remaining:
                    remaining.remove(n)
                    island.append(n)
        coords=[world@v.co for v in island]
        # Placas soltas das luvas do rip e o cabo que se projeta atrás da cabeça.
        glove=(len(island)<90 and all(abs(p.x)>.49 and .90<p.z<1.075 for p in coords))
        hilt=(all(p.y>.12 for p in coords) and max(p.z for p in coords)>1.60)
        if glove or hilt:
            remove.extend(island)
    bmesh.ops.delete(bm,geom=remove,context='VERTS')
    print('ANBU peças soltas removidas',len(remove))
    for v in bm.verts:
        x,y,z=world@v.co
        influence=(1-ease(abs(x)/.10))*ease((z-.67)/.11)*(1-ease((z-.86)/.06))
        if influence<=0:
            continue
        weights=v[deform]
        for index,value in list(weights.items()):
            weights[index]=value*(1-influence*.90)
        weights[pelvis]=weights.get(pelvis,0)+influence*.90
    # Painel de tecido sob a costura central. Tem peso de pélvis e UV da calça
    # para que a abertura original não mostre o fundo durante a pose.
    uv=bm.loops.layers.uv.active
    fabric_faces=[f for f in bm.faces if .73<(world@f.calc_center_median()).z<.89]
    source=min(fabric_faces,key=lambda f:(world@f.calc_center_median()-Vector((.074,-.11,.837))).length)
    uv_coord=sum((loop[uv].uv for loop in source.loops),Vector((0,0)))/len(source.loops)
    inv=world.inverted()
    patch=[]
    for x,y,z in ((-.053,-.102,.866),(.053,-.102,.866),(.038,-.105,.806),
                  (.018,-.083,.769),(-.018,-.083,.769),(-.038,-.105,.806)):
        v=bm.verts.new(inv@Vector((x,y,z)))
        v[deform][pelvis]=1.0
        patch.append(v)
    face=bm.faces.new(tuple(reversed(patch)))
    face.material_index=source.material_index
    face.smooth=True
    for loop in face.loops:
        loop[uv].uv=uv_coord
    bm.normal_update()
    bm.to_mesh(mesh.data)
    bm.free()

for path,role in files:
    if __import__('os').environ.get('HYPE_CREATOR_QUICK') and path.stem not in ('SK_Outfit_ItachiAnbu','Hair_sasuke_boruto'):
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(path),colors_type='LINEAR')
    mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    w=mesh.matrix_world.copy()
    inv=w.inverted()
    if path.stem=='SK_Outfit_ItachiAnbu':
        clean_anbu(mesh,w)
    for v in mesh.data.vertices:
        p=w@v.co
        q=outfit_form(p) if role=='outfit' else head_form(p,role)
        if path.stem=='Hair_sasuke_boruto':
            front=ease((-p.y-.045)/.054)
            eye_band=ease((abs(p.x)-.012)/.022)*(1-ease((abs(p.x)-.058)/.026))
            tip=1-ease((p.z-1.56)/.11)
            q.z+=.043*front*eye_band*tip
            q.z+=.010*ease((p.z-1.67)/.10)
        v.co=inv@q
    target=out/path.relative_to(r)
    target.parent.mkdir(parents=True,exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    mesh.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=mesh
    bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'ARMATURE','MESH'},
        add_leaf_bones=False,bake_anim=False,use_armature_deform_only=False,apply_unit_scale=True,
        axis_forward='-Z',axis_up='Y',mesh_smooth_type='FACE',colors_type='LINEAR',path_mode='AUTO')
    print('SHAPED',path.stem)
