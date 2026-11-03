"""Teste rápido de traje modular no Play: cabeça + cabelo do Indra + o traje, na mesma pose do jogador (Leader Pose), longe
dele para aparecer sozinho. Fotos com Shot: parado (frente, costas, pescoço de frente e de costas), correndo e atacando
(corpo e pescoço). Uso: unreal._teste_traje_malha = caminho do SK_Outfit_*; resultado em unreal._teste_traje.
Com unreal._teste_traje_modo = "congelado": só o golpe parado sempre no mesmo instante, para comparar duas versões da malha."""
import unreal

MOD = "/Game/HypeWorld/Personagens/Indra/Modular/"
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
body = next(m for m in c.get_components_by_class(unreal.SkeletalMeshComponent) if m.get_name() == "Indra")
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
HEAD = "9ind00t0-head1_0243_150"
LADO = 400.0
st = {"t": 0.0, "log": [], "done": set(), "view": None, "extras": []}
unreal._teste_traje = st


def log(msg):
    st["log"].append("%.1f %s" % (st["t"], msg))


def shot(tag):
    log("foto " + tag)
    unreal.SystemLibrary.execute_console_command(w, "Shot")


def novo(cls):
    return unreal.NinjaEditorTools.spawn_actor_in_play_world(c, cls)


def boneco(malha):
    a = novo(unreal.SkeletalMeshActor)
    comp = a.skeletal_mesh_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.attach_to_component(body, "", unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET,
                          unreal.AttachmentRule.SNAP_TO_TARGET, False)
    a.root_component.set_relative_location(unreal.Vector(LADO, 0, 0), False, False)
    comp.set_skinned_asset_and_update(unreal.load_asset(malha))
    comp.set_leader_pose_component(body)
    st["extras"].append(a)
    return comp


def montar():
    ap = next(x for x in c.get_components_by_class(unreal.HypeAppearanceComponent))
    log("menu: " + str([str(p.get_editor_property("id")) for p in ap.get_editor_property("outfit_options")]))
    st["cam"] = novo(unreal.CameraActor)
    st["cam"].camera_component.set_editor_property("constrain_aspect_ratio", False)
    pc.set_view_target_with_blend(st["cam"], 0.0)
    boneco(MOD + "SK_Indra_Head")
    boneco(unreal._teste_traje_malha)
    st["cabelo"] = boneco(MOD + "SK_Hair_Indra")
    log("montado")


def osso(frag):
    return next(str(body.get_bone_name(i)) for i in range(body.get_num_bones()) if str(body.get_bone_name(i)).startswith("9ind00t0-" + frag + "_"))


def congelar(pos):
    """Golpes duram menos de 0,5 s: para no primeiro quadro em que a montagem aparece."""
    ai = c.mesh.get_anim_instance()
    m = ai.get_current_active_montage()
    if not m:
        return False
    ai.montage_pause(m)
    ai.montage_set_position(m, pos)
    log("congelado em %.2f, %s" % (pos, m.get_name()))
    return True


def view(a, z, alvo_z, fov, bone=None):
    st["view"] = (a, z, alvo_z)
    st["bone"] = bone or HEAD
    st["cabelo"].set_visibility(a in (540, -540) or st.get("com_cabelo", False))   # nos closes o cabelo esconderia a emenda
    st["cam"].camera_component.set_editor_property("field_of_view", fov)


def place_cam():
    if not st["view"]:
        return
    a, z, alvo_z = st["view"]
    alvo = body.get_socket_location(st["bone"]) + unreal.Vector(0, 0, alvo_z) + body.get_forward_vector() * (LADO * body.get_editor_property("relative_scale3d").x)
    pos = alvo + c.get_actor_forward_vector() * a + unreal.Vector(0, 0, z)
    st["cam"].set_actor_location_and_rotation(pos, unreal.MathLibrary.find_look_at_rotation(pos, alvo), False, False)


def press(nome):
    sub.inject_input_vector_for_action(c.get_editor_property(nome), unreal.Vector(1, 0, 0), [], [])


FRENTE, COSTAS = (540, 0, -75, 40), (-540, 0, -75, 40)
PESC_F, PESC_C = (70, 0, -7, 30), (-70, 0, -7, 30)
ev = [(0.5, "montar", montar)]
t = 1.2
MODO = getattr(unreal, "_teste_traje_modo", "")
for nome, v in () if MODO == "congelado" else (("frente", FRENTE), ("costas", COSTAS), ("pescoco_frente", PESC_F), ("pescoco_costas", PESC_C)):
    ev.append((t, "v_" + nome, lambda v=v: view(*v)))
    ev.append((t + 0.7, "s_" + nome, lambda n=nome: shot(n)))
    t += 1.1
ev += [] if MODO == "congelado" else [(t, "v_cor", lambda: view(*FRENTE)), (t + 0.1, "sp", lambda: (st.__setitem__("anda", True), press("sprint_action"))),
       (t + 1.7, "s_cor", lambda: shot("corrida")), (t + 2.0, "v_cor2", lambda: view(*PESC_F)),
       (t + 2.7, "s_cor2", lambda: shot("corrida_pescoco")), (t + 3.0, "para", lambda: st.__setitem__("anda", False))]
t += 4.2
ev += [(1.0, "ca", lambda: view(*FRENTE)), (1.3, "ga", lambda: (press("attack_action"), st.__setitem__("congelar", 0.35)))]
tc = 2.4
for nome, v in (("peito", PESC_F), ("corpo", (260, 0, -40, 40)), ("braco_e", (90, 0, 0, 30, "l-forearm")), ("braco_d", (90, 0, 0, 30, "r-forearm"))):
    ev.append((tc, "cv" + nome, lambda v=v: view(*(v[:4] + ((osso(v[4]),) if len(v) > 4 else ())))))
    ev.append((tc + 0.8, "cs" + nome, lambda n=nome: shot("congelado_" + n)))
    tc += 1.2
if MODO == "congelado":
    t = tc - 4.5
ev += [] if MODO == "congelado" else [(t, "v_at", lambda: view(*FRENTE)), (t + 0.4, "a1", lambda: press("attack_action")), (t + 0.75, "s_at", lambda: shot("ataque")),
       (t + 2.0, "v_at2", lambda: view(*PESC_F)), (t + 2.6, "a2", lambda: press("attack_action")), (t + 2.95, "s_at2", lambda: shot("ataque_pescoco"))]
if MODO != "congelado":
    ev = [e for e in ev if e[1] not in ("ca", "ga") and not e[1].startswith(("cv", "cs"))]
ev.append((t + 4.5, "fim", None))


def tick(dt):
    st["t"] += dt
    if st.get("anda"):
        sub.inject_input_vector_for_action(c.get_editor_property("move_action"), unreal.Vector(-1, 0, 0), [], [])
    if st.get("congelar") and congelar(st["congelar"]):
        st["congelar"] = None
    if st.get("cam"):
        place_cam()
    for t_, k, fn in ev:
        if st["t"] >= t_ and k not in st["done"]:
            st["done"].add(k)
            try:
                if fn:
                    fn()
                if k == "fim":
                    for a in st["extras"] + [st["cam"]]:
                        a.destroy_actor()
                    log("fim")
                    unreal.unregister_slate_post_tick_callback(st["h"])
            except Exception as e:
                log("ERRO %s %s" % (k, e))
                unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
