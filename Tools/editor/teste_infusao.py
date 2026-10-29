"""Teste da infusão de chakra nos pés no Play (teclas simuladas): super pulo, parede/teto no túnel, desligar no teto, lago."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "jump_action", "chakra_feet_action")}
st = {"t": 0.0, "log": [], "done": set(), "zmax": -1e9}
unreal._teste_inf = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def inject(name, vec=unreal.Vector(1, 0, 0)):
    sub.inject_input_vector_for_action(IA[name], vec, [], [])


def shot(label):
    unreal.SystemLibrary.execute_console_command(w, "Shot showui")
    g = m.get_gravity_direction()
    l = c.get_actor_location()
    mt = c.mesh.get_anim_instance().get_current_active_montage()
    log("FOTO %s | pos (%.0f %.0f %.0f) | gravidade (%.2f %.2f %.2f) | modo %s | carga %.2f | montagem %s" % (label, l.x, l.y, l.z, g.x, g.y, g.z, m.movement_mode.name[5:], c.get_super_jump_charge(), mt.get_name() if mt else "-"))


def place(x, y, z, yaw):
    c.set_actor_location_and_rotation(unreal.Vector(x, y, z), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False, True)
    m.set_editor_property("velocity", unreal.Vector(0, 0, 0))
    pc.set_control_rotation(unreal.Rotator(pitch=-10.0, yaw=yaw, roll=0.0))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        l = c.get_actor_location()
        # 1) super pulo: segura Espaço de 1,0 a 2,8 s
        once("p", 0.3, lambda: (place(-1500, -2000, 100, 0.0), log("infusão ligada? %s" % c.is_passive_enabled(unreal.NinjaPassive.CHAKRA_FEET))))
        if 1.0 <= t < 2.8:
            inject("jump_action")
        once("c1", 1.6, lambda: shot("carregando"))
        once("c2", 2.5, lambda: shot("carregando mais"))
        once("x0", 2.79, lambda: st.update(x0=c.get_actor_location().x, z0=c.get_actor_location().z, zmax=-1e9))
        if 2.8 <= t < 6.0:
            st["zmax"] = max(st["zmax"], l.z)
        for i, tt in enumerate([2.95, 3.3]):
            once("j%d" % i, tt, lambda: shot("super pulo no ar"))
        once("land", 6.0, lambda: log("SUPER PULO: andou %.0f cm, subiu %.0f cm, modo %s" % (c.get_actor_location().x - st["x0"], st["zmax"] - st["z0"], m.movement_mode.name[5:])))
        # toque rápido = pulo normal
        once("tap0", 6.3, lambda: (place(-1500, -2000, 100, 0.0), st.update(z0=100.0, zmax=-1e9)))
        if 6.6 <= t < 6.68:
            inject("jump_action")
        if 6.6 <= t < 7.6:
            st["zmax"] = max(st["zmax"], l.z)
        once("tap1", 7.7, lambda: log("TOQUE RÁPIDO: subiu %.0f cm (pulo normal ~245)" % (st["zmax"] - 92)))
        # 2) túnel: anda contra a parede B (+Y) e continua "para a frente"
        once("w0", 8.0, lambda: place(3500, -2000, 100, 90.0))
        if 8.3 <= t < 17.5:
            inject("move_action", unreal.Vector(0, 1, 0))
        for i, tt in enumerate([10.5, 12.0, 13.5, 15.0, 17.4]):
            once("ws%d" % i, tt, lambda: shot("túnel"))
        # 3) desliga no teto
        once("off", 17.6, lambda: inject("chakra_feet_action"))
        once("offl", 17.7, lambda: log("V: infusão agora %s" % c.is_passive_enabled(unreal.NinjaPassive.CHAKRA_FEET)))
        once("offs", 18.6, lambda: shot("depois de desligar"))
        once("on", 18.8, lambda: inject("chakra_feet_action"))
        # 4) lago: da ponta do deque (x 4100, z 120) anda para +X sobre a água
        once("l0", 19.2, lambda: place(4100, 2800, 230, 0.0))
        if 19.5 <= t < 23.0:
            inject("move_action", unreal.Vector(0, 1, 0))
        once("ls", 23.1, lambda: shot("sobre a água (infusão ligada)"))
        once("loff", 23.3, lambda: inject("chakra_feet_action"))
        once("ls2", 24.5, lambda: shot("afundou (infusão desligada)"))
        once("lon", 24.7, lambda: inject("chakra_feet_action"))
        if t > 25.0:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
