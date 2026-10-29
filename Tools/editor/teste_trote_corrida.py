"""Teste do trote (PvP: passos laterais com paradas, não pode voltar a andar) e das curvas da corrida ninja (A/D e voltar com S).
Rodar com o jogo em Play (ue_exec -f). Resultado em unreal._ttc["log"].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
mv = c.get_ninja_movement()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "sprint_action")}
st = {"t": 0.0, "log": [], "done": set(), "last": 0.0, "marks": {}}
unreal._ttc = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def move(x, y):   # x = direita, y = frente (como o teclado: D = (1,0), W = (0,1))
    sub.inject_input_vector_for_action(IA["move_action"], unreal.Vector(x, y, 0), [], [])


def modo():
    return "CORRIDA_NINJA" if mv.is_sprinting() else ("TROTE" if mv.is_running() else "ANDAR")


def estado(tag):
    v = c.get_velocity(); v.z = 0.0
    cam = pc.get_control_rotation().yaw
    vel_yaw = unreal.MathLibrary.make_rot_from_x(v).yaw if v.length() > 10 else 0.0
    log("%s %s vel %.0f | corpo %.0f° vel %.0f° câmera %.0f° | anim x%.2f" % (tag, modo(), v.length(), c.get_actor_rotation().yaw, vel_yaw, cam, c.mesh.get_editor_property("global_anim_rate_scale")))


# Roteiro: (início, fim, x, y)
TROTE = [(1.0, 1.6, 0, 1), (1.6, 2.1, -1, 0), (2.6, 3.1, 1, 0), (3.6, 4.1, -1, 0), (4.6, 5.1, 1, 0)]       # W, A, pausa, D, pausa, A, pausa, D
CORRIDA = [(6.5, 7.7, 0, 1), (7.7, 8.7, -1, 0), (8.7, 9.3, 0, 1), (9.3, 10.3, 0, -1), (10.3, 10.8, 0, 1)]   # W, só A (curva), W, S (voltar), W


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("s", 0.3, lambda: (c.set_actor_location_and_rotation(unreal.Vector(-1500, -3500, 100), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0), False, True),
                                pc.set_control_rotation(unreal.Rotator(pitch=-10.0, yaw=0.0, roll=0.0))))
        once("shift1", 0.6, lambda: (sub.inject_input_vector_for_action(IA["sprint_action"], unreal.Vector(1, 0, 0), [], []), log("Shift 1x (trote)")))
        for a, b, x, y in TROTE + CORRIDA:
            if a <= t < b:
                move(x, y)
        once("shift2", 6.3, lambda: (sub.inject_input_vector_for_action(IA["sprint_action"], unreal.Vector(1, 0, 0), [], []), log("Shift 2x (corrida ninja)")))
        if 0.9 <= t < 11.2 and t - st["last"] >= (0.25 if t < 6.3 else 0.12):
            st["last"] = t
            estado("")
        for i, tt in enumerate([1.45, 1.95, 3.0, 8.1]):   # trote de frente, de lado (A), de lado (D), curva da corrida ninja
            once("f%d" % i, tt, lambda: unreal.SystemLibrary.execute_console_command(w, "Shot"))
        if t > 11.3:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
