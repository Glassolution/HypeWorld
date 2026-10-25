"""Teste do mortal para trás (modo de batalha + S + Espaço): o corpo tem que continuar de frente para onde a câmera olha
durante o voo e depois de pousar (segurando S o tempo todo, como o Felipe joga). Depois, fora da batalha, o super pulo
carregado. Os dois têm voo planado (sobe, flutua para a frente e desce devagar). Resultado em unreal._tmo["log"].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
mv = c.get_ninja_movement()
ai = c.mesh.get_anim_instance()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "jump_action", "focus_action", "chakra_feet_action")}
st = {"t": 0.0, "log": [], "done": set(), "last": 0.0}
unreal._tmo = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def press(name, vec=unreal.Vector(1, 0, 0)):
    sub.inject_input_vector_for_action(IA[name], vec, [], [])


def estado():
    l = c.get_actor_location(); mt = ai.get_current_active_montage()
    diff = unreal.MathLibrary.normalize_axis(c.get_actor_rotation().yaw - pc.get_control_rotation().yaw)
    v = c.get_velocity()
    log("z %.0f x %.0f vz %.0f vx %.0f %s | corpo-câmera %.0f° | %s" % (l.z, l.x, v.z, v.x, mv.movement_mode.name[5:], diff, mt.get_name() if mt else "-"))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("s", 0.3, lambda: (c.set_actor_location_and_rotation(unreal.Vector(-1500, -3500, 100), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0), False, True),
                                pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=0.0, roll=0.0))))
        once("b", 0.6, lambda: press("focus_action"))
        if 0.9 <= t < 3.6:
            press("move_action", unreal.Vector(0, -1, 0))   # segura S até depois de pousar
        once("j", 1.1, lambda: (press("jump_action"), log("batalha=%s, S + ESPAÇO" % c.is_in_battle_mode())))
        if (1.0 <= t < 3.8 or 6.4 <= t < 10.5) and t - st["last"] >= 0.15:
            st["last"] = t
            estado()
        # super pulo: sai da batalha, liga a infusão nos pés, segura Espaço 1,6 s (barra cheia) e solta
        once("boff", 3.9, lambda: press("focus_action"))
        once("p", 4.2, lambda: (c.set_actor_location_and_rotation(unreal.Vector(-1500, -3500, 100), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0), False, True),
                                pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=0.0, roll=0.0)),
                                None if c.is_passive_enabled(unreal.NinjaPassive.CHAKRA_FEET) else press("chakra_feet_action")))
        if 4.8 <= t < 6.4:
            press("jump_action")
        once("sj", 6.4, lambda: log("solta Espaço: batalha=%s carga %.2f" % (c.is_in_battle_mode(), c.get_super_jump_charge())))
        for i, tt in enumerate([1.4, 1.8, 3.2, 7.4, 8.4]):
            once("f%d" % i, tt, lambda: unreal.SystemLibrary.execute_console_command(w, "Shot"))
        if t > 10.6:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
