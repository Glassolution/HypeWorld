"""Cria /Game/Ninja/Maps/Lvl_Treino: chão limpo, céu/luz e uma área pequena para testar movimento.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_mapa_treino.py
"""
import unreal

MAP_PATH = "/Game/Ninja/Maps/Lvl_Treino"
CUBE = unreal.load_asset("/Game/LevelPrototyping/Meshes/SM_Cube")  # 100x100x100 cm, pivô no canto
MAT_FLOOR = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray")
MAT_PROPS = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark")

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    raise RuntimeError(MAP_PATH + " já existe; não vou sobrescrever.")
if not les.new_level(MAP_PATH, False):  # mapa pequeno, sem World Partition (mais leve no M1)
    raise RuntimeError("Falha ao criar o mapa")


def spawn(cls, label, loc=(0, 0, 0), rot=(0, 0, 0), folder="Ambiente"):
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def block(label, size_m, center_m, mat=MAT_PROPS, folder="Teste"):
    """Bloco com tamanho e centro (base apoiada) em metros."""
    sx, sy, sz = size_m
    cx, cy, cz = center_m
    a = spawn(unreal.StaticMeshActor, label, (cx * 100 - sx * 50, cy * 100 - sy * 50, cz * 100), folder=folder)
    smc = a.static_mesh_component
    smc.set_static_mesh(CUBE)
    smc.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    return a


# --- Céu e luz (mesmos ajustes do mapa do template) ---
sun = spawn(unreal.DirectionalLight, "Sol", rot=(25.555, -60.779, -14.988))  # Rotator(roll, pitch, yaw)
sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property("intensity", 3.0)
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sky = spawn(unreal.SkyLight, "LuzDoCeu")
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("real_time_capture", True)
spawn(unreal.SkyAtmosphere, "Atmosfera")
spawn(unreal.VolumetricCloud, "Nuvens")
spawn(unreal.ExponentialHeightFog, "Neblina")

# --- Chão: 200 x 200 m, topo em z = 0 ---
block("Chao", (200, 200, 1), (0, 0, -1), mat=MAT_FLOOR, folder="Ambiente")

# --- Onde o jogador nasce ---
spawn(unreal.PlayerStart, "Inicio", loc=(0, 0, 100), folder="Ambiente")

# --- Área de teste (à direita do início, tudo num canto para o resto ficar livre) ---
block("Parede_CorridaNaParede", (30, 0.5, 6), (15, 12, 0))   # parede longa para correr nela
block("Plataforma_1m5", (4, 4, 1.5), (8, -10, 0))           # alturas para testar pulo e pulo duplo
block("Plataforma_3m", (4, 4, 3), (14, -10, 0))
block("Plataforma_4m5", (4, 4, 4.5), (20, -10, 0))

unreal.EditorLoadingAndSavingUtils.save_current_level()
print("OK:", MAP_PATH, "atores:", len(eas.get_all_level_actors()))
