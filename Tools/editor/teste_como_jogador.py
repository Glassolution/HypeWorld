"""Teste no Play como o jogador: tudo pelo Enhanced Input (W, Espaço, Q), sem mexer em nada por código.
Fotos em sequência (Shot); registro em unreal._teste_jog['log'] com o estado da animação."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
m = c.get_ninja_movement()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "jump_action", "dash_action")}
st = {"t": 0.0, "log": [], "done": set()}
unreal._teste_jog = st
ai = c.mesh.get_anim_instance()


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def shot(label):
    unreal.SystemLibrary.execute_console_command(w, "Shot")
    mt = ai.get_current_active_montage()
    log("FOTO %s | vel %.0f z %.0f | modo %s | montagem %s seção %s | batalha %s | taxa %.2f" % (label, c.get_velocity().length(), c.get_velocity().z,
        m.movement_mode.name[5:], mt.get_name() if mt else "-", ai.montage_get_current_section(mt) if mt else "-", c.is_in_battle_mode(), c.mesh.get_editor_property("global_anim_rate_scale")))


def once(key, t, fn):
    if st["t"] >= t and key not in st["done"]:
        st["done"].add(key)
        fn()


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        if t < 0.2:
            c.set_actor_location(unreal.Vector(-600, -2000, 100), False, True)
        pc.set_control_rotation(unreal.Rotator(pitch=-8.0, yaw=90.0, roll=0.0))   # câmera de lado
        if 1.6 <= t < 3.6:
            sub.inject_input_vector_for_action(IA["move_action"], unreal.Vector(1, 0, 0), [], [])   # D (anda de lado na tela)
        for i, tt in enumerate([1.0, 1.75, 2.0, 2.4, 3.2]):
            once("p%d" % i, tt, lambda i=i: shot("parado" if i == 0 else "andando"))
        once("j1", 4.5, lambda: sub.inject_input_vector_for_action(IA["jump_action"], unreal.Vector(1, 0, 0), [], []))
        once("j2", 4.85, lambda: sub.inject_input_vector_for_action(IA["jump_action"], unreal.Vector(1, 0, 0), [], []))
        for i, tt in enumerate([4.9, 4.98, 5.06, 5.14, 5.22, 5.3, 5.38, 5.46, 5.6, 5.8]):
            once("dj%d" % i, tt, lambda: shot("pulo duplo"))
        once("d", 7.0, lambda: sub.inject_input_vector_for_action(IA["dash_action"], unreal.Vector(1, 0, 0), [], []))
        for i, tt in enumerate([7.04, 7.12, 7.2, 7.28, 7.36, 7.44, 7.52, 7.6, 7.7, 7.85]):
            once("dd%d" % i, tt, lambda: shot("dash"))
        once("fim", 8.0, lambda: (unreal.unregister_slate_post_tick_callback(st["h"]), st.update(ok=True)))
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
