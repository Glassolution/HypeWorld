"""Folha de quadros (tempo da ESQUERDA para a DIREITA; rótulo no começo da linha): poses de várias animações lado a lado (linhas = animações, colunas = momentos), no céu do mapa aberto,
foto pela câmera do editor e depois apaga os bonecos temporários (o mapa não é salvo).
Uso: ue_exec -f com ANIMS e ARQUIVO definidos antes do código.
"""
import unreal

try:
    ANIMS
except NameError:
    ANIMS = []
try:
    ARQUIVO
except NameError:
    ARQUIVO = "folha_quadros.png"
COLS = globals().get("COLS", 6)
MESH = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
ORIG = unreal.Vector(0, 0, 20000)
DZ, DX = 230.0, 150.0   # linhas (empilhadas para baixo) e colunas (lado a lado)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
temp = []
for r, path in enumerate(ANIMS):
    anim = unreal.load_asset(path)
    L = anim.get_play_length()
    for c in range(COLS):
        t = L * (c + 0.5) / COLS
        loc = ORIG + unreal.Vector(-c * DX, 0, -r * DZ)   # câmera olha +Y: -X fica à direita => tempo da esquerda para a direita
        a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0))
        comp = a.skeletal_mesh_component
        comp.set_skeletal_mesh_asset(MESH)
        comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        play = unreal.SingleAnimationPlayData()
        play.set_editor_property("anim_to_play", anim)
        play.set_editor_property("saved_position", t)
        play.set_editor_property("saved_playing", False)
        comp.set_editor_property("animation_data", play)
        comp.set_position(t, False)
        temp.append(a)
    lab = eas.spawn_actor_from_class(unreal.TextRenderActor, ORIG + unreal.Vector(130, 0, -r * DZ + 100), unreal.Rotator(pitch=0.0, yaw=-90.0, roll=0.0))
    lab.text_render.set_text("%d %s" % (r + 1, path.split("_CustomMan_")[-1].split("_ao_")[-1][:22]))
    lab.text_render.set_editor_property("world_size", 20.0)
    lab.text_render.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_RIGHT)  # rótulo à esquerda da linha (início do tempo)
    lab.text_render.set_text_render_color(unreal.Color(r=255, g=40, b=40, a=255))
    temp.append(lab)
# câmera de lado (bonecos olham para -X = para a esquerda na foto)
n = len(ANIMS)
mid = ORIG + unreal.Vector(-(COLS - 1) * DX / 2 + 60, 0, -(n - 1) * DZ / 2 + 90)
dist = max((COLS * DX + 300) * 0.55, n * DZ * 0.8) + 100
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(mid + unreal.Vector(0, -dist, 0), unreal.Rotator(pitch=0.0, yaw=90.0, roll=0.0))
unreal.AutomationLibrary.take_high_res_screenshot(2000, 1300, ARQUIVO)
open("/tmp/folha_temp_count.txt", "w").write(str(len(temp)))
print("bonecos temporários:", len(temp))
