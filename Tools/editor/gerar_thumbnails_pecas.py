"""Prévias das peças da criação de personagem (30/10/2026): /Game/HypeWorld/UI/CharacterSelect/Thumbs/T_Thumb_<Id>.
Para cada cabelo e traje (catálogo do UHypeAppearanceComponent) e cada parte de cima / de baixo / calçado do
DT_CustomizationParts: veste a peça no SK_Base_Body (pose de referência, sem animação), longe do mapa, e uma câmera de
captura (SceneCapture2D, só esses atores, fundo preto) fotografa a peça enquadrada pela caixa dela.
Imagens 384x150 (mesma proporção da área de prévia do card). Olhos não precisam: o card usa a textura da íris.
Duas etapas (texturas precisam de alguns segundos para carregar): rodar, esperar ~15 s, rodar de novo.
Pode rodar de novo: só gera as que faltam (FORCAR = True refaz todas).
Rodar (com o L_CharacterCreation aberto): python3 -I Tools/ue_exec.py -f Tools/editor/gerar_thumbnails_pecas.py
"""
import math
import unreal

FORCAR = False   # True = refaz todas (apaga e gera de novo só as T_Thumb_ deste script)
EAL = unreal.EditorAssetLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
tools = unreal.AssetToolsHelpers.get_asset_tools()

PASTA = "/Game/HypeWorld/UI/CharacterSelect/Thumbs"
LARGURA, ALTURA = 384, 150
FOV_H = 24.0
LONGE = unreal.Vector(0.0, 0.0, 60000.0)   # no céu, longe de tudo

CORPO = unreal.load_asset("/Game/HypeWorld/Characters/Base/SK_Base_Body")
PELE = unreal.load_asset("/Game/Ninja/Characters/Customization/Materials/MI_CreatorBody")

# --- Lista de peças: (id, malha, material ou None, máscara de corpo ou None) ---
pecas = []
comp_cdo = unreal.get_default_object(unreal.HypeAppearanceComponent)
for campo in ("hair_options", "outfit_options"):
    for p in comp_cdo.get_editor_property(campo):
        pecas.append((str(p.get_editor_property("id")), p.get_editor_property("mesh"), None, p.get_editor_property("body_mask")))
dt = unreal.load_asset("/Game/HypeWorld/Characters/Parts/DT_CustomizationParts")
colunas = {c: unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, c) for c in ("Slot", "Mesh", "Material")}
for i, linha in enumerate(unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)):
    slot = colunas["Slot"][i].split("::")[-1]
    if slot not in ("Top", "Bottom", "Shoes"):
        continue
    malha = unreal.load_asset(colunas["Mesh"][i].split("'")[1] if "'" in colunas["Mesh"][i] else colunas["Mesh"][i])
    mat_txt = colunas["Material"][i]
    material = unreal.load_asset(mat_txt.split("'")[1] if "'" in mat_txt else mat_txt) if mat_txt and mat_txt != "None" else None
    pecas.append((str(linha), malha, material, None))
print("peças:", len(pecas))

# --- Alvo de captura (asset, para virar textura) ---
if not EAL.does_asset_exist(f"{PASTA}/RT_Thumb"):
    tools.create_asset("RT_Thumb", PASTA, unreal.TextureRenderTarget2D, unreal.TextureRenderTargetFactoryNew())
rt = unreal.load_asset(f"{PASTA}/RT_Thumb")
rt.set_editor_property("size_x", LARGURA)
rt.set_editor_property("size_y", ALTURA)
rt.set_editor_property("render_target_format", unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
rt.set_editor_property("clear_color", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))

CABELO_PREVIA = unreal.LinearColor(0.32, 0.29, 0.27, 1.0)   # cinza quente: a forma aparece no fundo escuro
ESPACO = 400.0


def malha_ator(mesh, pos, material=None, rotulo=""):
    a = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, pos, unreal.Rotator(0, 0, 0))
    c = a.skeletal_mesh_component
    c.set_skinned_asset_and_update(mesh, True)
    if material:
        for s in range(c.get_num_materials()):
            c.set_material(s, material)
    c.set_editor_property("force_mip_streaming", True)
    a.set_folder_path("Criacao/TMP_Thumbs")
    a.set_actor_label(rotulo)
    return a


montados = {a.get_actor_label(): a for a in eas.get_all_level_actors() if str(a.get_folder_path()) == "Criacao/TMP_Thumbs"}
pendentes = [p for p in pecas if p[1] and (FORCAR or not EAL.does_asset_exist(f"{PASTA}/T_Thumb_{p[0]}"))]

if not montados:
    # Etapa 1: monta cada peça num corpo próprio (em fila, no céu) e pede as texturas inteiras. Rodar de novo depois
    # de ~15 s para fotografar (a captura no mesmo comando sairia com texturas ainda carregando = tudo cinza).
    for i, (pid, malha, material, mascara) in enumerate(pendentes):
        pos = unreal.Vector(LONGE.x + i * ESPACO, LONGE.y, LONGE.z)
        corpo = malha_ator(CORPO, pos, None, f"TMPCORPO_{pid}")
        pele = corpo.skeletal_mesh_component.create_dynamic_material_instance(0, PELE)
        m = mascara if mascara else unreal.NinjaBodyMask()
        for nome, valor in (("Mask_TorsoMinZ", m.torso_min_z), ("Mask_TorsoMaxZ", m.torso_max_z), ("Mask_ArmMaxT", m.arm_max_t),
                            ("Mask_LegMinZ", m.leg_min_z), ("Mask_FootMaxZ", m.foot_max_z)):
            pele.set_scalar_parameter_value(nome, valor)
        peca = malha_ator(malha, pos, material, f"TMPPECA_{pid}")
        if pid.startswith("Hair_"):
            c = peca.skeletal_mesh_component
            for s in range(c.get_num_materials()):
                mid = c.create_dynamic_material_instance(s, c.get_material(s))
                mid.set_vector_parameter_value("BaseColor Tint", CABELO_PREVIA)
        for ator in (corpo, peca):
            ator.skeletal_mesh_component.prestream_textures(30.0, True, 0)
    print("etapa 1: montadas", len(pendentes), "peças. Espere ~15 s e rode de novo para fotografar.")
else:
    captura = eas.spawn_actor_from_class(unreal.SceneCapture2D, LONGE, unreal.Rotator(0, 0, 0))
    cc = captura.capture_component2d
    cc.set_editor_property("texture_target", rt)
    cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    cc.set_editor_property("primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
    cc.set_editor_property("capture_every_frame", False)
    cc.set_editor_property("capture_on_movement", False)
    cc.set_editor_property("fov_angle", FOV_H)
    # Fundo preto: sem céu, neblina e nuvens na foto
    cc.set_editor_property("show_flag_settings", [unreal.EngineShowFlagsSetting(show_flag_name=n, enabled=False)
                                                  for n in ("Atmosphere", "Fog", "VolumetricFog", "Cloud", "Bloom")])
    FOV_V = 2.0 * math.degrees(math.atan(math.tan(math.radians(FOV_H) * 0.5) * ALTURA / LARGURA))
    feitos = 0
    for pid, malha, material, mascara in pendentes:
        corpo, peca = montados.get(f"TMPCORPO_{pid}"), montados.get(f"TMPPECA_{pid}")
        if not corpo or not peca:
            continue
        origem, extensao = peca.get_actor_bounds(False)
        # Câmera de frente (o corpo olha para +Y), girada 20°, longe o bastante para caber a caixa da peça
        raio_v = max(extensao.z, 6.0) * 1.18
        raio_h = max(extensao.x, 6.0) * 1.18
        dist = max(raio_v / math.tan(math.radians(FOV_V) * 0.5), raio_h / math.tan(math.radians(FOV_H) * 0.5)) + extensao.y
        giro = math.radians(20.0)
        pos = unreal.Vector(origem.x + math.sin(giro) * dist, origem.y + math.cos(giro) * dist, origem.z + 4.0)
        captura.set_actor_location_and_rotation(pos, unreal.MathLibrary.find_look_at_rotation(pos, origem), False, False)
        cc.clear_show_only_components()
        cc.show_only_actor_components(corpo)
        cc.show_only_actor_components(peca)
        cc.capture_scene()
        if EAL.does_asset_exist(f"{PASTA}/T_Thumb_{pid}"):
            EAL.delete_asset(f"{PASTA}/T_Thumb_{pid}")   # prévia gerada por este script: refeita
        tex = unreal.RenderingLibrary.render_target_create_static_texture2d_editor_only(rt, f"T_Thumb_{pid}",
            unreal.TextureCompressionSettings.TC_DEFAULT, unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        if tex:
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            EAL.save_asset(tex.get_path_name().split(".")[0], only_if_is_dirty=False)
            feitos += 1
    for ator in list(montados.values()) + [captura]:
        eas.destroy_actor(ator)
    print("etapa 2: prévias geradas:", feitos)
