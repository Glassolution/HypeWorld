"""Infusão de chakra nos pés (passiva para teste): montagens do super pulo, brilho nos pés, tecla V, passiva no personagem
e área de teste no Lvl_Treino (túnel com teto para andar na parede/teto e lago com rampa para andar na água).
Rodar com ue_exec -f, editor fora do Play.
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
BASE = "/Game/ANAP/Animations/Base/Base_Anim_Armature_CustomMan_"
OUT = "/Game/Ninja/Characters/Animations/ANAP"
save = []


def seq(path):
    a = unreal.load_asset(path)
    assert a, path
    return a


def montage_asset(name, first):
    m = unreal.load_asset("%s/%s" % (OUT, name))
    if not m:
        f = unreal.AnimMontageFactory()
        f.set_editor_property("source_animation", first)
        f.set_editor_property("target_skeleton", first.get_editor_property("skeleton"))
        m = tools.create_asset(name, OUT, unreal.AnimMontage, f)
    save.append(m.get_outermost())
    return m


# ---------------- Montagens do super pulo ----------------
# Carga: ChakraJump_Charge_Start (da base de luta até agachado, tronco à frente) e o Loop segurando a pose
carga_parts = [(BASE + "ChakraJump_Charge_Start", 0.0, -1.0, "Default"), (BASE + "ChakraJump_Charge_Loop", 0.0, -1.0, "Loop")]
carga = montage_asset("AM_ANAP_PuloChakra_Carga", seq(carga_parts[0][0]))
print("carga:", unreal.NinjaEditorTools.build_section_montage(carga, [seq(p) for p, _, _, _ in carga_parts], [a for _, a, _, _ in carga_parts],
      [b for _, _, b, _ in carga_parts], [s for _, _, _, s in carga_parts], ["Loop"], 0.1, 0.12, False), round(carga.get_play_length(), 2))
# Salto: impulso (ChakraJump_Charge_End) -> mergulho para a frente (Aerial_Dash_Start) -> sai do voo para a queda (Aerial_Dash_End)
salto_parts = [(BASE + "ChakraJump_Charge_End", 0.0, -1.0), (BASE + "Aerial_Dash_Start", 0.45, 1.0), (BASE + "Aerial_Dash_End", 0.40, 1.25)]
salto = montage_asset("AM_ANAP_PuloChakra_Salto", seq(salto_parts[0][0]))
print("salto:", unreal.NinjaEditorTools.build_sequence_montage(salto, [seq(p) for p, _, _ in salto_parts], [a for _, a, _ in salto_parts],
      [b for _, _, b in salto_parts], 0.05, 0.3, False, False), round(salto.get_play_length(), 2))

# ---------------- Materiais: brilho nos pés e água ----------------
def material(path, build):
    m = unreal.load_asset(path)
    if not m:
        folder, name = path.rsplit("/", 1)
        m = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    build(m)
    mel.recompile_material(m)
    save.append(m.get_outermost())
    return m


def pes(m):
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fres = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -400, 0)
    one = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -250, 0)
    mel.connect_material_expressions(fres, "", one, "")
    col = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 150)
    col.set_editor_property("constant", unreal.LinearColor(0.08, 0.3, 1.1, 1.0))
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -100, 50)
    mel.connect_material_expressions(col, "", mul, "A")
    mel.connect_material_expressions(one, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


def agua(m):
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    col = mel.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(0.02, 0.18, 0.32, 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 120)
    rough.set_editor_property("r", 0.05)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    op = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200)
    op.set_editor_property("r", 0.72)
    mel.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)


mat_pes = material("/Game/Ninja/VFX/Materials/M_Chakra_Pes", pes)
mat_agua = material("/Game/Ninja/VFX/Materials/M_Agua_Teste", agua)

# ---------------- Tecla V: liga/desliga a infusão ----------------
ia = unreal.load_asset("/Game/Ninja/Core/Input/IA_ChakraFeet") or tools.create_asset("IA_ChakraFeet", "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
save.append(ia.get_outermost())
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
used = {str(m.key.get_editor_property("key_name")): (m.action.get_name() if m.action else None)
        for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}
if used.get("V") is None:
    k = unreal.Key()
    k.set_editor_property("key_name", "V")
    IMC.map_key(ia, k)
elif used.get("V") != "IA_ChakraFeet":
    print("AVISO: V já usada por", used["V"])
save.append(IMC.get_outermost())

# ---------------- Personagem: passiva aprendida (para teste) ----------------
bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
cdo = unreal.get_default_object(bp.generated_class())
cdo.modify()
p = unreal.NinjaPassiveInfo()
p.set_editor_property("id", unreal.NinjaPassive.CHAKRA_FEET)
p.set_editor_property("name", unreal.Text("Infusão de Chakra nos pés"))
p.set_editor_property("description", unreal.Text("Andar na água, nas paredes e no teto; segurar Espaço = super pulo"))
p.set_editor_property("origin", unreal.Text("Aprendida"))
p.set_editor_property("learned", True)
p.set_editor_property("enabled_by_default", True)
cdo.set_editor_property("passives", [p])
cdo.set_editor_property("super_jump_charge_montage", carga)
cdo.set_editor_property("super_jump_launch_montage", salto)
cdo.set_editor_property("chakra_feet_material", mat_pes)
cdo.set_editor_property("chakra_feet_action", ia)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save.append(bp.get_outermost())

# ---------------- Área de teste no mapa ----------------
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
FOLDER = "Teste/Infusao"
for a in eas.get_all_level_actors():
    if str(a.get_folder_path()) == FOLDER:
        eas.destroy_actor(a)
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")


def caixa(label, centro, tamanho, pitch=0.0, material=None):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*centro), unreal.Rotator(pitch=pitch, yaw=0.0, roll=0.0))
    a.set_actor_label(label)
    a.set_folder_path(FOLDER)
    c = a.static_mesh_component
    c.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(tamanho[0] / 100.0, tamanho[1] / 100.0, tamanho[2] / 100.0))
    if material:
        c.set_material(0, material)
    return a


def placa(texto, pos, yaw=0.0):
    t = eas.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*pos), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    t.set_folder_path(FOLDER)
    t.set_actor_label("Placa_" + texto[:12])
    t.text_render.set_text(texto)
    t.text_render.set_editor_property("world_size", 40.0)
    t.text_render.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
    t.text_render.set_text_render_color(unreal.Color(r=120, g=200, b=255, a=255))


# Túnel: duas paredes de 7 m e um teto — subir a parede e passar para o teto
caixa("Tunel_ParedeA", (3500, -2425, 350), (1200, 50, 700))
caixa("Tunel_ParedeB", (3500, -1575, 350), (1200, 50, 700))
caixa("Tunel_Teto", (3500, -2000, 725), (1200, 900, 50))
placa("TÚNEL: ande contra a parede (infusão ligada) e suba até o teto", (2850, -2000, 260), yaw=180.0)

# Lago: bacia de 1,2 m (fundo = chão), água em cima; deque e rampa do lado oeste
LX, LY, NIVEL = 5000, 2800, 120
caixa("Lago_BordaNorte", (LX, LY + 825, NIVEL / 2), (1700, 50, NIVEL))
caixa("Lago_BordaSul", (LX, LY - 825, NIVEL / 2), (1700, 50, NIVEL))
caixa("Lago_BordaLeste", (LX + 825, LY, NIVEL / 2), (50, 1600, NIVEL))
caixa("Lago_Deque", (LX - 1100, LY, NIVEL / 2), (600, 1600, NIVEL))
caixa("Lago_Rampa", (LX - 1700, LY, NIVEL / 2 - 10), (620, 400, 20), pitch=11.3)
agua_sup = caixa("Lago_Agua", (LX, LY, NIVEL - 5), (1600, 1600, 10), material=mat_agua)
c = agua_sup.static_mesh_component
c.set_collision_profile_name("Custom")
c.set_collision_object_type(unreal.CollisionChannel.ECC_AGUA)   # canal "Agua" (DefaultEngine.ini)
c.set_collision_response_to_all_channels(unreal.CollisionResponseType.ECR_IGNORE)
c.set_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN, unreal.CollisionResponseType.ECR_BLOCK)
c.set_editor_property("cast_shadow", False)
placa("LAGO: com a infusão você anda na água (V liga/desliga)", (LX - 1400, LY, 330), yaw=180.0)
unreal.EditorLoadingAndSavingUtils.save_current_level()

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
cdo = unreal.get_default_object(unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
print("passivas:", [str(x.get_editor_property("name")) for x in cdo.get_editor_property("passives")])
print("água bloqueia Pawn:", c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN), "| tipo:", c.get_collision_object_type())
