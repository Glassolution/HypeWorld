"""Investigação da "trava" do Gōkakyū v2 ao soltar: 3 disparos seguidos (9 s entre eles), registrando por quadro,
de 0,2 s a 0,7 s depois do cuspe: idade do projétil (s de jogo), distância do centro até o ponto de saída (cm),
duração real do quadro (ms, para ver engasgo de shader/PSO) e os sistemas Niagara vivos na boca.
Resultado em unreal._teste_trava (lista de linhas). Rodar com o jogo em Play.
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
st = {"t": 0.0, "casts": 0, "next": 0.5, "p": None, "t0": None, "start": None, "out": []}
unreal._teste_trava = st["out"]


def tick(dt):
    try:
        st["t"] += dt
        if st["p"] is None and st["casts"] < 3 and st["t"] >= st["next"]:
            ok = c.try_use_jutsu_slot(0)
            st["casts"] += 1
            st["out"].append("== disparo %d: %s" % (st["casts"], ok))
            st["next"] = st["t"] + 9.0
            st["p"] = "esperando"
        now = unreal.GameplayStatics.get_time_seconds(w)
        if st["p"] == "esperando":
            for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile):
                if a.get_actor_location().z > -10000:
                    st["p"], st["t0"], st["start"] = a, now, a.get_actor_location()
                    st["out"].append("nasceu em %s" % st["start"])
        if st["p"] not in (None, "esperando"):
            age = now - st["t0"]
            p = st["p"]
            if age >= 0.2 and age <= 0.7:
                try:
                    d = (p.get_actor_location() - st["start"]).length()
                    core = [m for m in p.get_components_by_class(unreal.StaticMeshComponent)
                            if m.static_mesh and m.static_mesh.get_name() == "SM_Katon_Core"]
                    dc = (core[0].get_world_location() - st["start"]).length() if core else -1
                except Exception:
                    d, dc = -1, -1
                st["out"].append("idade %.3f  ator %7.1f cm  nucleo %7.1f cm  quadro %5.1f ms" % (age, d, dc, dt * 1000))
            if age > 0.7:
                st["p"] = None
        if st["casts"] >= 3 and st["p"] is None:
            st["out"].append("fim")
            unreal.unregister_slate_post_tick_callback(st["h"])
    except Exception as e:
        st["out"].append("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
