"""Teste da câmera nas costas e do passo lateral (teclas simuladas). Registro em unreal._teste_cam['log']."""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "sprint_action", "look_action", "mouse_look_action")}
st = {"t": 0.0, "log": [], "done": set()}
unreal._teste_cam = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def estado(label, shot=True):
    v = c.get_velocity()
    import math
    vyaw = math.degrees(math.atan2(v.y, v.x)) if v.length() > 5 else float("nan")
    if shot:
        unreal.SystemLibrary.execute_console_command(w, "Shot showui")
    log("%s | corpo yaw %.0f | câmera yaw %.0f | movimento yaw %.0f | vel %.0f | taxa anim %.2f" % (label, c.get_actor_rotation().yaw, pc.get_control_rotation().yaw, vyaw, v.length(), c.mesh.get_editor_property("global_anim_rate_scale")))


def move(x, y):
    sub.inject_input_vector_for_action(IA["move_action"], unreal.Vector(x, y, 0), [], [])


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("p", 0.2, lambda: (c.set_actor_location_and_rotation(unreal.Vector(-1500, -3500, 100), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0), False, True), pc.set_control_rotation(unreal.Rotator(pitch=-10.0, yaw=0.0, roll=0.0))))
        once("e0", 0.8, lambda: estado("parado"))
        if 1.0 <= t < 2.6: move(1, 0)           # D andando
        once("e1", 2.4, lambda: estado("D andando"))
        if 3.0 <= t < 4.4: move(0, -1)          # S andando
        once("e2", 4.2, lambda: estado("S andando"))
        once("r", 4.6, lambda: sub.inject_input_vector_for_action(IA["sprint_action"], unreal.Vector(1, 0, 0), [], []))
        if 4.7 <= t < 6.4: move(1, 0)           # D trotando
        once("e3", 6.2, lambda: estado("D trotando"))
        once("r2", 6.5, lambda: sub.inject_input_vector_for_action(IA["sprint_action"], unreal.Vector(1, 0, 0), [], []))
        if 6.6 <= t < 9.6: move(1, 0.3)         # corrida ninja com D (+um pouco de W)
        once("e4", 7.6, lambda: estado("corrida ninja D 1s"))
        once("e5", 9.4, lambda: estado("corrida ninja D 3s"))
        # parado: girar a câmera (mouse) não gira o corpo
        if 10.5 <= t < 11.5:
            sub.inject_input_vector_for_action(IA["mouse_look_action"], unreal.Vector(8, 0, 0), [], [])
        once("e6", 11.7, lambda: estado("parado girando a câmera"))
        if t > 12.0:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
