"""Teste das animações do pack Anime Ninja no Play: roteiro quadro a quadro com fotos (comando Shot) de cada movimento.
Guarda o registro em unreal._teste_anap['log'] (ler depois). Câmera de lado para ver o corpo.
"""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
dummy = sorted(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaTrainingDummy), key=lambda a: a.get_actor_label())[0]
st = {"t": 0.0, "log": [], "done": set(), "shots": []}
unreal._teste_anap = st
START = unreal.Vector(-600, -2000, 100)


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def montage():
    ai = c.mesh.get_anim_instance()
    mt = ai.get_current_active_montage() if ai else None
    return mt.get_name() if mt else "-"


def shot(name):
    unreal.SystemLibrary.execute_console_command(w, "Shot")
    st["shots"].append(name)
    log("FOTO %s | montagem %s | vel %.0f | modo %s | taxa anim %.2f" % (name, montage(), c.get_velocity().length(), m.movement_mode, c.mesh.get_editor_property("global_anim_rate_scale")))


def once(key, t, fn):
    if st["t"] >= t and key not in st["done"]:
        st["done"].add(key)
        fn()


def side_cam(yaw=90.0):
    pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=yaw, roll=0.0))


def place(loc, yaw=0.0):
    c.set_actor_location_and_rotation(loc, unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False, True)
    m.set_editor_property("velocity", unreal.Vector(0, 0, 0))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("ini", 0.0, lambda: (place(START), side_cam()))
        once("idle", 1.0, lambda: shot("parado"))
        if 1.2 <= t < 6.2:
            c.add_movement_input(unreal.Vector(1, 0, 0), 1.0, False)
            side_cam()
        once("walk", 2.6, lambda: shot("andando"))
        once("run", 2.8, lambda: c.do_sprint_start())
        once("runshot", 4.2, lambda: shot("correndo"))
        once("sp1", 4.35, lambda: c.do_sprint_end())
        once("sp2", 4.4, lambda: c.do_sprint_start())
        once("sp3", 4.45, lambda: c.do_sprint_end())
        once("sp4", 4.5, lambda: c.do_sprint_start())
        once("sprintshot", 5.9, lambda: shot("corrida_ninja"))
        once("stop", 6.2, lambda: (c.do_sprint_end(), place(START), side_cam()))
        once("dash", 7.0, lambda: c.do_dash())
        once("dashshot", 7.18, lambda: shot("dash"))
        once("dashend", 7.6, lambda: log("dash andou %.0f cm" % (c.get_actor_location().x - START.x)))
        once("jre", 7.8, lambda: (place(START), side_cam()))
        once("j1", 8.3, lambda: c.do_jump_start())
        once("j1e", 8.4, lambda: c.do_jump_end())
        once("j1s", 8.5, lambda: shot("pulo"))
        once("j2", 8.7, lambda: c.do_jump_start())
        once("j2e", 8.8, lambda: c.do_jump_end())
        once("j2s", 8.95, lambda: shot("pulo_duplo_a"))
        once("j2s2", 9.25, lambda: shot("pulo_duplo_b"))
        once("cre", 10.3, lambda: (place(START), side_cam()))
        once("ch", 10.6, lambda: c.do_charge_pressed())
        once("chs", 11.6, lambda: shot("carregar_chakra"))
        once("chr", 12.0, lambda: c.do_charge_released())
        def go_dummy():
            d = dummy.get_actor_location()
            fwd = dummy.get_actor_forward_vector()
            place(unreal.Vector(d.x + fwd.x * 130, d.y + fwd.y * 130, d.z), (dummy.get_actor_rotation().yaw + 180.0))
            side_cam(dummy.get_actor_rotation().yaw + 90.0)
            st["hp0"] = dummy.get_health()
            log("boneco %s vida %.0f" % (dummy.get_actor_label(), st["hp0"]))
        once("dum", 12.6, go_dummy)
        def aim_and_hit():
            to = dummy.get_actor_location() - c.get_actor_location()
            pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=unreal.MathLibrary.make_rot_from_x(to).yaw, roll=0.0))
            c.do_attack_pressed()
            log("ataque apertado | vida do boneco %.0f" % dummy.get_health())
        for i, tt in enumerate([13.2, 13.65, 14.15, 14.7]):
            once("a%d" % i, tt, aim_and_hit)
            once("c%d" % i, tt + 0.05, lambda: side_cam(dummy.get_actor_rotation().yaw + 90.0))
        once("as0", 13.45, lambda: shot("golpe1_jab"))
        once("as1", 13.95, lambda: shot("golpe2_direto"))
        once("as2", 14.55, lambda: shot("golpe3_gancho"))
        once("as3", 15.05, lambda: shot("golpe4_chute"))
        once("rs", 15.5, lambda: (shot("boneco_reacao"), log("vida do boneco logo após %.0f" % dummy.get_health())))
        once("fim", 17.0, lambda: (log("vida do boneco %.0f -> %.0f" % (st["hp0"], dummy.get_health())), unreal.unregister_slate_post_tick_callback(st["h"]), st.update(ok=True)))
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
