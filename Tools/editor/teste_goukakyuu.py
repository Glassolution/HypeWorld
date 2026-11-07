"""Teste visual do Gōkakyū (saída → voo → impacto) no Play.
Câmera do jogo (padrão) ou alta atrás do personagem (unreal._teste_gk_camera = "cima"), câmera lenta durante o jutsu e fotos (comando Shot) em
instantes fixos contados a partir de quando o projétil nasce. Fotos em Saved/Screenshots/WindowsEditor.
Rodar com o jogo em Play. Registro em unreal._teste_gk['log'].
Ajustes antes de rodar (opcionais): unreal._teste_gk_slomo (0,3), unreal._teste_gk_tempos (lista em s de jogo).
"""
import unreal

SLOMO = getattr(unreal, "_teste_gk_slomo", 0.3)
TEMPOS = getattr(unreal, "_teste_gk_tempos", [0.0, 0.03, 0.06, 0.1, 0.15, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 0.9, 1.1])
# "jogador" (padrão, 06/11: o Felipe avalia pela câmera do jogo, atrás do personagem) ou "cima" (câmera alta)
CAMERA = getattr(unreal, "_teste_gk_camera", "jogador")
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
st = {"t": 0.0, "log": [], "done": set(), "t0": None, "shots": 0}
unreal._teste_gk = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def slot_index():
    loadout = c.get_editor_property("jutsu_loadout")
    for i, j in enumerate(loadout):
        if j and j.get_name() == "DA_Katon_Goukakyuu":
            return i
    return -1


def camera():
    rot = pc.get_control_rotation()
    fwd = unreal.MathLibrary.get_forward_vector(unreal.Rotator(roll=0, pitch=0, yaw=rot.yaw))
    base = c.get_actor_location()
    pos = base - fwd * 450 + unreal.Vector(0, 0, 900)
    look = base + fwd * 650
    cam = unreal.NinjaEditorTools.spawn_local_actor(w, unreal.CameraActor, pos, unreal.MathLibrary.find_look_at_rotation(pos, look))
    cam.camera_component.set_editor_property("field_of_view", 75.0)
    cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
    pc.set_view_target_with_blend(cam, 0.0)
    log("câmera em %s" % pos)


def cast():
    # a mira segue o ponto de vista da câmera do jogador: lança com ela nivelada; a câmera alta só entra depois
    rot = pc.get_control_rotation()
    pc.set_control_rotation(unreal.Rotator(roll=0, pitch=-4, yaw=rot.yaw))
    i = slot_index()
    unreal.SystemLibrary.execute_console_command(w, "slomo %s" % SLOMO)
    log("slot %d -> %s (mira yaw %.0f)" % (i, c.try_use_jutsu_slot(i), rot.yaw))


def projectile():
    for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile):
        if a.get_actor_location().z > -10000:   # ignora a cópia de pré-carga, 200 m abaixo
            return a
    return None


def luzes():
    """Luzes pontuais acesas a menos de 25 m do personagem: dono/componente, intensidade e distância (m)"""
    base = c.get_actor_location()
    out = []
    for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
        for l in a.get_components_by_class(unreal.PointLightComponent):
            if l.is_visible() and l.intensity > 0:
                d = (l.get_world_location() - base).length() / 100.0
                if d < 25:
                    out.append("%s.%s=%.0f@%.1fm" % (a.get_name()[:24], l.get_name()[:20], l.intensity, d))
    return (" | luzes: " + ", ".join(out)) if out else ""


def tick(dt):
    try:
        st["t"] += dt
        once("cast", 0.5, cast)
        now = unreal.GameplayStatics.get_time_seconds(w)
        if st["t0"] is None and st["t"] > 0.5:
            if projectile():
                st["t0"] = now
                if CAMERA == "cima":
                    camera()
                log("projétil nasceu")
        if st["t0"] is not None:
            age = now - st["t0"]
            # uma foto por quadro no máximo (duas no mesmo quadro chegaram a fotografar o viewport do editor)
            if st["shots"] < len(TEMPOS) and age >= TEMPOS[st["shots"]]:
                unreal.SystemLibrary.execute_console_command(w, "Shot")
                log("foto %d em %.3f s de jogo%s" % (st["shots"], age, luzes()))
                st["shots"] += 1
                while st["shots"] < len(TEMPOS) and age >= TEMPOS[st["shots"]]:
                    st["shots"] += 1   # instante já passou neste quadro: pula
        if st["shots"] >= len(TEMPOS) or st["t"] > 20.0:
            unreal.SystemLibrary.execute_console_command(w, "slomo 1")
            log("fim")
            unreal.unregister_slate_post_tick_callback(st["h"])
    except Exception as e:
        log("ERRO %r" % e)
        unreal.SystemLibrary.execute_console_command(w, "slomo 1")
        unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
