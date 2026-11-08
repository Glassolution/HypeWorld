"""Custo de GPU do Gōkakyū v2 no Play: ProfileGPU num quadro sem o jutsu (base) e nos picos (carga 0,25 s, liberação
0,35 s, voo 0,6 s, impacto). O resultado sai no log do editor (LogRHI); quem lê compara as passadas de translúcido.
Câmera do jogo, sem câmera lenta. Registro em unreal._teste_gk['log'].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
TEMPOS = getattr(unreal, "_teste_custo_tempos", [0.25, 0.35, 0.6, 1.15])
st = {"t": 0.0, "log": [], "done": set(), "t0": None, "i": 0}
unreal._teste_gk = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def prof(tag):
    unreal.SystemLibrary.execute_console_command(w, "ProfileGPU")
    unreal.log("GOKAKYU_CUSTO_MARCA %s" % tag)
    log("ProfileGPU %s" % tag)


def tick(dt):
    try:
        st["t"] += dt
        if "base" not in st["done"] and st["t"] > 0.3:
            st["done"].add("base")
            prof("base")
        if "cast" not in st["done"] and st["t"] > 1.2:
            st["done"].add("cast")
            log("cast %s" % c.try_use_jutsu_slot(0))
        now = unreal.GameplayStatics.get_time_seconds(w)
        if st["t0"] is None and "cast" in st["done"]:
            for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile):
                if a.get_actor_location().z > -10000:
                    st["t0"] = now
        if st["t0"] is not None and st["i"] < len(TEMPOS) and now - st["t0"] >= TEMPOS[st["i"]]:
            prof("%.2f" % TEMPOS[st["i"]])
            st["i"] += 1
        if st["i"] >= len(TEMPOS) or st["t"] > 12:
            log("fim")
            unreal.unregister_slate_post_tick_callback(st["h"])
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
