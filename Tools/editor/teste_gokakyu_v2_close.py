"""Close da bola do Gōkakyū v2 PARADA (4 fotos em 1 s) para ver as fitas girando.

No Play: segura a bola na boca por 3 s só neste teste (GokakyuHoldTime do CDO em memória, sem salvar; volta ao valor
anterior no fim), põe uma câmera de lado a ~7 m da bola e tira fotos (Shot) em 1,0 / 1,25 / 1,5 / 1,75 s depois do
cuspe. Fotos em Saved/Screenshots/WindowsEditor. Registro em unreal._teste_gk['log'].
"""
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
bp = unreal.load_asset("/Game/HypeWorld/Jutsus/Katon/BP_Katon_Goukakyuu")
cdo = unreal.get_default_object(bp.generated_class())
TEMPOS = getattr(unreal, "_teste_close_tempos", [1.0, 1.25, 1.5, 1.75])
DIST = getattr(unreal, "_teste_close_dist", 700.0)
st = {"t": 0.0, "log": [], "done": set(), "t0": None, "shots": 0, "hold": cdo.get_editor_property("gokakyu_hold_time")}
unreal._teste_gk = st
cdo.set_editor_property("gokakyu_hold_time", 3.0)


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def projectile():
    for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.NinjaJutsuProjectile):
        if a.get_actor_location().z > -10000:
            return a
    return None


def fim():
    cdo.set_editor_property("gokakyu_hold_time", st["hold"])
    pc.set_view_target_with_blend(c, 0.0)
    log("fim (hold voltou a %.2f)" % st["hold"])
    unreal.unregister_slate_post_tick_callback(st["h"])


def tick(dt):
    try:
        st["t"] += dt
        if "cast" not in st["done"] and st["t"] > 0.5:
            st["done"].add("cast")
            rot = pc.get_control_rotation()
            pc.set_control_rotation(unreal.Rotator(roll=0, pitch=-4, yaw=rot.yaw))
            log("cast %s" % c.try_use_jutsu_slot(0))
        now = unreal.GameplayStatics.get_time_seconds(w)
        p = projectile()
        if st["t0"] is None and p:
            st["t0"] = now
            log("projétil nasceu")
        if st["t0"] is not None and p:
            age = now - st["t0"]
            if getattr(unreal, "_teste_close_so_niagara", False):
                # só para ver as fitas: esconde no jogo o núcleo e as cascas (malhas do C++)
                for comp in p.get_components_by_class(unreal.StaticMeshComponent):
                    sm = comp.static_mesh
                    if sm and sm.get_name() in ("SM_Katon_Core", "SM_Katon_Shell"):
                        comp.set_hidden_in_game(True)
            if "cam" not in st["done"] and age > 0.6:
                st["done"].add("cam")
                ball = p.get_actor_location()
                fwd = p.get_actor_forward_vector()
                side = unreal.Vector.cross(fwd, unreal.Vector(0, 0, 1)).normal()
                pos = ball + side * DIST + unreal.Vector(0, 0, 120) - fwd * 150
                cam = unreal.NinjaEditorTools.spawn_local_actor(w, unreal.CameraActor, pos,
                                                                unreal.MathLibrary.find_look_at_rotation(pos, ball))
                cam.camera_component.set_editor_property("field_of_view", 60.0)
                cam.camera_component.set_editor_property("constrain_aspect_ratio", False)
                pc.set_view_target_with_blend(cam, 0.0)
                log("câmera de lado em %s, bola em %s" % (pos, ball))
            if st["shots"] < len(TEMPOS) and age >= TEMPOS[st["shots"]]:
                unreal.SystemLibrary.execute_console_command(w, "Shot")
                log("foto %d em %.3f s" % (st["shots"], age))
                st["shots"] += 1
        if st["shots"] >= len(TEMPOS) and "ultima" not in st:
            st["ultima"] = st["t"]
        # espera a última foto ser gravada (o Shot sai alguns quadros depois) antes de devolver a câmera
        if ("ultima" in st and st["t"] > st["ultima"] + 0.5) or st["t"] > 15.0:
            fim()
    except Exception as e:
        log("ERRO %r" % e)
        fim()


st["h"] = unreal.register_slate_post_tick_callback(tick)
