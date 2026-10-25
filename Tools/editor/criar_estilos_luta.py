"""Estilos de luta (poses de batalha do menu de combate), combos variados e dashes por direção, com o pack Anime Ninja.
- 3 estilos de mão livre, cada um com 3 combos (o jogo sorteia um diferente a cada sequência nova), finalizadores (F),
  golpes correndo e no ar sorteados. 4 estilos com arma (Katana, Garras, Lança, Kunai) trancados até existir arma.
- Tempos de acerto: medidos por Tools/editor/analisar_golpes_pack (pico de alcance da mão/pé à frente).
- Atordoamento de cada golpe calculado para o combo encaixar mesmo com a queda por golpe seguido (HitstunDecay 0,9).
- Dashes: frente (2 variações), trás, esquerda/direita (passo lateral e desvio correndo), no ar.
Rodar com ue_exec -f, editor fora do Play. Saída: /Game/Ninja/Characters/Animations/ANAP/Estilos e .../Dash
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
ATK = "/Game/ANAP/Animations/Attack/HandBlades/HandBlades_Anim_Armature_CustomMan_Attack_"
BASE = "/Game/ANAP/Animations/Base/Base_Anim_Armature_CustomMan_"
POSE = "/Game/ANAP/Animations/Pose/"
OUT_GOLPES = "/Game/Ninja/Characters/Animations/ANAP/Golpes"
OUT_DASH = "/Game/Ninja/Characters/Animations/ANAP/Dash"
OUT_ESTILOS = "/Game/Ninja/Characters/Estilos"
R = unreal.NinjaHitReaction
DECAY = 0.9
save = []


def seq(path):
    a = unreal.load_asset(path)
    assert a, path
    return a


def montage_asset(folder, name, first):
    path = "%s/%s" % (folder, name)
    m = unreal.load_asset(path)
    if not m:
        f = unreal.AnimMontageFactory()
        f.set_editor_property("source_animation", first)
        f.set_editor_property("target_skeleton", first.get_editor_property("skeleton"))
        m = tools.create_asset(name, folder, unreal.AnimMontage, f)
    save.append(m.get_outermost())
    return m


class Golpe:
    """Um golpe do pack: acertos (s, relógio da animação) e de onde ele começa (para o blend in)."""
    def __init__(self, nome, acertos, inicio_longe=False):
        self.nome, self.acertos, self.inicio_longe = nome, acertos, inicio_longe
        self.anim = seq(ATK + nome)
        self.L = self.anim.get_play_length()

    def montagem(self, combo_aberto):
        last = self.acertos[-1]
        end = min(self.L, last + 0.75)
        combo = last + 0.1 if combo_aberto else -1.0
        end = max(end, (combo if combo > 0 else last) + 0.32)
        end = min(end, self.L)
        # golpe no meio de combo (com janela) e golpe solto (sem janela) são montagens separadas
        m = montage_asset(OUT_GOLPES, "AM_G_" + self.nome + ("_Combo" if combo_aberto else ""), self.anim)
        ok = unreal.NinjaEditorTools.build_attack_montage(m, self.anim, 0.0, end, list(self.acertos), combo, 0.18 if self.inicio_longe else 0.1, 0.25)
        assert ok, self.nome
        return m, end, combo


def ataque(nome, montage, dano, rate, stun, reaction=R.LIGHT, knockback=200.0, launch=0.0, reach=150.0, radius=60.0, breaks_guard=False):
    a = unreal.NinjaAttack()
    for k, v in dict(name=nome, montage=montage, damage=dano, play_rate=rate, reaction=reaction, knockback=knockback, launch=launch,
                     stun=stun, reach=reach, radius=radius, breaks_guard=breaks_guard, root_motion_scale=1.0).items():
        a.set_editor_property(k, v)
    return a


def combo(nome, golpes, dano=26.0, rate=1.35, fim_reacao=R.KNOCKBACK, fim_knockback=650.0, fim_launch=200.0):
    """Sequência de golpes: o último arremessa; o atordoamento de cada um cobre o tempo até o próximo acerto."""
    c = unreal.NinjaComboString()
    c.set_editor_property("name", unreal.Text(nome))
    attacks = []
    for i, g in enumerate(golpes):
        last = i == len(golpes) - 1
        m, end, combo_t = g.montagem(not last)
        if last:
            stun = 0.9
            attacks.append(ataque(g.nome, m, dano + 6, rate, stun, fim_reacao, fim_knockback, fim_launch, reach=165, radius=65))
        else:
            nxt = golpes[i + 1]
            gap = (combo_t - g.acertos[-1]) / rate + nxt.acertos[0] / rate
            stun = min(1.6, max(0.6, (gap + 0.22) / (DECAY ** i)))
            attacks.append(ataque(g.nome, m, dano, rate, stun))
    c.set_editor_property("attacks", attacks)
    return c


def soltos(golpes, dano, rate, stun, reaction, knockback, launch, breaks_guard=False):
    out = []
    for g in golpes:
        m, _, _ = g.montagem(False)
        out.append(ataque(g.nome, m, dano, rate, stun, reaction, knockback, launch, reach=170, radius=70, breaks_guard=breaks_guard))
    return out


def estilo(asset_name, nome, descricao, cor, idle, combos=(), fins=(), corrida=(), ar=(), lanc=(), arma=unreal.NinjaWeapon.NONE):
    s = unreal.load_asset("%s/%s" % (OUT_ESTILOS, asset_name))
    if not s:
        f = unreal.DataAssetFactory()
        f.set_editor_property("data_asset_class", unreal.NinjaFightingStyle)
        s = tools.create_asset(asset_name, OUT_ESTILOS, unreal.NinjaFightingStyle, f)
    s.set_editor_property("display_name", unreal.Text(nome))
    s.set_editor_property("description", unreal.Text(descricao))
    s.set_editor_property("icon_color", cor)
    s.set_editor_property("required_weapon", arma)
    s.set_editor_property("battle_idle", seq(idle))
    s.set_editor_property("combos", list(combos))
    s.set_editor_property("finishers", list(fins))
    s.set_editor_property("sprint_attacks", list(corrida))
    s.set_editor_property("air_attacks", list(ar))
    s.set_editor_property("launchers", list(lanc))
    save.append(s.get_outermost())
    print("estilo %-18s combos %d | fortes %d | correndo %d | aéreo %d | lançador %d | arma %s" % (nome, len(combos), len(fins), len(corrida), len(ar), len(lanc), arma))
    return s


G = Golpe
FORTE = dict(dano=60, rate=1.25, stun=1.0, reaction=R.KNOCKBACK, knockback=800, launch=250, breaks_guard=True)
CORRIDA = dict(dano=45, rate=1.3, stun=0.8, reaction=R.KNOCKBACK, knockback=700, launch=0)
AR = dict(dano=40, rate=1.3, stun=0.75, reaction=R.KNOCKBACK, knockback=500, launch=0)


def aereo(golpes, rate=1.3):
    """Combo aéreo em ordem: cada golpe deixa os dois suspensos no ar (o código cuida); o último (KNOCKBACK) arremessa para longe."""
    out = []
    for i, g in enumerate(golpes):
        last = i == len(golpes) - 1
        m, _, _ = g.montagem(not last)
        if last:
            out.append(ataque(g.nome, m, 38, rate, 1.0, R.KNOCKBACK, 1500.0, 450.0, reach=175, radius=75))
        else:
            out.append(ataque(g.nome, m, 28, rate, 0.95, R.LIGHT, 120.0, 0.0, reach=170, radius=75))
    return out


def lancador(golpes, rate=1.3):
    """Lançador (Espaço no meio do combo): joga para cima; tem janela de combo para o primeiro golpe aéreo."""
    out = []
    for g in golpes:
        m, _, _ = g.montagem(True)
        out.append(ataque(g.nome, m, 30, rate, 1.3, R.LAUNCH, 100.0, 1150.0, reach=160, radius=65, breaks_guard=True))   # abre a defesa e lança
    return out

# ---------------- Punho de Chakra (base de luta Idle_Loop) ----------------
punho = estilo("DA_Estilo_PunhoDeChakra", "Punho de Chakra", "Socos e palmas carregados de chakra", unreal.LinearColor(0.95, 0.42, 0.18, 1.0), BASE + "Idle_Loop",
    combos=[
        combo("Rajada", [G("ChakraFistA_cmb_00", [0.30]), G("ChakraFist_B_StraightPunch", [0.43]), G("ChakraFist_B_Palm", [0.30, 0.57]), G("ChakraFist_B_DoubleTurnKick_Large", [0.17, 0.43])]),
        combo("Palma explosiva", [G("ChakraFist_B_Palm", [0.30, 0.57]), G("ChakraFistA_cmb_00", [0.30]), G("ChakraFistA_Palm_Large", [0.77]), G("ChakraFistA_BlastPunch", [0.53], True)]),
        combo("Dragão", [G("ChakraFist_B_StraightPunch", [0.43]), G("ChakraFistA_cmb_14", [0.30], True), G("ChakraFistA_cmb08", [0.90], True), G("ChakraFistA_DragonKick", [0.71], True)]),
    ],
    fins=soltos([G("ChakraFistA_StraightPunch", [0.80, 1.33]), G("ChakraFistA_DragonKick", [0.71], True), G("ChakraFistA_BlastPunch", [0.53], True), G("ChakraFist_B_QigongGun", [0.43, 1.39], True)], **FORTE),
    corrida=soltos([G("Hand_Punch_Left", [0.17]), G("ChakraFistA_DragonKick", [0.71], True)], **CORRIDA),
    ar=aereo([G("Aerial_ChakraFist_B_Palm", [0.24, 0.58]), G("Aerial_cmb04", [0.20], True), G("Aerial_ChakraFist_B_TurnKick", [0.70], True)]),
    lanc=lancador([G("BeniHien_Uppercut", [0.37], True)]))

# ---------------- Taijutsu Ninja (base Hand_Idle_Loop) ----------------
taijutsu = estilo("DA_Estilo_TaijutsuNinja", "Taijutsu Ninja", "Golpes rápidos, garras de chakra e chutes", unreal.LinearColor(0.2, 0.75, 0.7, 1.0), BASE + "Hand_Idle_Loop",
    combos=[
        combo("Corrente", [G("cmb07", [0.13, 0.53]), G("cmb05", [0.37], True), G("cmb09", [0.50], True), G("Hand_JackCut", [0.76], True)]),
        combo("Garra de chakra", [G("Hand_ChakraClawsB_cmb_12", [0.33], True), G("Hand_ChakraClawsB_cmb_01", [0.56], True), G("Hand_Short_ChakraClawsA", [0.17, 0.43, 0.73], True), G("KickUp", [0.86, 1.49], True)],
              fim_reacao=R.LAUNCH, fim_knockback=300.0, fim_launch=600.0),
        combo("Vento", [G("Hand_Punch_Left", [0.17]), G("cmb05", [0.37], True), G("Hand_SidePalm", [0.86]), G("Hand_BackKick", [0.86, 1.49], True)]),
    ],
    fins=soltos([G("Hand_DragonKick", [1.43], True), G("Hand_WindMill", [0.73, 0.96, 1.62, 1.85], True), G("Hand_StraightPunch", [0.80, 1.33], True), G("Hand_MantisFist", [0.79, 1.29, 1.72], True)], **FORTE),
    corrida=soltos([G("JumpTurnKick_Right", [0.50], True), G("Hand_ChakraClawsB_cmb_01", [0.56], True)], **CORRIDA),
    ar=aereo([G("Aerial_Hand_Punch", [0.47, 1.17]), G("Aerial_Hand_TurnKick", [0.76], True), G("Aerial_Hand_TurnHeelDropKick", [0.90], True)]),
    lanc=lancador([G("KickUp", [0.86], True)]))

# ---------------- Punho Blindado (soqueiras e luvas de chakra; base Hand_Idle_Loop) ----------------
blindado = estilo("DA_Estilo_PunhoBlindado", "Punho Blindado", "Soqueiras e luvas de chakra: golpes pesados", unreal.LinearColor(0.62, 0.38, 0.95, 1.0), BASE + "Hand_Idle_Loop",
    combos=[
        combo("Soqueira", [G("CyakraKnuckley_Cmb01", [0.57]), G("CyakraKnuckley_Cmb02", [0.47, 0.77], True), G("CyakraKnuckley_Cmb03", [0.43], True), G("ChakraGloves_B_Uppercut_Large", [0.47])],
              fim_reacao=R.LAUNCH, fim_knockback=300.0, fim_launch=600.0),
        combo("Luvas", [G("ChakraGloves_B_DoublePunch_Uppercut", [0.20, 0.43]), G("ChakraGloves_B_Uppercut", [0.60]), G("CyakraKnuckley_Cmb01", [0.57]), G("ChakraKnuckleB_Cmb02", [0.57, 0.87, 1.10], True)]),
        combo("Martelo", [G("CyakraKnuckley_Cmb02_Rateup", [0.50, 0.73]), G("ChakraKnuckleA_Cmb02", [1.22], True), G("ChakraGloves_B_Uppercut", [0.60]), G("ChakraGloves_B_DoublePunch_Kick", [0.23, 0.83, 1.13])]),
    ],
    fins=soltos([G("ChakraKnuckleA_Cmb01", [0.47, 1.06, 1.83]), G("ChakraKnuckleB_Cmb01", [0.90, 1.54]), G("BeniHien_BlastPunch", [1.10]), G("BeniHien_Uppercut", [0.37], True)], **FORTE),
    corrida=soltos([G("CyakraKnuckley_Cmb05", [0.33], True)], **CORRIDA),
    ar=aereo([G("Aerial_CyakraKnuckley_Cmb01", [0.57], True), G("Aerial_CyakraKnuckley_Cmb03", [0.34], True), G("Aerial_ChakraKnuckleA_Cmb01", [0.63], True)]),
    lanc=lancador([G("ChakraGloves_B_Uppercut_Large", [0.47])]))

# ---------------- Estilos com arma (trancados até existir arma no jogo) ----------------
W = unreal.NinjaWeapon
armados = [
    estilo("DA_Estilo_Katana", "Katana de Chakra", "Cortes rápidos de espada", unreal.LinearColor(0.85, 0.85, 0.9, 1.0), BASE + "BigSword_Idle_Loop", arma=W.KATANA),
    estilo("DA_Estilo_Garras", "Presas Gêmeas", "Garras duplas", unreal.LinearColor(0.9, 0.25, 0.3, 1.0), BASE + "Hand_Idle_Loop", arma=W.CLAWS),
    estilo("DA_Estilo_Lanca", "Lança", "Estocadas de longo alcance", unreal.LinearColor(0.9, 0.75, 0.3, 1.0), BASE + "Idle_Loop", arma=W.SPEAR),
    estilo("DA_Estilo_Kunai", "Kunai", "Facas ninja de curto alcance", unreal.LinearColor(0.5, 0.6, 0.7, 1.0), BASE + "Kunai_Idle_Loop", arma=W.KUNAI),
]


# ---------------- Dashes por direção ----------------
# A seção End (aterrissagem) vai até o fim da animação, quando ele já se levantou: terminar agachado e "puxar" para a pose
# parada parecia corte. Se o jogador se mexer na aterrissagem, o código desfaz a montagem suave.
def dash(name, parts, loop=False):
    """parts = [(animação, início, fim, seção)]; seção Loop repete; End = aterrissagem chamada pelo código no fim do dash."""
    anims = [seq(p) for p, _, _, _ in parts]
    m = montage_asset(OUT_DASH, name, anims[0])
    ok = unreal.NinjaEditorTools.build_section_montage(m, anims, [float(a) for _, a, _, _ in parts], [float(b) for _, _, b, _ in parts],
                                                       [s for _, _, _, s in parts], ["Loop"] if loop else [], 0.22, 0.25, False)   # entrada 0,22 s: com 0,06 o corpo pulava para o mergulho
    print("dash %-26s %s %.2fs" % (name, "ok" if ok else "FALHOU", m.get_play_length()))
    return m


VOO = (BASE + "Aerial_Dash_Loop", 0.05, 0.37, "Loop")
frente = [unreal.load_asset("/Game/Ninja/Characters/Animations/ANAP/AM_ANAP_Dash"),
          dash("AM_Dash_Frente_Mao", [(BASE + "Hand_DashStep_Front", 0.0, 0.60, "Default"), VOO, (BASE + "Hand_DashStep_Front", 0.60, -1, "End")], loop=True)]   # voo e aterrissagem emendam em 0,60 (pose de voo parada de 0,3 a 0,6); antes pulava de 0,5 para 0,85 = corte
tras = [dash("AM_Dash_Tras_Mortal", [(BASE + "DashStep_Behind", 0.0, 0.50, "Default"), (BASE + "DashStep_Behind", 0.50, -1, "End")])]   # emenda = fim do dash (0,5 s): passa para o End sem pular quadro (antes 0,78 = corte)
esquerda = [dash("AM_Dash_Esquerda_Passo", [(BASE + "DashStep_Left", 0.0, 0.45, "Default"), (BASE + "DashStep_Left", 0.45, -1, "End")]),
            dash("AM_Dash_Esquerda_Desvio", [(BASE + "Dodge_Run_Left", 0.05, 0.55, "Default"), (BASE + "Dodge_Run_Left", 0.55, -1, "End")])]
direita = [dash("AM_Dash_Direita_Passo", [(BASE + "DashStep_Right", 0.0, 0.45, "Default"), (BASE + "DashStep_Right", 0.45, -1, "End")]),
           dash("AM_Dash_Direita_Desvio", [(BASE + "Dodge_Run_Right", 0.05, 0.55, "Default"), (BASE + "Dodge_Run_Right", 0.55, -1, "End")])]
ar = [dash("AM_Dash_Ar", [(BASE + "Aerial_Dash_Start", 0.42, 1.0, "Default"), VOO, (BASE + "Aerial_Dash_End", 0.45, 1.25, "End")], loop=True)]

# ---------------- Tecla do menu de combate (Tab / botão Menu do controle) ----------------
IA_PATH = "/Game/Ninja/Core/Input/IA_CombatMenu"
ia = unreal.load_asset(IA_PATH) or tools.create_asset("IA_CombatMenu", "/Game/Ninja/Core/Input", unreal.InputAction, unreal.InputAction_Factory())
ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
save.append(ia.get_outermost())
IMC = unreal.load_asset("/Game/Input/IMC_Default")
IMC.modify()
used = {str(m.key.get_editor_property("key_name")): (m.action.get_name() if m.action else None)
        for m in IMC.get_editor_property("default_key_mappings").get_editor_property("mappings")}
for key_name in ("Tab", "Gamepad_Special_Right"):
    if used.get(key_name) not in (None, "IA_CombatMenu"):
        print("AVISO: tecla", key_name, "já usada por", used[key_name])
    elif used.get(key_name) is None:
        k = unreal.Key()
        k.set_editor_property("key_name", key_name)
        IMC.map_key(ia, k)
save.append(IMC.get_outermost())

# ---------------- Personagem ----------------
bp = unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
cdo = unreal.get_default_object(bp.generated_class())
cdo.modify()
cdo.set_editor_property("fighting_styles", [punho, taijutsu, blindado] + armados)
dv = unreal.NinjaDashSet()
for k, v in dict(front=frente, back=tras, left=esquerda, right=direita, air=ar).items():
    dv.set_editor_property(k, v)
cdo.set_editor_property("dash_variants", dv)
cdo.set_editor_property("combat_menu_action", ia)


def seq_montage(name, parts, blend_in=0.08, blend_out=0.25, rate=1.0):
    anims = [seq(p) for p, _, _ in parts]
    m = montage_asset(OUT_DASH.replace("/Dash", "/Aereo"), name, anims[0])
    unreal.NinjaEditorTools.build_sequence_montage(m, anims, [float(a) for _, a, _ in parts], [float(b) for _, _, b in parts], blend_in, blend_out, False, False)
    m.set_editor_property("rate_scale", rate)
    return m


MISC = "/Game/ANAP/Animations/Misc/Misc_Anim_Armature_CustomMan_"
reac_ar = [seq_montage("AM_Reacao_Ar_Leve", [(MISC + "Beaten_Aerial_Small_Conect_toFall", 0, -1)], 0.05, 0.3, 1.2),
           seq_montage("AM_Reacao_Ar_Barriga", [(MISC + "Beaten_Aerial_Stagger_Belly", 0, -1)], 0.05, 0.3, 1.2)]
reac_lancado = seq_montage("AM_Reacao_Lancado", [(MISC + "Beaten_SpinBlowOff_Large_Up", 0, -1)], 0.05, 0.3, 1.0)
mortal_tras = seq_montage("AM_Mortal_Tras", [(BASE + "DashStep_Behind", 0.05, 0.8)], 0.06, 0.3, 0.75)   # mortal de costas no ar (~1 s)
for path in ("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter", "/Game/Ninja/Characters/BP_NinjaTrainingDummy"):
    b = unreal.load_asset(path)
    o = unreal.get_default_object(b.generated_class())
    o.modify()
    o.set_editor_property("air_hit_react_montages", reac_ar)
    o.set_editor_property("launch_react_montage", reac_lancado)
    o.set_editor_property("backflip_montage", mortal_tras)
    o.set_editor_property("max_combo_hits", 10)     # o combo agora tem parte aérea
    o.set_editor_property("max_combo_time", 5.0)
    o.get_editor_property("character_movement").set_editor_property("dash_cooldown", 0.5)   # luta mais acelerada
    unreal.BlueprintEditorLibrary.compile_blueprint(b)
    save.append(b.get_outermost())
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save.append(bp.get_outermost())

print("salvo:", unreal.EditorLoadingAndSavingUtils.save_packages(save, False))
cdo = unreal.get_default_object(unreal.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter").generated_class())
print("estilos:", [str(s.get_editor_property("display_name")) for s in cdo.get_editor_property("fighting_styles")])
d = cdo.get_editor_property("dash_variants")
print("dashes:", {k: [m.get_name() for m in d.get_editor_property(k)] for k in ("front", "back", "left", "right", "air")})
for s in [punho, taijutsu, blindado]:
    for c in s.get_editor_property("combos"):
        print("  %-16s %-18s" % (s.get_editor_property("display_name"), c.get_editor_property("name")),
              [("%s %.2fs" % (a.get_editor_property("name"), a.get_editor_property("stun"))) for a in c.get_editor_property("attacks")])
