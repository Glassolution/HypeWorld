"""Teste do combo aéreo (lançador com Espaço no meio do combo + golpes no ar) e do mortal para trás (batalha + S + Espaço)."""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
ai = c.mesh.get_anim_instance()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "jump_action", "attack_action", "focus_action")}
ALVO = getattr(unreal, "_tca_alvo", "Boneco_Treino_2")   # Treino_2 defende sempre
dummy = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaTrainingDummy) if a.get_actor_label() == ALVO)
st = {"t": 0.0, "log": [], "done": set(), "last": 0.0, "hp": None}
unreal._tca = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def press(name, vec=unreal.Vector(1, 0, 0)):
    sub.inject_input_vector_for_action(IA[name], vec, [], [])


def aim():
    to = dummy.get_actor_location() - c.get_actor_location()
    pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=unreal.MathLibrary.make_rot_from_x(to).yaw, roll=0.0))


def setup():
    d = dummy.get_actor_location(); f = dummy.get_actor_forward_vector()
    c.set_actor_location_and_rotation(unreal.Vector(d.x + f.x * 140, d.y + f.y * 140, d.z), unreal.Rotator(pitch=0.0, yaw=dummy.get_actor_rotation().yaw + 180.0, roll=0.0), False, True)
    st["hp"] = dummy.get_health()
    aim()


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("s", 0.3, setup)
        # Como o Felipe joga: combo normal, no MEIO do combo segura Espaço (modo de batalha automático) e continua clicando
        for i, tt in enumerate([1.0, 1.35, 1.7, 2.0, 2.3]):
            once("a%d" % i, tt, lambda: (aim(), press("attack_action")))
        if 1.5 <= t < 2.5:
            press("jump_action")
        once("j", 1.5, lambda: log("segura ESPAÇO no meio do combo (batalha=%s)" % c.is_in_battle_mode()))
        for i, tt in enumerate([2.9, 3.3, 3.7, 4.1, 4.5, 4.9]):
            once("aa%d" % i, tt, lambda: (aim(), press("attack_action")))
        if 0.9 <= t < 6.8 and t - st["last"] >= 0.2:
            st["last"] = t
            mt = ai.get_current_active_montage()
            hp = dummy.get_health()
            dist = (dummy.get_actor_location() - c.get_actor_location()); dist.z = 0.0
            log("eu z %.0f (%s%s) | boneco z %.0f (%s%s) dist %.0f invul %s | vida %.0f | %s" % (
                c.get_actor_location().z, c.get_ninja_movement().movement_mode.name[5:], " SUSP" if c.get_ninja_movement().is_air_suspended() else "",
                dummy.get_actor_location().z, dummy.get_ninja_movement().movement_mode.name[5:], " SUSP" if dummy.get_ninja_movement().is_air_suspended() else "",
                dist.length(), dummy.is_invulnerable(), hp, mt.get_name() if mt else "-"))
        # mortal para trás: modo de batalha (clique direito), segura S e aperta Espaço
        once("b0", 7.0, lambda: (c.set_actor_location_and_rotation(unreal.Vector(-1500, -3500, 100), unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0), False, True), pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=0.0, roll=0.0))))
        once("b1", 7.3, lambda: press("focus_action"))
        if 7.6 <= t < 8.2:
            press("move_action", unreal.Vector(0, -1, 0))
        once("b2", 7.9, lambda: (press("jump_action"), log("batalha=%s, S + ESPAÇO" % c.is_in_battle_mode())))
        if 7.9 <= t < 9.6 and t - st["last"] >= 0.2:
            st["last"] = t
            mt = ai.get_current_active_montage(); l = c.get_actor_location(); v = c.get_velocity()
            log("mortal: pos x %.0f z %.0f | vel x %.0f z %.0f | montagem %s" % (l.x, l.z, v.x, v.z, mt.get_name() if mt else "-"))
        for i, tt in enumerate([2.2, 3.0, 3.8, 4.6, 5.4, 8.15, 8.4, 8.65]):
            once("f%d" % i, tt, lambda: unreal.SystemLibrary.execute_console_command(w, "Shot"))
        once("cam", 7.05, lambda: pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=0.0, roll=0.0)))
        if t > 9.8:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
