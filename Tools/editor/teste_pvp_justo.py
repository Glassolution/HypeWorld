"""Teste do PvP justo no Play: combo longo no boneco (limite de combo, atordoamento caindo) e substituição do boneco.
Registro em unreal._teste_pvp['log']."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
dummy = sorted(unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaTrainingDummy), key=lambda a: a.get_actor_label())[0]
st = {"t": 0.0, "log": [], "done": set(), "hp": None, "stun": False, "stun_t": 0.0}
unreal._teste_pvp = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(key, t, fn):
    if st["t"] >= t and key not in st["done"]:
        st["done"].add(key)
        fn()


def face_dummy_and_press(heavy=False):
    to = dummy.get_actor_location() - c.get_actor_location()
    pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=unreal.MathLibrary.make_rot_from_x(to).yaw, roll=0.0))
    c.do_heavy_attack_pressed() if heavy else c.do_attack_pressed()


def setup():
    d = dummy.get_actor_location()
    fwd = dummy.get_actor_forward_vector()
    c.set_actor_location_and_rotation(unreal.Vector(d.x + fwd.x * 140, d.y + fwd.y * 140, d.z), unreal.Rotator(pitch=0.0, yaw=dummy.get_actor_rotation().yaw + 180.0, roll=0.0), False, True)
    st["hp"] = dummy.get_health()
    log("início: vida do boneco %.0f | cargas de substituição do boneco %d" % (st["hp"], dummy.get_substitution_charges()))


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("setup", 0.2, setup)
        once("lim", 0.25, lambda: (c.set_editor_property("max_combo_hits", int(globals().get("LIMITE", 7))), log("limite de combo do teste: %d" % c.get_editor_property("max_combo_hits"))))
        # Fase A: aperta ataque sem parar (combo longo), 0,45 s entre cliques
        for i in range(4):
            once("a%d" % i, 1.0 + i * 0.4, lambda: face_dummy_and_press())
        # depois da corrente: segue batendo (golpe forte + leves) para chegar no limite de combo
        once("h", 2.7, lambda: face_dummy_and_press(heavy=True))
        for i in range(6):
            once("c%d" % i, 3.6 + i * 0.4, lambda: face_dummy_and_press())
        # Fase B: novo combo e o boneco usa a substituição no 2º golpe
        once("resetB", 8.5, lambda: (setup(), log("---- fase B: substituição ----")))
        once("b0", 9.0, lambda: face_dummy_and_press())
        once("b1", 9.45, lambda: face_dummy_and_press())
        def sub():
            p0 = dummy.get_actor_location()
            log("boneco atordoado=%s tenta substituição" % dummy.is_stunned())
            dummy.try_substitution()
            p1 = dummy.get_actor_location()
            log("substituição: andou %.0f cm | cargas agora %d | invulnerável %s | atordoado %s" % ((p1 - p0).length(), dummy.get_substitution_charges(), dummy.is_invulnerable(), dummy.is_stunned()))
        once("bs", 9.75, sub)
        once("b2", 9.8, lambda: face_dummy_and_press())
        once("bend", 11.0, lambda: log("fim B: vida %.0f | invulnerável %s" % (dummy.get_health(), dummy.is_invulnerable())))
        # registro contínuo de acertos e atordoamento
        hp = dummy.get_health()
        if st["hp"] is not None and hp < st["hp"] - 0.5:
            log("ACERTO -%.0f (vida %.0f) | dist %.0f | invulnerável depois=%s" % (st["hp"] - hp, hp, (dummy.get_actor_location() - c.get_actor_location()).length(), dummy.is_invulnerable()))
        st["hp"] = hp
        s = dummy.is_stunned()
        if s and not st["stun"]:
            st["stun_t"] = t
        if not s and st["stun"]:
            log("   atordoado por %.2f s" % (t - st["stun_t"]))
        st["stun"] = s
        if dummy.is_invulnerable() != st.get("inv", False):
            st["inv"] = dummy.is_invulnerable()
            log("   boneco invulnerável=%s" % st["inv"])
        if t > 11.5:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
