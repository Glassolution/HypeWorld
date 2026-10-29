"""Teste dos controles no Play pelo Enhanced Input (como apertar a tecla): Shift alterna trote/corrida ninja/andar,
botão direito liga/desliga o modo de batalha, dash. Registro em unreal._teste_ctl['log']."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {"IA_Sprint": c.get_editor_property("sprint_action"), "IA_Focus": c.get_editor_property("focus_action"), "IA_Dash": c.get_editor_property("dash_action")}
st = {"t": 0.0, "log": [], "done": set()}
unreal._teste_ctl = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def press(name):
    sub.inject_input_vector_for_action(IA[name], unreal.Vector(1, 0, 0), [], [])
    log("apertou %s (%s)" % (name, IA[name].get_name() if IA[name] else None))


def state(label):
    log("%s | vel %.0f | trote=%s ninja=%s | batalha=%s | dash=%s" % (label, c.get_velocity().size2d() if hasattr(c.get_velocity(), "size2d") else c.get_velocity().length(),
        m.is_running(), m.is_sprinting(), c.is_in_battle_mode(), m.is_dashing()))


def once(key, t, fn):
    if st["t"] >= t and key not in st["done"]:
        st["done"].add(key)
        fn()


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("ini", 0.0, lambda: (c.set_actor_location(unreal.Vector(-600, -2000, 100), False, True), pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=90.0, roll=0.0))))
        moving = 0.5 <= t < 5.0 or 6.0 <= t < 7.0
        if moving:
            c.add_movement_input(unreal.Vector(1, 0, 0), 1.0, False)
        once("w", 1.2, lambda: state("andando"))
        once("s1", 1.3, lambda: press("IA_Sprint"))
        once("w1", 2.6, lambda: (state("1 clique Shift"), unreal.SystemLibrary.execute_console_command(w, "Shot")))
        once("s2", 2.7, lambda: press("IA_Sprint"))
        once("w2", 4.4, lambda: (state("2 cliques Shift"), unreal.SystemLibrary.execute_console_command(w, "Shot")))
        once("w3", 5.8, lambda: state("parou 0,8 s"))
        once("w4", 6.9, lambda: state("andou de novo"))
        once("f1", 7.5, lambda: press("IA_Focus"))
        once("b1", 7.8, lambda: state("clique botão direito"))
        once("b2", 16.0, lambda: (state("8 s depois"), unreal.SystemLibrary.execute_console_command(w, "Shot")))
        once("f2", 16.2, lambda: press("IA_Focus"))
        once("b3", 16.6, lambda: state("2º clique botão direito"))
        once("d1", 17.0, lambda: press("IA_Dash"))
        once("d2", 17.1, lambda: state("depois do dash"))
        once("fim", 17.5, lambda: (unreal.unregister_slate_post_tick_callback(st["h"]), st.update(ok=True)))
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
