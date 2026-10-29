"""Fase 2: teclas de combate, montagens de reação, personagem, boneco de treino e mapa.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/configurar_combate.py
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
save = []
ANIMS = "/Game/Characters/Mannequins/Anims/"
NINJA_ANIMS = "/Game/Ninja/Characters/Animations/"
VFX = "/Game/Ninja/VFX/Materials/"
SK = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")


def key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k


# --- Teclas ---
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
existing = {(m.action.get_name() if m.action else None, str(m.key.get_editor_property("key_name"))) for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}
actions = {}
for name, keys in {"IA_Attack": ["LeftMouseButton", "Gamepad_FaceButton_Left"], "IA_ChargeChakra": ["C", "Gamepad_FaceButton_Top"]}.items():
    path = "/Game/Ninja/Core/Input/" + name
    ia = unreal.load_asset(path) or tools.create_asset(name, "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
    ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    save.append(ia.get_outermost())
    for k in keys:
        if (name, k) not in existing:
            IMC.map_key(ia, key(k))
    actions[name] = ia
dash = unreal.load_asset("/Game/Ninja/Core/Input/IA_Dash")
IMC.unmap_key(dash, key("Q"))                       # Q fica livre para o 1º jutsu
if ("IA_Dash", "LeftControl") not in existing:
    IMC.map_key(dash, key("LeftControl"))
save.append(IMC.get_outermost())
print("teclas:", sorted((m.action.get_name(), str(m.key.get_editor_property("key_name"))) for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings") if m.action and m.action.get_name() in ("IA_Attack", "IA_ChargeChakra", "IA_Dash")))


# --- Montagens de reação e nocaute ---
def montage(name, source, blend_in=0.08, blend_out=0.2):
    path = NINJA_ANIMS + name
    m = unreal.load_asset(path)
    if not m:
        f = unreal.AnimMontageFactory()
        f.set_editor_property("source_animation", unreal.load_asset(source))
        f.set_editor_property("target_skeleton", SK)
        m = tools.create_asset(name, NINJA_ANIMS.rstrip("/"), unreal.AnimMontage, f)
    b = m.get_editor_property("blend_in"); b.set_editor_property("blend_time", blend_in); m.set_editor_property("blend_in", b)
    b = m.get_editor_property("blend_out"); b.set_editor_property("blend_time", blend_out); m.set_editor_property("blend_out", b)
    save.append(m.get_outermost())
    return m


hit_reacts = [montage("AM_Ninja_HitReact_%d" % i, ANIMS + "Rifle/HitReact/" + src)
              for i, src in enumerate(["MM_HitReact_Front_Lgt_01", "MM_HitReact_Front_Lgt_02", "MM_HitReact_Front_Med_01"], 1)]
knockout = montage("AM_Ninja_Knockout", ANIMS + "Death/MM_Death_Front_01", 0.1, 0.4)
knockout.set_editor_property("enable_auto_blend_out", False)   # fica caído até levantar
combo = unreal.load_asset("/Game/Variant_Combat/Anims/AM_ComboAttack")


def setup_ninja(cdo):
    cdo.set_editor_property("hit_react_montages", hit_reacts)
    cdo.set_editor_property("knockout_montage", knockout)
    cdo.set_editor_property("hit_flash_material", unreal.load_asset(VFX + "M_HitFlash"))
    aura = cdo.get_editor_property("chakra_aura")
    aura.set_editor_property("shell_material", unreal.load_asset(VFX + "M_Chakra_Shell"))
    aura.set_editor_property("flame_material", unreal.load_asset(VFX + "M_Chakra_ToonFlame"))
    aura.set_editor_property("ground_material", unreal.load_asset(VFX + "M_Chakra_GroundGlow"))
    aura.set_editor_property("spark_material", unreal.load_asset(VFX + "M_Chakra_Spark"))


# --- Personagem do jogador ---
PLAYER = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
bp = unreal.load_asset(PLAYER)
cdo = unreal.get_default_object(bp.generated_class())
cdo.modify()
cdo.set_editor_property("attack_action", actions["IA_Attack"])
cdo.set_editor_property("charge_chakra_action", actions["IA_ChargeChakra"])
setup_ninja(cdo)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save.append(bp.get_outermost())

# --- Boneco de treino (BP filho do C++, com o mesmo corpo e animação do jogador) ---
DUMMY = "/Game/Ninja/Characters/BP_NinjaTrainingDummy"
dbp = unreal.load_asset(DUMMY)
if not dbp:
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", unreal.NinjaTrainingDummy)
    dbp = tools.create_asset("BP_NinjaTrainingDummy", "/Game/Ninja/Characters", unreal.Blueprint, f)
dcdo = unreal.get_default_object(dbp.generated_class())
dcdo.modify()
pmesh = cdo.get_editor_property("mesh")
dmesh = dcdo.get_editor_property("mesh")
manny = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple") or pmesh.get_editor_property("skeletal_mesh_asset")
dmesh.set_editor_property("skeletal_mesh_asset", manny)
dmesh.set_editor_property("anim_class", pmesh.get_editor_property("anim_class"))
dmesh.set_editor_property("relative_location", pmesh.get_editor_property("relative_location"))
dmesh.set_editor_property("relative_rotation", pmesh.get_editor_property("relative_rotation"))
setup_ninja(dcdo)
unreal.BlueprintEditorLibrary.compile_blueprint(dbp)
save.append(dbp.get_outermost())

# --- Mapa: dois bonecos na frente do início ---
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Boneco_Treino"):
        eas.destroy_actor(a)
for i, loc in enumerate([unreal.Vector(450, -150, 100), unreal.Vector(450, 150, 100)]):
    d = eas.spawn_actor_from_class(dbp.generated_class(), loc, unreal.Rotator(0, 0, 180))
    d.set_actor_label("Boneco_Treino_%d" % (i + 1))
    d.set_folder_path("Teste")
unreal.EditorLoadingAndSavingUtils.save_current_level()

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
print("personagem:", len(cdo.get_editor_property("hit_react_montages")), "reações (golpes: criar_combate_pvp.py)")
print("boneco:", dbp.get_name(), "| malha:", dmesh.get_editor_property("skeletal_mesh_asset").get_name())
