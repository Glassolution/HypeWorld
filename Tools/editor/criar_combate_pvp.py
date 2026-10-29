"""Combate PvP: monta os golpes (montagens) e os combos, e a tecla F.
Base: os golpes da Epic feitos para este boneco (MM_Attack_01/02/03 e MM_ChargedAttack) — são uma corrente: o 1º termina
na pose em que o 2º começa e o 2º na do 3º, então emendam sem salto. Os chutes do Mixamo (pasta Luta) entram inteiros
(da guarda até a guarda) como finalizadores. Tempos de acerto = maior extensão da mão/pé, no relógio da animação original.
Rodar com: python3 -I Tools/ue_exec.py -f Tools/editor/criar_combate_pvp.py
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
EPIC = "/Game/Characters/Mannequins/Anims/Unarmed/Attack/"
LUTA = "/Game/Ninja/Characters/Animations/Mixamo/Luta/A_Ninja_MX_Luta_"
OUT = "/Game/Ninja/Characters/Animations/Combate"
save = []
R = unreal.NinjaHitReaction


def montage(name, anim, start, end, hits=(), combo=-1.0, blend_in=0.12, blend_out=0.25):
    """Montagem com o trecho [start, end] da animação, acertos e janela de combo."""
    seq = unreal.load_asset(anim)
    path = f"{OUT}/{name}"
    m = unreal.load_asset(path)
    if not m:
        f = unreal.AnimMontageFactory()
        f.set_editor_property("source_animation", seq)
        f.set_editor_property("target_skeleton", seq.get_editor_property("skeleton"))
        m = tools.create_asset(name, OUT, unreal.AnimMontage, f)
    # A habilidade termina quando o fim suave (blend out) começa: acertos e janela de combo têm que vir antes dele
    assert all(h < end - blend_out for h in hits) and combo < end - blend_out, (name, "acerto/combo depois do blend out")
    ok = unreal.NinjaEditorTools.build_attack_montage(m, seq, start, end, list(hits), combo, blend_in, blend_out)
    save.append(m.get_outermost())
    print("%-30s %s  %.2f-%.2fs (%.2fs)  acertos %s  combo %s" % (name, "ok" if ok else "FALHOU", start, end, m.get_play_length(), list(hits), combo))
    return m


def attack(name, m, damage, rate=1.15, reaction=R.LIGHT, knockback=150.0, launch=0.0, stun=0.65,
           reach=140.0, radius=55.0, breaks_guard=False, root_scale=1.0):
    a = unreal.NinjaAttack()
    for k, v in dict(name=name, montage=m, damage=damage, play_rate=rate, reaction=reaction, knockback=knockback, launch=launch,
                     stun=stun, reach=reach, radius=radius, breaks_guard=breaks_guard, root_motion_scale=root_scale).items():
        a.set_editor_property(k, v)
    return a


# --- Golpes -----------------------------------------------------------------------------------------------------------
# Combo da Epic (a janela de combo abre logo antes do ponto em que a pose casa com o começo do próximo: ~0,55 s)
direto     = montage("AM_Golpe_Direto",         EPIC + "MM_Attack_01", 0.00, 1.00, [0.40], 0.48, blend_in=0.12)
gancho_e   = montage("AM_Golpe_GanchoEsquerdo", EPIC + "MM_Attack_02", 0.00, 1.00, [0.45], 0.52, blend_in=0.10)
chute_soco = montage("AM_Golpe_ChuteESoco",     EPIC + "MM_Attack_03", 0.00, 1.67, [0.38, 0.80], 1.00, blend_in=0.10, blend_out=0.35)
carregado  = montage("AM_Golpe_SocoCarregado",  EPIC + "MM_ChargedAttack", 0.75, 1.83, [1.15], 1.30, blend_in=0.18, blend_out=0.3)
investida  = montage("AM_Golpe_Investida",      EPIC + "MM_Attack_01", 0.10, 1.00, [0.40], 0.48, blend_in=0.15)
# Chutes do Mixamo inteiros (guarda -> chute -> volta para a guarda)
circular   = montage("AM_Golpe_ChuteCircular",  LUTA + "Mma_Kick",   0.05, 1.35, [0.62], 0.95, blend_in=0.2, blend_out=0.3)
frontal    = montage("AM_Golpe_ChuteFrontal",   LUTA + "Mma_Kick_1", 0.05, 1.25, [0.62], 0.85, blend_in=0.2, blend_out=0.3)
giratorio  = montage("AM_Golpe_ChuteGiratorio", LUTA + "Mma_Kick_2", 0.55, 1.60, [0.95], 1.15, blend_in=0.2, blend_out=0.3)
aereo      = montage("AM_Golpe_ChuteAereo",     LUTA + "Mma_Kick_1", 0.25, 1.00, [0.62], 0.75, blend_in=0.12, blend_out=0.2)
# Reações
reac_leve  = montage("AM_Reacao_Costela",       LUTA + "Rib_Hit",    0.00, 0.85, blend_in=0.08, blend_out=0.2)
reac_forte = montage("AM_Reacao_Barriga",       LUTA + "Stomach_Hit", 0.00, 1.10, blend_in=0.08, blend_out=0.25)

# --- Combos -----------------------------------------------------------------------------------------------------------
light = [
    attack("Direto",           direto,     30, rate=1.15, knockback=380, stun=0.65, root_scale=0.6),
    attack("Gancho esquerdo",  gancho_e,   32, rate=1.15, knockback=380, stun=0.7, root_scale=0.6),
    attack("Chute e soco",     chute_soco, 30, rate=1.1,  reaction=R.KNOCKBACK, knockback=600, launch=200, stun=0.9, reach=160, radius=60, root_scale=0.6),
]
heavy = [
    attack("Soco carregado",   carregado, 70, rate=1.1,  reaction=R.KNOCKBACK, knockback=800, launch=250, stun=1.0, reach=150, breaks_guard=True),
    attack("Chute circular",   circular,  55, rate=1.25, reaction=R.KNOCKBACK, knockback=700, launch=200, stun=0.9, reach=170, radius=65, breaks_guard=True),
    attack("Chute frontal",    frontal,   55, rate=1.25, reaction=R.KNOCKBACK, knockback=750, stun=0.9, reach=170, breaks_guard=True),
    attack("Chute giratório",  giratorio, 65, rate=1.2,  reaction=R.LAUNCH, knockback=400, launch=600, stun=1.2, reach=170, radius=65, breaks_guard=True),
]
sprint = attack("Investida", investida, 45, rate=1.2, reaction=R.KNOCKBACK, knockback=700, stun=0.8, reach=150, root_scale=1.4)
air = attack("Chute aéreo", aereo, 40, rate=1.2, reaction=R.KNOCKBACK, knockback=500, stun=0.7, reach=160, radius=70)

# --- Tecla F (e RB no controle) para o golpe forte --------------------------------------------------------------------
IA = "/Game/Ninja/Core/Input/IA_HeavyAttack"
ia = unreal.load_asset(IA) or tools.create_asset("IA_HeavyAttack", "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
save.append(ia.get_outermost())
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
mappings = IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")
used = {str(m.key.get_editor_property("key_name")): (m.action.get_name() if m.action else None) for m in mappings}
for key_name in ("F", "Gamepad_RightShoulder"):
    if used.get(key_name) not in (None, "IA_HeavyAttack"):
        print("AVISO: tecla", key_name, "já usada por", used[key_name])
    if used.get(key_name) != "IA_HeavyAttack":
        k = unreal.Key()
        k.set_editor_property("key_name", key_name)
        IMC.map_key(ia, k)
save.append(IMC.get_outermost())

# --- Personagem e boneco ----------------------------------------------------------------------------------------------
hit_reacts = [unreal.load_asset("/Game/Ninja/Characters/Animations/AM_Ninja_HitReact_%d" % i) for i in (1, 2)] + [reac_leve]


ABP = unreal.load_asset("/Game/Ninja/Characters/Animations/ABP_Ninja")
abp_cdo = unreal.get_default_object(ABP.generated_class())
abp_cdo.modify()
abp_cdo.set_editor_property("fist_hands_pose", unreal.load_asset(EPIC + "MM_Attack_01"))
abp_cdo.set_editor_property("fist_pose_time", 0.4)
unreal.BlueprintEditorLibrary.compile_blueprint(ABP)
save.append(ABP.get_outermost())


def setup(cdo, with_attacks):
    cdo.modify()
    cdo.set_editor_property("hit_react_montages", hit_reacts)
    cdo.set_editor_property("heavy_hit_react_montage", reac_forte)
    if with_attacks:
        cdo.set_editor_property("light_combo", light)
        cdo.set_editor_property("heavy_attacks", heavy)
        cdo.set_editor_property("sprint_attack", sprint)
        cdo.set_editor_property("air_attack", air)
        cdo.set_editor_property("heavy_attack_action", ia)
        cdo.set_editor_property("attack_body_spacing", 85.0)   # distância mínima entre os corpos golpeando (cm)


for path, with_attacks in (("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", True), ("/Game/Ninja/Characters/BP_NinjaTrainingDummy", True)):
    bp = unreal.load_asset(path)
    setup(unreal.get_default_object(bp.generated_class()), with_attacks)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    save.append(bp.get_outermost())

# Um dos bonecos fica sempre defendendo (para treinar a quebra de defesa com o F)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "Boneco_Treino_2":
        a.set_editor_property("always_guard", True)
        print("Boneco_Treino_2 sempre defendendo")
unreal.EditorLoadingAndSavingUtils.save_current_level()

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
cdo = unreal.get_default_object(unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
print("combo leve:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("light_combo")])
print("fortes:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("heavy_attacks")])
print("correndo:", cdo.get_editor_property("sprint_attack").get_editor_property("name"), "| no ar:", cdo.get_editor_property("air_attack").get_editor_property("name"))
