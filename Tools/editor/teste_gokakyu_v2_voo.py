"""Gōkakyū v2 em voo: câmera de cima (trajetória e cauda) ou câmera que acompanha a bola de lado (close).

unreal._teste_voo_modo = "cima" (padrão) ou "close"; unreal._teste_voo_tempos = instantes (s de jogo desde o cuspe);
unreal._teste_voo_slomo (0,3). Mira nivelada (pitch -4, igual ao teste da câmera do jogo).
Fotos (Shot) em Saved/Screenshots/WindowsEditor; registro em unreal._teste_gk['log'].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
MODO = getattr(unreal, "_teste_voo_modo", "cima")
TEMPOS = getattr(unreal, "_teste_voo_tempos", [0.45, 0.6, 0.75, 0.9, 1.05])
SLOMO = getattr(unreal, "_teste_voo_slomo", 0.3)
st = {"t": 0.0, "log": [], "t0": None, "p": None, "shots": 0, "cam": None}
unreal._teste_gk = st


def log(m):
    st["log"].append("[%.2fs] %s" % (st["t"], m))


def fim():
    unreal.SystemLibrary.execute_console_command(w, "slomo 1")
    pc.set_view_target_with_blend(c, 0.0)
    log("fim")
    unreal.unregister_slate_post_tick_callback(st["h"])


def tick(dt):
    try:
        st["t"] += dt
        if "mira" not in st and st["t"] > 0.2:
            # a mira nivela ANTES: a câmera só atualiza no quadro seguinte e o disparo usa o ponto sob a câmera
            st["mira"] = 1
            rot = pc.get_control_rotation()
            pc.set_control_rotation(unreal.Rotator(roll=0, pitch=-4, yaw=rot.yaw))
        if "cast" not in st and st["t"] > 0.6:
            # projéteis de disparos anteriores (explodidos, ainda vivos) não contam
            st["velhos"] = set(a.get_name() for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile))
            unreal.SystemLibrary.execute_console_command(w, "slomo %s" % SLOMO)
            st["cast"] = c.try_use_jutsu_slot(0)
            log("cast %s" % st["cast"])
        now = unreal.GameplayStatics.get_time_seconds(w)
        if st["t0"] is None and "cast" in st:
            for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile):
                if a.get_actor_location().z > -10000 and a.get_name() not in st["velhos"]:
                    st["t0"], st["p"] = now, a
                    st["start"] = a.get_actor_location()
                    st["fwd"] = a.get_actor_forward_vector()
                    log("projétil nasceu")
                    so = getattr(unreal, "_teste_voo_so", None)
                    if so == "malhas":
                        # diagnóstico: esconde só as malhas do C++ (núcleo/línguas), o Niagara fica inteiro
                        for m in a.get_components_by_class(unreal.StaticMeshComponent):
                            m.set_hidden_in_game(True)
                        log("malhas do C++ escondidas")
                    elif so:
                        # diagnóstico: só os emissores pedidos ficam ligados no sistema da bola
                        todos = ["Fita01", "Fita02", "Fita03", "Fita04", "Labaredas", "LabaredasTras", "Linguas", "Cauda",
                                 "SparkTrail", "FrontSparks", "Halo"]
                        for nc in a.get_components_by_class(unreal.NiagaraComponent):
                            if nc.get_asset() and nc.get_asset().get_name().startswith("NS_Katon_Gokakyu_v2_Voo"):
                                for em in todos:
                                    nc.set_emitter_enable(em, em in so)
                                log("só %s ligados" % so)
                        for m in a.get_components_by_class(unreal.StaticMeshComponent):
                            m.set_hidden_in_game(True)
                    if MODO == "cima":
                        f = st["fwd"]
                        pos = st["start"] + unreal.Vector(f.x, f.y, 0) * 950 + unreal.Vector(0, 0, 2900)
                        cam = unreal.NinjaEditorTools.spawn_local_actor(w, unreal.CameraActor, pos,
                                                                        unreal.Rotator(pitch=-89.9, yaw=unreal.MathLibrary.make_rot_from_x(f).yaw, roll=0))
                        cam.camera_component.set_editor_property("field_of_view", 80.0)
                        cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
                        pc.set_view_target_with_blend(cam, 0.0)
                        st["cam"] = cam
        if st["t0"] is not None:
            age = now - st["t0"]
            p = st["p"]
            if MODO == "close" and age > 0.3:
                try:
                    ball = p.get_actor_location()
                    f = st["fwd"]
                    side = unreal.Vector.cross(f, unreal.Vector(0, 0, 1)).normal()
                    pos = ball + side * 750 - f * 250 + unreal.Vector(0, 0, 120)
                    if st["cam"] is None:
                        st["cam"] = unreal.NinjaEditorTools.spawn_local_actor(w, unreal.CameraActor, pos, unreal.Rotator())
                        st["cam"].camera_component.set_editor_property("field_of_view", 60.0)
                        st["cam"].camera_component.set_editor_property("constrain_aspect_ratio", False)
                        pc.set_view_target_with_blend(st["cam"], 0.0)
                    st["cam"].set_actor_location_and_rotation(pos, unreal.MathLibrary.find_look_at_rotation(pos, ball), False, False)
                except Exception:
                    pass
            if st["shots"] < len(TEMPOS) and age >= TEMPOS[st["shots"]]:
                unreal.SystemLibrary.execute_console_command(w, "Shot")
                try:
                    d = (p.get_actor_location() - st["start"]).length()
                except Exception:
                    d = -1
                log("foto %d em %.3f s (bola a %.0f cm da boca)" % (st["shots"], age, d))
                st["shots"] += 1
            if st["shots"] >= len(TEMPOS) and "ultima" not in st:
                st["ultima"] = st["t"]
        if ("ultima" in st and st["t"] > st["ultima"] + 0.6) or st["t"] > 25:
            fim()
    except Exception as e:
        log("ERRO %r" % e)
        fim()


st["h"] = unreal.register_slate_post_tick_callback(tick)
