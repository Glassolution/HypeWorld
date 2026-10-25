"""Blender (em segundo plano): ajusta uma animação de andar do Motifect.
- tronco inclinado para frente (LEAN graus, distribuído na coluna) com a cabeça compensando (olhar para frente)
- balanço lateral do tronco reduzido (SWAY_KEEP = fração que fica)
- pés mais afastados (abre as coxas um pouco e endireita o pé de volta)
Rodar: blender -b --factory-startup --python Tools/blender/ajustar_andar.py -- <entrada.fbx> <saida.fbx>
"""
import bpy
import math
import sys
from mathutils import Matrix, Vector

LEAN = {"Spine1": 3.0, "Spine2": 2.0, "Chest": 2.0}   # graus para frente (total 7)
HEAD_BACK = {"Neck1": -3.0, "Head": -2.0}             # a cabeça volta para olhar à frente
SWAY_KEEP = 0.6                                         # fica 60% do balanço lateral do tronco
WIDEN_CM = 2.5                                          # cada pé vai X cm para fora

src, dst = sys.argv[sys.argv.index("--") + 1:][:2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=src)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
scene = bpy.context.scene
act = arm.animation_data.action
f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
scene.frame_start, scene.frame_end = f0, f1
pb = arm.pose.bones
for b in pb:
    b.rotation_mode = "QUATERNION"

def world(bone_name, head=True):
    b = pb[bone_name]
    return arm.matrix_world @ (b.head if head else b.tail)

# Direção de caminhada (horizontal) e eixo lateral, pelo deslocamento do quadril ao longo do clipe
scene.frame_set(f0); p0 = arm.matrix_world.translation.copy()
scene.frame_set(f1); p1 = arm.matrix_world.translation.copy()
fwd = Vector((p1.x - p0.x, p1.y - p0.y, 0)).normalized()
up = Vector((0, 0, 1))
side = fwd.cross(up).normalized()          # aponta para a direita de quem anda
pitch_axis = up.cross(fwd).normalized()    # girar em volta dele leva o topo para frente
print("frente", tuple(round(v, 3) for v in fwd))

def rotate_world(bone_name, axis, degrees):
    """Gira o osso em volta do próprio início, no espaço do mundo (filhos acompanham)."""
    b = pb[bone_name]
    mw = arm.matrix_world @ b.matrix
    pivot = mw.translation.copy()
    r = Matrix.Rotation(math.radians(degrees), 4, axis)
    new = Matrix.Translation(pivot) @ r @ Matrix.Translation(-pivot) @ mw
    b.matrix = arm.matrix_world.inverted() @ new
    bpy.context.view_layer.update()

# Lado de cada pé (+1 = direita), olhando o primeiro quadro
scene.frame_set(f0)
left_sign = 1.0 if (world("LeftLeg") - world("RightLeg")).dot(side) > 0 else -1.0
thigh_len = (world("LeftShin") - world("LeftLeg")).length + (world("LeftFoot") - world("LeftShin")).length
widen_deg = math.degrees(math.atan2(WIDEN_CM / 100.0 * scene.unit_settings.scale_length if False else WIDEN_CM * 0.01, thigh_len))
print("comprimento da perna %.3f | abrir %.2f graus" % (thigh_len, widen_deg))

keyed = ["Spine1", "Spine2", "Chest", "Neck1", "Head", "LeftLeg", "RightLeg", "LeftFoot", "RightFoot"]
for f in range(f0, f1 + 1):
    scene.frame_set(f)
    # balanço lateral atual do tronco (quadril -> pescoço), em graus para o lado
    v = world("Neck1") - world("Spine1")
    sway = math.degrees(math.atan2(v.dot(side), v.dot(up)))
    rotate_world("Spine1", fwd, -sway * (1.0 - SWAY_KEEP))   # sinal medido: negativo desfaz   # gira em volta da direção de caminhada: desfaz parte do balanço
    for name, deg in LEAN.items():
        rotate_world(name, pitch_axis, deg)
    for name, deg in HEAD_BACK.items():
        rotate_world(name, pitch_axis, deg)
    # pés para fora: coxa abre, pé volta a ficar reto
    for thigh, foot, sgn in (("LeftLeg", "LeftFoot", left_sign), ("RightLeg", "RightFoot", -left_sign)):
        rotate_world(thigh, fwd, -widen_deg * sgn)   # sinal medido: assim o pé vai para fora
        rotate_world(foot, fwd, widen_deg * sgn)
    for name in keyed:
        pb[name].keyframe_insert("rotation_quaternion", frame=f)

# medição depois
scene.frame_set(f0)
largura = []
for f in range(f0, f1 + 1, 3):
    scene.frame_set(f)
    largura.append(abs((world("LeftFoot") - world("RightFoot")).dot(side)))
print("largura média dos pés depois: %.1f cm" % (100 * sum(largura) / len(largura)))

bpy.ops.export_scene.fbx(filepath=dst, use_selection=False, object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                         bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                         bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
print("EXPORTADO", dst)
