"""Teste do modo de batalha e das pernas no Play (fotos com Shot). Registro em unreal._teste_bat['log']."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
dummy = sorted(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaTrainingDummy), key=lambda a: a.get_actor_label())[0]
st = {"t": 0.0, "log": [], "done": set()}
unreal._teste_bat = st
START = unreal.Vector(-600, -2000, 100)


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def shot(name):
    unreal.SystemLibrary.execute_console_command(w, "Shot")
    log("FOTO %s | batalha=%s | vel %.0f" % (name, c.is_in_battle_mode(), c.get_velocity().length()))


def once(key, t, fn):
    if st["t"] >= t and key not in st["done"]:
        st["done"].add(key)
        fn()


def cam(yaw):
    pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=yaw, roll=0.0))


def place(loc, yaw=0.0):
    c.set_actor_location_and_rotation(loc, unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False, True)
    m.set_editor_property("velocity", unreal.Vector(0, 0, 0))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("ini", 0.0, lambda: (place(START, 0.0), cam(90.0)))
        once("s1", 1.5, lambda: shot("parado_normal"))
        once("f1", 1.7, lambda: (c.do_focus_start(), cam(90.0)))
        once("s2", 2.8, lambda: shot("botao_direito_luta"))
        once("f2", 3.0, lambda: c.do_focus_end())
        once("s3", 3.2, lambda: log("soltou: batalha=%s" % c.is_in_battle_mode()))
        once("s3b", 4.5, lambda: shot("soltou_botao"))
        if 5.0 <= t < 6.6:
            c.add_movement_input(unreal.Vector(1, 0, 0), 1.0, False)
            cam(90.0)
        once("s4", 6.3, lambda: shot("andando"))
        once("r0", 6.6, lambda: c.do_sprint_start())
        if 6.6 <= t < 8.0:
            c.add_movement_input(unreal.Vector(1, 0, 0), 1.0, False)
            cam(90.0)
        once("s5", 7.8, lambda: shot("correndo"))
        once("r1", 8.0, lambda: c.do_sprint_end())

        def go_dummy():
            d = dummy.get_actor_location()
            fwd = dummy.get_actor_forward_vector()
            place(unreal.Vector(d.x + fwd.x * 130, d.y + fwd.y * 130, d.z), dummy.get_actor_rotation().yaw + 180.0)
            cam(dummy.get_actor_rotation().yaw + 180.0)
        once("d", 8.5, go_dummy)
        once("a", 9.0, lambda: c.do_attack_pressed())
        once("cs", 9.05, lambda: cam(dummy.get_actor_rotation().yaw + 90.0))
        once("s6", 11.0, lambda: shot("depois_de_bater"))
        once("s7", 17.5, lambda: shot("6s_depois"))
        once("fim", 17.6, lambda: (unreal.unregister_slate_post_tick_callback(st["h"]), st.update(ok=True)))
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
