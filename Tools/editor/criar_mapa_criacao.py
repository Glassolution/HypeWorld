"""Mapa da criação de personagem (30/10/2026): /Game/Maps/CharacterCreation/L_CharacterCreation.
Recorte do L_Tutorial: cópia do mapa com o personagem em pé na trilha (x 0, y 0), de frente para a câmera, e ao fundo
a trilha, a ponte, o portal e as árvores (escolhido por fotos de 5 pontos da trilha). O que fica atrás da câmera ou
longe e fora do quadro é apagado da CÓPIA (o L_Tutorial não muda). Luz de estúdio no personagem: key (frente-esquerda,
quente), fill (frente-direita, fraca) e rim (atrás, fria), só perto dele.
A câmera não fica no mapa: a tela (UHypeCharacterSelectWidget) cria a dela à frente do personagem.
Também liga o fluxo: jogo abre no L_Boot → tela de carregamento (MapaSeguinte = este mapa) → criação → L_Tutorial.
Pode rodar de novo: o mapa só é copiado uma vez; palco e luzes (pasta Criacao) são refeitos.
Rodar: python3 -I Tools/ue_exec.py -f Tools/editor/criar_mapa_criacao.py
"""
import math
import unreal

EAL = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

ORIGEM = "/Game/Maps/Tutorial/L_Tutorial"
MAPA = "/Game/Maps/CharacterCreation/L_CharacterCreation"
PALCO = unreal.Vector(0.0, 0.0, 0.0)   # x, y do personagem (z = chão)
OLHA_PARA = 90.0                       # yaw do personagem: de frente para +Y (a câmera fica em +Y olhando o portal)

copiou = False
if not EAL.does_asset_exist(MAPA):
    # "Salvar como": abre o L_Tutorial e grava com outro nome (duplicate_asset de mapa trava o editor no load seguinte)
    les.load_level(ORIGEM)
    if not unreal.EditorLoadingAndSavingUtils.save_map(ues.get_editor_world(), MAPA):
        raise RuntimeError("não consegui gravar a cópia do L_Tutorial")
    copiou = True
if ues.get_editor_world().get_outermost().get_name() != MAPA:
    les.load_level(MAPA)
world = ues.get_editor_world()


def chao(x, y):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 5000), unreal.Vector(x, y, -5000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[4].z if hit else 0.0


PALCO.z = chao(PALCO.x, PALCO.y)

# --- Recorte (só na primeira vez): apaga o que não aparece no fundo ---
FICA = ("Landscape", "DirectionalLight", "SkyLight", "ExponentialHeightFog", "PostProcessVolume", "SkyAtmosphere",
        "VolumetricCloud", "SphereReflectionCapture", "InstancedFoliageActor", "WaterBrushManager", "WaterZone",
        "WaterBodyRiver", "RuntimeVirtualTextureVolume", "WorldSettings", "LandscapeStreamingProxy")
if copiou:
    apagados = 0
    for a in eas.get_all_level_actors():
        cls = a.get_class().get_name()
        if any(cls.startswith(f) for f in FICA):
            continue
        # Coisas de jogo do tutorial: início, alvos, câmeras de conferência
        if cls in ("PlayerStart", "TargetPoint", "CameraActor"):
            eas.destroy_actor(a)
            apagados += 1
            continue
        p = a.get_actor_location()
        dx, dy = p.x - PALCO.x, p.y - PALCO.y
        atras = dy > 1500.0                                  # atrás da câmera (câmera em y ~ +440)
        dist = math.hypot(dx, dy)
        no_quadro = dy < 0 and abs(dx) < (-dy) * 0.9 + 3000.0  # cone para -Y, com folga
        if atras or (dist > 9000.0 and not no_quadro):
            eas.destroy_actor(a)
            apagados += 1
    print("recorte: apagados", apagados, "atores")

# --- Palco: refeito a cada execução ---
for a in eas.get_all_level_actors():
    if str(a.get_folder_path()).startswith("Criacao"):
        eas.destroy_actor(a)

inicio = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(PALCO.x, PALCO.y, PALCO.z + 100.0),
                                    unreal.Rotator(pitch=0.0, yaw=OLHA_PARA, roll=0.0))
inicio.set_actor_label("PlayerStart_Criacao")
inicio.set_folder_path("Criacao")

PEITO = unreal.Vector(PALCO.x, PALCO.y, PALCO.z + 130.0)


def luz(nome, deslocamento, candelas, cor, cone):
    """Spot apontado para o peito do personagem; deslocamento em cm (x = direita da tela, y = para a câmera)"""
    pos = unreal.Vector(PALCO.x + deslocamento[0], PALCO.y + deslocamento[1], PALCO.z + deslocamento[2])
    rot = unreal.MathLibrary.find_look_at_rotation(pos, PEITO)
    a = eas.spawn_actor_from_class(unreal.SpotLight, pos, rot)
    a.set_actor_label(nome)
    a.set_folder_path("Criacao/Luzes")
    c = a.spot_light_component
    c.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    c.set_intensity(candelas)
    c.set_light_color(cor)
    c.set_editor_property("outer_cone_angle", cone)
    c.set_editor_property("inner_cone_angle", cone * 0.5)
    c.set_editor_property("attenuation_radius", 900.0)
    c.set_editor_property("source_radius", 40.0)
    c.set_editor_property("soft_source_radius", 80.0)
    c.set_editor_property("cast_shadows", False)
    a.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    return a


# Sol do mapa: 4 lux (exposição manual). Iluminância ≈ candelas / distância² (m)
luz("Luz_Key", (-260.0, 320.0, 260.0), 46.0, unreal.LinearColor(1.0, 0.92, 0.82, 1.0), 26.0)
luz("Luz_Fill", (300.0, 260.0, 120.0), 14.0, unreal.LinearColor(0.85, 0.9, 1.0, 1.0), 32.0)
luz("Luz_Rim", (140.0, -320.0, 300.0), 70.0, unreal.LinearColor(0.75, 0.86, 1.0, 1.0), 22.0)

les.save_current_level()
print("palco em", PALCO, "| salvo", MAPA)

# --- Fluxo: carregamento → criação → jogo ---
CARREGAMENTO = "/Game/HypeWorld/UI/Loading/WBP_LoadingScreen"
if EAL.does_asset_exist(CARREGAMENTO):
    wbp = unreal.load_asset(CARREGAMENTO)
    cdo = unreal.get_default_object(wbp.generated_class())
    cdo.set_editor_property("mapa_seguinte", unreal.load_asset(MAPA))
    # A tela de criação é aberta pelo AHypeWorldPlayerController no mapa de criação (não pela tela de carregamento)
    cdo.set_editor_property("tela_ao_entrar", None)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    print("carregamento → ", cdo.get_editor_property("mapa_seguinte").get_path_name(),
          "| salvo", EAL.save_asset(CARREGAMENTO, only_if_is_dirty=False))
