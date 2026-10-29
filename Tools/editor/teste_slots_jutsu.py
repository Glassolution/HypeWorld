"""Teste dos slots de jutsu com a arte do Felipe e do bloco de vida menor (com o jogo em Play).
Fotos (comando Shot): fora da batalha (slots abaixados), na batalha (slots levantados) e com estados:
slot 1 selecionado, slot 2 em cooldown (8 de 10 s), slot 3 usado (flash), slots 4/5 sem chakra se o chakra < custo.
Registro em unreal._tsj['log'].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
hud = next(h for h in unreal.ObjectIterator(unreal.NinjaHUDWidget) if h.get_world() == w)
st = {"t": 0.0, "log": [], "done": set()}
unreal._tsj = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def shot(name):
    unreal.SystemLibrary.execute_console_command(w, "Shot showui")
    log("FOTO %s | batalha %s | chakra %.0f" % (name, c.is_in_battle_mode(), c.get_chakra()))


def estados():
    hud.set_selected_jutsu(0)
    hud.set_jutsu_cooldown(1, 8.0, 10.0)
    hud.play_jutsu_used(2)


def tick(dt):
    try:
        st["t"] += dt
        once("f1", 1.5, lambda: shot("fora_da_batalha"))
        once("bat", 1.8, lambda: sub.inject_input_vector_for_action(c.get_editor_property("focus_action"), unreal.Vector(1, 0, 0), [], []))
        once("f2", 3.3, lambda: shot("batalha"))
        once("est", 3.5, estados)
        once("f3", 3.56, lambda: shot("estados_flash"))
        once("f4", 4.3, lambda: shot("estados"))
        if st["t"] > 4.6:
            unreal.unregister_slate_post_tick_callback(st["h"])
            st["ok"] = True
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])
        st["ok"] = True


st["h"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
