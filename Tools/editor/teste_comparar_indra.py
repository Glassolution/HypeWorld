"""Compara no Play, na mesma pose: SK_Indra original (esquerda) x personagem do jogador (centro, como o jogo monta)
x Indra modular com cabelo e traje do Indra (direita). Os dois de fora copiam a pose do corpo do jogador (Leader Pose),
sem mexer em nada salvo. Fotos com o comando Shot; idle, corrida e ataque, em close do rosto e corpo inteiro."""
import unreal

INDRA = "/Game/HypeWorld/Personagens/Indra/"
MOD = INDRA + "Modular/"
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
body = next(m for m in c.get_components_by_class(unreal.SkeletalMeshComponent) if m.get_name() == "Indra")
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
HEAD = "9ind00t0-head1_0243_150"
LADO = 95.0
TROCA = -1.0 if getattr(unreal, "_teste_cmp_modo", "") == "trocado" else 1.0   # original à direita: separa luz de malha
st = {"t": 0.0, "log": [], "done": set(), "view": None, "extras": []}
unreal._teste_cmp = st


def log(msg):
    st["log"].append("%.1f %s" % (st["t"], msg))


def shot(tag):
    log("foto " + tag)
    unreal.SystemLibrary.execute_console_command(w, "Shot")


def novo(cls):
    return unreal.NinjaEditorTools.spawn_actor_in_play_world(c, cls)


def boneco(malha, x):
    a = novo(unreal.SkeletalMeshActor)
    comp = a.skeletal_mesh_component
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.attach_to_component(body, "", unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET,
                          unreal.AttachmentRule.SNAP_TO_TARGET, False)
    a.root_component.set_relative_location(unreal.Vector(x, 0, 0), False, False)
    comp.set_skinned_asset_and_update(unreal.load_asset(malha))
    comp.set_leader_pose_component(body)
    st["extras"].append(a)


def montar():
    st["cam"] = novo(unreal.CameraActor)
    st["cam"].camera_component.set_editor_property("constrain_aspect_ratio", False)
    pc.set_view_target_with_blend(st["cam"], 0.0)
    boneco(INDRA + "SK_Indra", -LADO * TROCA)
    for m in ("SK_Indra_Head", "SK_Hair_Indra", "SK_Outfit_Indra"):
        boneco(MOD + m, LADO * TROCA)
    log("montado")


def view(a, z, alvo_z, fov, x=0.0):
    st["view"] = (a, z, alvo_z, x)
    st["cam"].camera_component.set_editor_property("field_of_view", fov)


def place_cam():
    if not st["view"]:
        return
    a, z, alvo_z, x = st["view"]
    alvo = body.get_socket_location(HEAD) + unreal.Vector(0, 0, alvo_z) + body.get_forward_vector() * (x * body.get_editor_property("relative_scale3d").x)
    pos = alvo + c.get_actor_forward_vector() * a + unreal.Vector(0, 0, z)
    st["cam"].set_actor_location_and_rotation(pos, unreal.MathLibrary.find_look_at_rotation(pos, alvo), False, False)


def press(nome):
    ia = c.get_editor_property(nome)
    sub.inject_input_vector_for_action(ia, unreal.Vector(1, 0, 0), [], [])


CORPO = (520, 0, -75, 45)
MODO = getattr(unreal, "_teste_cmp_modo", "completo")
ev = [(0.5, "montar", montar)]
t = 1.2
for nome, x in (("rosto_original", -LADO), ("rosto_jogador", 0.0), ("rosto_modular", LADO)):
    ev.append((t, "v_" + nome, lambda x=x: view(70, 0, 2, 30, x)))
    ev.append((t + 0.7, "s_" + nome, lambda n=nome: shot(n)))
    t += 1.1
ev += [(t, "v_corpo", lambda: view(*CORPO)), (t + 0.8, "s_corpo", lambda: shot("corpo_idle"))]
t += 1.2
if MODO == "completo":
    ev += [(t, "sp", lambda: (st.__setitem__("anda", True), press("sprint_action"))),
           (t + 1.6, "s_corre", lambda: shot("corpo_corrida")), (t + 1.9, "para", lambda: st.__setitem__("anda", False)),
           (t + 3.0, "a", lambda: press("attack_action")), (t + 3.35, "s_ataque", lambda: shot("corpo_ataque"))]
ev.append((ev[-1][0] + 1.5, "fim", None))


def tick(dt):
    st["t"] += dt
    if st.get("anda"):
        sub.inject_input_vector_for_action(c.get_editor_property("move_action"), unreal.Vector(-1, 0, 0), [], [])
    if st.get("cam"):
        place_cam()
    for t, k, fn in ev:
        if st["t"] >= t and k not in st["done"]:
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
