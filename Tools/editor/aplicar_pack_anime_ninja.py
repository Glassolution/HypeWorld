"""Troca TODAS as animações do personagem (e do boneco de treino) pelas do pack comprado "Anime Ninja Animation" (/Game/ANAP).
O pack já usa o esqueleto do manequim (sem retarget) e as animações são "no lugar" (sem root motion): o deslocamento
continua vindo do código de movimento. O pack só tem andar/correr para a frente, então o corpo passa a virar para onde anda
(sem strafe; botão direito ainda deixa de costas para a câmera).
Nada é apagado: as montagens antigas (Epic/Mixamo/DeepMotion) continuam no projeto, só deixam de ser usadas.
Antes: corrigir_ik_pack (ossos guia dos pés). Rodar com ue_exec -f, editor fora do Play.
"""
import re
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
BASE = "/Game/ANAP/Animations/Base/Base_Anim_Armature_CustomMan_"
ATK = "/Game/ANAP/Animations/Attack/HandBlades/HandBlades_Anim_Armature_CustomMan_Attack_"
MISC = "/Game/ANAP/Animations/Misc/Misc_Anim_Armature_CustomMan_"
OUT = "/Game/Ninja/Characters/Animations/ANAP"
ABP = "/Game/Ninja/Characters/Animations/ABP_Ninja"
R = unreal.NinjaHitReaction
save = []


def seq(path):
    a = unreal.load_asset(path)
    assert a, path
    return a


def new_montage(name, first_anim):
    path = f"{OUT}/{name}"
    m = unreal.load_asset(path)
    if not m:
        f = unreal.AnimMontageFactory()
        f.set_editor_property("source_animation", first_anim)
        f.set_editor_property("target_skeleton", first_anim.get_editor_property("skeleton"))
        m = tools.create_asset(name, OUT, unreal.AnimMontage, f)
    save.append(m.get_outermost())
    return m


def sequence(name, parts, blend_in=0.15, blend_out=0.25, loop_last=False, hold=False):
    """Montagem com trechos em sequência: parts = [(animação, início, fim)] (fim -1 = até o fim)."""
    anims = [seq(p) for p, _, _ in parts]
    m = new_montage(name, anims[0])
    ok = unreal.NinjaEditorTools.build_sequence_montage(m, anims, [float(s) for _, s, _ in parts], [float(e) for _, _, e in parts],
                                                         blend_in, blend_out, loop_last, hold)
    print("%-28s %s %.2fs %s" % (name, "ok" if ok else "FALHOU", m.get_play_length(), "(repete)" if loop_last else ""))
    return m


def attack_montage(name, anim, start, end, hits, combo=-1.0, blend_in=0.1, blend_out=0.25):
    a = seq(anim)
    m = new_montage(name, a)
    assert all(h < end - blend_out for h in hits) and combo < end - blend_out, (name, "acerto/combo depois do blend out")
    ok = unreal.NinjaEditorTools.build_attack_montage(m, a, start, end, list(hits), combo, blend_in, blend_out)
    print("%-28s %s %.2f-%.2fs acertos %s combo %s" % (name, "ok" if ok else "FALHOU", start, end, list(hits), combo))
    return m


def attack(name, m, damage, rate=1.15, reaction=R.LIGHT, knockback=300.0, launch=0.0, stun=0.65,
           reach=150.0, radius=60.0, breaks_guard=False):
    a = unreal.NinjaAttack()
    for k, v in dict(name=name, montage=m, damage=damage, play_rate=rate, reaction=reaction, knockback=knockback, launch=launch,
                     stun=stun, reach=reach, radius=radius, breaks_guard=breaks_guard, root_motion_scale=1.0).items():
        a.set_editor_property(k, v)
    return a


# --- Locomoção: parado / andar / correr (velocidade natural medida: andar 113 cm/s, corrida ninja 491 cm/s) ---------------
IDLE = seq(BASE + "CharaSelect_Idle")   # parado comum (em pé, neutro) — o Felipe reprovou o do Sasuke
BATTLE_IDLE = seq(BASE + "Idle_Loop")   # parado em modo de batalha (base de luta) — misturado por código (NinjaAnimInstance)
WALK = seq(BASE + "Walk_Type01_Loop")  # andar comum (reto) — natural 75 cm/s; o Walk_Loop é agachado, parece de luta
TROT = seq("/Game/Ninja/Characters/Animations/Mixamo/A_Ninja_MX_Avulsa_Run2_Forward")  # trote (Shift 1 clique) — Mixamo Run2 (natural 560), mesmo conjunto dos passos laterais Run2_Left/Right; o Run_Type01_Loop do pack (natural 146, tocado 2,4x) ficou feio
RUN = seq(BASE + "Run_Loop")          # corrida ninja (Shift 2º clique, braços para trás) — natural 491 cm/s
WALK_SPEED, RUN_SPEED, SPRINT_SPEED = 150.0, 520.0, 800.0
RUN_STRAFE_SPEED = 430.0   # trote de lado (Run2_Left 428 / Right 342 natural)  # amostras do blend space = velocidades do jogo

BS_DST = OUT + "/BS_ANAP_Locomocao"
if not unreal.EditorAssetLibrary.does_asset_exist(BS_DST):
    unreal.EditorAssetLibrary.duplicate_asset("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run", BS_DST)
bs = unreal.load_asset(BS_DST)
params = list(bs.get_editor_property("blend_parameters"))
params[1].set_editor_property("max", SPRINT_SPEED)
params[1].set_editor_property("grid_num", 160)
for prm in params[:2]:
    prm.set_editor_property("snap_to_grid", False)
bs.set_editor_property("blend_parameters", params)
directions = sorted({round(s.get_editor_property("sample_value").x) for s in bs.get_editor_property("sample_data")})
# Andando/trotando o corpo fica de frente para a câmera (A/D = passo lateral, S = de costas). O pack só tem para a frente:
# lados e costas vêm do Mixamo (Walk2/Run2, já no esqueleto do manequim). Corrida ninja: o corpo vira para onde corre (só frente).
MX = "/Game/Ninja/Characters/Animations/Mixamo/A_Ninja_MX_Avulsa_"
BACK_SPEED = 220.0   # trote para trás = andar de costas acelerado (não existe corrida de costas)
grid = [(d, 0.0, IDLE) for d in directions]
grid += [(0.0, WALK_SPEED, WALK), (-90.0, WALK_SPEED, seq(MX + "Walk2_Left")), (90.0, WALK_SPEED, seq(MX + "Walk2_Right")),
         (-180.0, WALK_SPEED, seq(MX + "Walk2_Backward")), (180.0, WALK_SPEED, seq(MX + "Walk2_Backward")),
         (-180.0, BACK_SPEED, seq(MX + "Walk2_Backward")), (180.0, BACK_SPEED, seq(MX + "Walk2_Backward")),
         (0.0, RUN_SPEED, TROT), (-90.0, RUN_STRAFE_SPEED, seq(MX + "Run2_Left")), (90.0, RUN_STRAFE_SPEED, seq(MX + "Run2_Right")),
         (0.0, SPRINT_SPEED, RUN)]
samples = []
for d, speed, anim in grid:
    s = unreal.BlendSample()
    s.set_editor_property("animation", anim)
    s.set_editor_property("sample_value", unreal.Vector(float(d), speed, 0.0))
    samples.append(s)
bs.set_editor_property("sample_data", samples)
print("blend space refeito:", unreal.NinjaEditorTools.rebuild_blend_space(bs), len(samples), "amostras")
unreal.EditorAssetLibrary.save_asset(BS_DST, only_if_is_dirty=False)

# --- ABP_Ninja: nós de animação (exporta o nó como texto, troca o caminho, importa de volta) --------------------------------
NODE_BASE = ABP + ".ABP_Ninja:AnimGraph."
NODES = {
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_1.Idle.AnimGraphNode_SequencePlayer_1": ("Sequence", IDLE),
    "AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_2.Walk / Run.AnimGraphNode_BlendSpacePlayer_0": ("BlendSpace", bs),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_1.Jump.AnimGraphNode_SequencePlayer_1": ("Sequence", seq(BASE + "Jump_Front")),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_2.Fall Loop.AnimGraphNode_SequencePlayer_3": ("Sequence", seq(BASE + "Fall_Vertical_Loop")),
    "AnimGraphNode_StateMachine_1.Main States.AnimStateNode_3.Land.AnimGraphNode_SequencePlayer_4": ("Sequence", seq(BASE + "Land")),
}
for path, (field, asset) in NODES.items():
    graph_node = unreal.find_object(None, NODE_BASE + path)
    node = graph_node.get_editor_property("node")
    cls = "AnimSequence" if field == "Sequence" else "BlendSpace"
    new_text, n = re.subn(field + r'="[^"]*"', '%s="/Script/Engine.%s\'%s\'"' % (field, cls, asset.get_path_name()), node.export_text(), count=1)
    assert n == 1, path
    node.import_text(new_text)
    graph_node.set_editor_property("node", node)
    print("ABP:", path.split(".")[-2], "->", asset.get_name().split("_CustomMan_")[-1])
abp = unreal.load_asset(ABP)
abp_cdo = unreal.get_default_object(abp.generated_class())
abp_cdo.modify()
abp_cdo.set_editor_property("procedural_pose", False)   # sem retoques por código: só o pack
abp_cdo.set_editor_property("battle_idle_pose", BATTLE_IDLE)
unreal.BlueprintEditorLibrary.compile_blueprint(abp)
save.append(abp.get_outermost())

# --- Montagens de movimento -------------------------------------------------------------------------------------------
# Dash: DashStep_Front = mergulho + voo horizontal (0-0,55 s), aterrissagem (0,55-1,0 s) e depois volta à base de luta
# (não usar o fim!). Default = mergulho/voo desde o clique; Loop = voo (Aerial_Dash_Loop sem as pontas, que são em pé)
# se o dash durar mais; End = aterrissagem, que o código chama quando o dash acaba parado (correndo, volta direto à corrida)
DS = BASE + "DashStep_Front"
dash_parts = [(DS, 0.0, 0.55), (BASE + "Aerial_Dash_Loop", 0.05, 0.37), (DS, 0.55, -1)]   # End até ele se levantar (antes cortava agachado)
dash = new_montage("AM_ANAP_Dash", seq(DS))
print("AM_ANAP_Dash (seções)", unreal.NinjaEditorTools.build_section_montage(dash, [seq(p) for p, _, _ in dash_parts], [float(a) for _, a, _ in dash_parts],
      [float(b) for _, _, b in dash_parts], ["Default", "Loop", "End"], ["Loop"], 0.22, 0.25, False))   # entrada do dash 0,22 s (0,06 pulava)
# Pulo duplo: DoubleJump = agacha (0-0,3) -> salta -> mortal -> termina em pose de queda (1,5-1,75), parecida com a
# Fall_Vertical_Loop. Usa do salto ao fim (0,36-1,75) 2,2x mais rápido (~0,63 s = tempo no ar) e emenda suave na queda
# (blend out longo); se pousar antes, o código para a montagem e entra o Land
double_jump = sequence("AM_ANAP_PuloDuplo", [(BASE + "DoubleJump", 0.36, -1)], blend_in=0.05, blend_out=0.3)
double_jump.set_editor_property("rate_scale", 2.2)
wall_run = sequence("AM_ANAP_CorridaParede", [(BASE + "Run_Loop", 0, -1)], blend_in=0.15, blend_out=0.2, loop_last=True)
wall_jump = sequence("AM_ANAP_PuloParede", [(BASE + "Jump_Front", 0.05, -1)], blend_in=0.08, blend_out=0.3)
charge = sequence("AM_ANAP_CarregarChakra", [(BASE + "RecoveryChakra_Start", 0, -1), (BASE + "RecoveryChakra_Loop", 0, -1)], blend_in=0.15, blend_out=0.25, loop_last=True)

# --- Reações e nocaute ------------------------------------------------------------------------------------------------
reacts = [sequence("AM_ANAP_Reacao_Cabeca", [(MISC + "Beaten_UpFront_Head", 0, -1)], 0.06, 0.25),
          sequence("AM_ANAP_Reacao_Ombro", [(MISC + "Beaten_UpLeft_Shoulder", 0, -1)], 0.06, 0.25),
          sequence("AM_ANAP_Reacao_Lado", [(MISC + "Beaten_Left_Head", 0, -1)], 0.06, 0.25)]
heavy_react = sequence("AM_ANAP_Reacao_Forte", [(MISC + "Beaten_Blasted", 0, -1)], 0.06, 0.3)
knockout = sequence("AM_ANAP_Nocaute", [(MISC + "Beaten_OneHitDown_Long", 0, -1)], 0.08, 0.3, hold=True)

# --- Golpes (acertos = mão/pé mais à frente, medidos no relógio da animação) ----------------------------------------------
jab      = attack_montage("AM_ANAP_Golpe_Jab",         ATK + "ChakraFistA_cmb_00", 0.0, 1.04, [0.30], 0.42)
direto   = attack_montage("AM_ANAP_Golpe_Direto",      ATK + "ChakraFist_B_StraightPunch", 0.0, 1.17, [0.40], 0.52)
gancho   = attack_montage("AM_ANAP_Golpe_Gancho",      ATK + "ChakraGloves_B_Uppercut", 0.0, 1.67, [0.60], 0.78)
giro     = attack_montage("AM_ANAP_Golpe_ChuteGiro",   ATK + "ChakraFist_B_DoubleTurnKick_Large", 0.0, 1.67, [0.20, 0.45], blend_out=0.3)
forte    = attack_montage("AM_ANAP_Golpe_SocoForte",   ATK + "ChakraFistA_StraightPunch", 0.0, 1.80, [0.80], blend_out=0.35)
socos_up = attack_montage("AM_ANAP_Golpe_SocosGancho", ATK + "ChakraGloves_B_DoublePunch_Uppercut", 0.0, 1.83, [0.20, 0.43, 0.83], blend_out=0.3)
dragao   = attack_montage("AM_ANAP_Golpe_ChuteDragao", ATK + "ChakraFistA_DragonKick", 0.0, 1.58, [0.62], blend_out=0.3)
investida = attack_montage("AM_ANAP_Golpe_Investida",  ATK + "Hand_Punch_Left", 0.0, 1.12, [0.18], blend_out=0.3)
aereo    = attack_montage("AM_ANAP_Golpe_ChuteAereo",  ATK + "Aerial_cmb04", 0.0, 0.96, [0.22], blend_out=0.25)

light = [
    # Atordoamento dimensionado para a corrente de 4 golpes sempre encaixar mesmo com a queda por golpe seguido (HitstunDecay)
    attack("Jab",             jab,    25, rate=1.2, knockback=200, stun=0.75),
    attack("Direto",          direto, 28, rate=1.2, knockback=220, stun=0.85),
    attack("Gancho",          gancho, 30, rate=1.25, knockback=220, launch=150, stun=0.95),
    attack("Chute giratório", giro,   35, rate=1.15, reaction=R.KNOCKBACK, knockback=650, launch=200, stun=0.9, reach=170, radius=70),
]
heavy = [
    attack("Soco forte",         forte,    70, rate=1.15, reaction=R.KNOCKBACK, knockback=800, launch=250, stun=1.0, breaks_guard=True),
    attack("Socos e gancho",     socos_up, 60, rate=1.2, reaction=R.LAUNCH, knockback=300, launch=600, stun=1.2, breaks_guard=True),
    attack("Chute do dragão",    dragao,   65, rate=1.15, reaction=R.KNOCKBACK, knockback=900, launch=250, stun=1.0, reach=180, radius=70, breaks_guard=True),
]
sprint = attack("Investida", investida, 45, rate=1.15, reaction=R.KNOCKBACK, knockback=700, stun=0.8, reach=160)
air = attack("Chute aéreo", aereo, 40, rate=1.1, reaction=R.KNOCKBACK, knockback=500, stun=0.7, reach=170, radius=70)


# --- Personagem e boneco de treino --------------------------------------------------------------------------------------
def setup(cdo, is_player):
    cdo.modify()
    m = cdo.get_editor_property("character_movement")
    # Andando e trotando o corpo fica de frente para a câmera (câmera sempre nas costas; A/D = passo lateral); parado a câmera é livre
    m.set_editor_property("walk_faces_camera", True)
    m.set_editor_property("strafe_max_angle", 180.0)
    m.set_editor_property("run_strafe_speed", RUN_STRAFE_SPEED)
    m.set_editor_property("run_back_speed", BACK_SPEED)
    m.set_editor_property("focus_back_speed", 110.0)
    m.set_editor_property("max_walk_speed", WALK_SPEED)
    m.set_editor_property("focus_speed", WALK_SPEED)
    m.set_editor_property("run_speed", RUN_SPEED)
    m.set_editor_property("sprint_speed", SPRINT_SPEED)
    # Dash: velocidade constante (as animações do pack não andam sozinhas); a montagem acompanha o tempo do dash
    m.set_editor_property("dash_root_motion_anim", None)
    m.set_editor_property("dash_speed", 1400.0)
    m.set_editor_property("dash_duration", 0.5)
    m.set_editor_property("dash_anim_rate", 1.0)   # a montagem segue o dash pelas seções (voo repete, "End" no fim)
    # Velocidade da animação acompanha os pés: (velocidade do jogo, velocidade natural da animação)
    cdo.set_editor_property("anim_speed_curve", [unreal.Vector2D(WALK_SPEED, 75.0), unreal.Vector2D(RUN_SPEED, 560.0), unreal.Vector2D(SPRINT_SPEED, 491.0)])
    cdo.set_editor_property("run_anim_min_speed", 60.0)
    cdo.set_editor_property("max_run_anim_rate", 2.6)
    cdo.set_editor_property("run_anim_natural_speed", 491.0)
    cdo.set_editor_property("run_strafe_left_natural", 428.0)    # Mixamo Run2_Left / Right
    cdo.set_editor_property("run_strafe_right_natural", 342.0)
    cdo.set_editor_property("walk_strafe_natural", 164.0)        # Mixamo Walk2_Left / Right
    cdo.set_editor_property("walk_back_natural", 107.0)          # Mixamo Walk2_Backward
    cdo.set_editor_property("strafe_run_min_speed", 250.0)
    if is_player:
        # Câmera mais alta (vê melhor à frente): braço 3,6 m, 1,2 m acima, olhando um pouco para baixo
        boom = cdo.get_editor_property("camera_boom")
        boom.set_editor_property("target_arm_length", 360.0)
        boom.set_editor_property("socket_offset", unreal.Vector(0.0, 0.0, 120.0))
        cam = cdo.get_editor_property("follow_camera")
        cam.set_editor_property("relative_rotation", unreal.Rotator(pitch=-12.0, yaw=0.0, roll=0.0))
        # Corrida ninja: A/D fazem curva aberta e a câmera volta logo para as costas
        cdo.set_editor_property("sprint_steer_angle", 30.0)   # curva com A/D ≈ 280°/s medido (18 = 150°/s lento; 50 = 500°/s, gira no lugar)
        cdo.set_editor_property("sprint_rotation_rate", 1000.0)
        cdo.set_editor_property("sprint_camera_follow_speed", 8.0)
        cdo.set_editor_property("camera_follow_speed", 4.0)
    cdo.set_editor_property("dash_montage", dash)
    cdo.set_editor_property("double_jump_montage", double_jump)
    cdo.set_editor_property("wall_run_montage", wall_run)
    cdo.set_editor_property("wall_jump_montage", wall_jump)
    cdo.set_editor_property("charge_chakra_montage", charge)
    cdo.set_editor_property("turn_right_montage", None)
    cdo.set_editor_property("turn180_montage", None)
    cdo.set_editor_property("idle_break_montage", None)
    cdo.set_editor_property("hit_react_montages", reacts)
    cdo.set_editor_property("heavy_hit_react_montage", heavy_react)
    cdo.set_editor_property("knockout_montage", knockout)
    cdo.set_editor_property("light_combo", light)
    cdo.set_editor_property("heavy_attacks", heavy)
    cdo.set_editor_property("sprint_attack", sprint)
    cdo.set_editor_property("air_attack", air)


for path, is_player in (("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", True), ("/Game/Ninja/Characters/BP_NinjaTrainingDummy", False)):
    bp = unreal.load_asset(path)
    setup(unreal.get_default_object(bp.generated_class()), is_player)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    save.append(bp.get_outermost())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
cdo = unreal.get_default_object(unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
for p in ("dash_montage", "double_jump_montage", "wall_run_montage", "wall_jump_montage", "charge_chakra_montage", "heavy_hit_react_montage", "knockout_montage"):
    print(p, "=", cdo.get_editor_property(p).get_name())
print("leves:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("light_combo")])
print("fortes:", [str(a.get_editor_property("name")) for a in cdo.get_editor_property("heavy_attacks")])
print("reações:", [x.get_name() for x in cdo.get_editor_property("hit_react_montages")])
