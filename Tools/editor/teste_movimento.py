"""Teste automático do movimento ninja (rodar com o jogo em Play no editor).
Roda um roteiro quadro a quadro dentro do editor e guarda o resultado em unreal._teste_mov.
Rodar:  python3 -I Tools/ue_exec.py -f Tools/editor/teste_movimento.py
Ler:    python3 -I Tools/ue_exec.py "import unreal; print(chr(10).join(unreal._teste_mov['log']))"
"""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world()
c = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.HypeWorldCharacter)[0]
m = c.get_ninja_movement()

st = {"t": 0.0, "fase": "inicio", "log": [], "dados": {}}
unreal._teste_mov = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def pos():
    return c.get_actor_location()


def ir(fase):
    st["fase"] = fase
    st["t0"] = st["t"]


def tick(dt):
    try:
        st["t"] += dt
        f, tf = st["fase"], st["t"] - st.get("t0", 0.0)
        d = st["dados"]

        if f == "inicio":
            c.set_actor_location(unreal.Vector(0, -2000, 100), False, True)
            m.set_editor_property("velocity", unreal.Vector(0, 0, 0))
            ir("dash_espera")
        elif f == "dash_espera" and tf > 0.5:
            d["dash_x0"] = pos().x
            c.do_dash()
            ir("dash")
        elif f == "dash":
            if m.is_dashing():
                d["dash_visto"] = True
            if tf > 1.5:
                log("DASH: dash ativo visto=%s, andou %.0f cm (esperado ~480 + freada)" % (d.get("dash_visto"), pos().x - d["dash_x0"]))
                c.set_actor_location(unreal.Vector(0, -3000, 100), False, True)
                ir("pulo_espera")
        elif f == "pulo_espera" and tf > 0.5:
            d["z0"] = pos().z
            d["zmax1"] = d["z0"]
            c.jump()
            ir("pulo1")
        elif f == "pulo1":
            c.stop_jumping()
            d["zmax1"] = max(d["zmax1"], pos().z)
            if tf > 0.35:
                d["z_no_2"] = pos().z
                d["zmax2"] = pos().z
                c.jump()
                ir("pulo2")
        elif f == "pulo2":
            c.stop_jumping()
            d["zmax2"] = max(d["zmax2"], pos().z)
            if m.movement_mode == unreal.MovementMode.MOVE_WALKING and tf > 0.2:
                log("PULO: só 1º pulo subiria ~%.0f cm; altura máx total %.0f cm (pulo duplo se > ~260)" % (
                    (900 ** 2) / (2 * 980 * 1.6), d["zmax2"] - d["z0"]))
                # Corrida na parede: corre ao lado da parede (y=1175 a 1225 cm) em +X com corrida ninja e pula
                c.set_actor_location(unreal.Vector(-600, 1110, 100), False, True)
                c.do_sprint_start()
                d["wr_tempo"] = 0.0
                ir("parede")
        elif f == "parede":
            c.add_movement_input(unreal.Vector(1, 0, 0), 1.0)
            if tf > 0.7 and "wr_pulou" not in d:
                d["wr_pulou"] = True
                d["wr_x0"] = pos().x
                c.jump()
            elif "wr_pulou" in d:
                c.stop_jumping()
            if m.is_wall_running():
                d["wr_tempo"] += dt
                d["wr_zmin"] = min(d.get("wr_zmin", 9999), pos().z)
                d["wr_zmax"] = max(d.get("wr_zmax", -9999), pos().z)
            if tf > 3.0:
                c.do_sprint_end()
                log("PAREDE: correu na parede por %.2f s (máx 1.5), andou %.0f cm em X desde o pulo, z na parede %.0f a %.0f cm" % (
                    d["wr_tempo"], pos().x - d.get("wr_x0", 0), d.get("wr_zmin", -1), d.get("wr_zmax", -1)))
                ir("fim")
        elif f == "fim":
            log("FIM")
            unreal.unregister_slate_post_tick_callback(st["handle"])
            ir("parado")
    except Exception as e:  # noqa: BLE001
        log("ERRO: %r" % e)
        unreal.unregister_slate_post_tick_callback(st["handle"])
        st["fase"] = "parado"


st["handle"] = unreal.register_slate_post_tick_callback(tick)
print("teste iniciado")
