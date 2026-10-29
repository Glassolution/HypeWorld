"""Teste no Play: menu de combate (foto com interface), troca de estilo, combos variados no boneco e dashes por direção.
Registro em unreal._teste_est['log']."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
ai = c.mesh.get_anim_instance()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "dash_action", "combat_menu_action", "jump_action")}
dummy = sorted(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaTrainingDummy), key=lambda a: a.get_actor_label())[0]
st = {"t": 0.0, "log": [], "done": set(), "last": None}
unreal._teste_est = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def press(name, vec=unreal.Vector(1, 0, 0)):
    sub.inject_input_vector_for_action(IA[name], vec, [], [])


def near_dummy():
    d = dummy.get_actor_location(); f = dummy.get_actor_forward_vector()
    c.set_actor_location_and_rotation(unreal.Vector(d.x + f.x * 140, d.y + f.y * 140, d.z), unreal.Rotator(pitch=0.0, yaw=dummy.get_actor_rotation().yaw + 180.0, roll=0.0), False, True)


def hit():
    to = dummy.get_actor_location() - c.get_actor_location()
    pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=unreal.MathLibrary.make_rot_from_x(to).yaw, roll=0.0))
    c.do_attack_pressed()


def style(i):
    ok = c.select_fighting_style(i)
    log("estilo %d -> %s (%s)" % (i, c.get_fighting_style().get_editor_property("display_name"), ok))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("menu1", 0.5, lambda: press("combat_menu_action"))
        once("shot1", 1.2, lambda: (unreal.SystemLibrary.execute_console_command(w, "Shot showui"), log("menu aberto=%s" % pc.is_combat_menu_open())))
        once("menu2", 1.6, lambda: press("combat_menu_action"))
        once("menu3", 1.8, lambda: log("menu aberto=%s" % pc.is_combat_menu_open()))
        once("lock", 1.9, lambda: log("estilo com arma (Katana) aceito? %s" % c.select_fighting_style(3)))
        # combos: 3 estilos x 3 sequências de 4 cliques, com pausa entre as sequências
        T = 2.2
        for si in range(3):
            once("s%d" % si, T, lambda si=si: (style(si), near_dummy()))
            T += 0.3
            for seqn in range(3):
                for k in range(4):
                    once("a%d_%d_%d" % (si, seqn, k), T + k * 0.42, hit)
                T += 4 * 0.42 + 2.2
        st["end_combos"] = T
        # registro do golpe que está tocando
        mt = ai.get_current_active_montage()
        name = mt.get_name() if mt else None
        if name and name != st["last"] and ("AM_G_" in name or "AM_Dash" in name or "AM_ANAP_Dash" in name):
            log("toca %s" % name)
        st["last"] = name
        # dashes por direção (câmera olhando +X; W/S/A/D pelo Move)
        D = T + 0.5
        dirs = [("frente", unreal.Vector(0, 1, 0)), ("frente2", unreal.Vector(0, 1, 0)), ("esquerda", unreal.Vector(-1, 0, 0)), ("esquerda2", unreal.Vector(-1, 0, 0)),
                ("direita", unreal.Vector(1, 0, 0)), ("tras", unreal.Vector(0, -1, 0))]
        for i, (nome, v) in enumerate(dirs):
            base = D + i * 1.3
            once("dp%d" % i, base, lambda: (c.set_actor_location(unreal.Vector(-600, -2000, 100), False, True), pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=0.0, roll=0.0))))
            if base + 0.2 <= t < base + 0.45:
                press("move_action", v)
            once("dd%d" % i, base + 0.35, lambda nome=nome: (press("dash_action"), log("dash %s" % nome)))
            once("dy%d" % i, base + 0.5, lambda nome=nome: log("   corpo yaw %.0f (câmera 0) | vel %.0f" % (c.get_actor_rotation().yaw, c.get_velocity().length())))
        A = D + len(dirs) * 1.3
        once("air0", A, lambda: c.set_actor_location(unreal.Vector(-600, -2000, 100), False, True))
        once("air1", A + 0.3, lambda: press("jump_action"))
        once("air2", A + 0.55, lambda: (press("dash_action"), log("dash no ar")))
        if t > A + 2.0:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
