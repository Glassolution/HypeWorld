"""Teste de um cabelo no personagem do Play: põe o cabelo (Leader Pose, como o HypeAppearanceComponent),
fotos de perto de frente/lado/costas/cima parado e depois correndo, pulando e no dash (comando Shot).
Rodar com o jogo em Play (ue_exec -f). Registro em unreal._teste_cab['log']. Trocar CABELO para testar outro.
PREPARAR (editor parado, antes do Play): no mapa do editor, um SkeletalMeshActor "TesteCabelo" e um CameraActor
"TesteCabeloCam" (ue_exec com PREPARAR abaixo). Depois do teste: apagar os dois e não salvar o mapa.
PREPARAR = 'import unreal;s=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);[s.spawn_actor_from_class(k,unreal.Vector(0,0,-5000)).set_actor_label(n) for k,n in ((unreal.SkeletalMeshActor,"TesteCabelo"),(unreal.CameraActor,"TesteCabeloCam"))]'
"""
import unreal

# outro cabelo / só as fotos paradas: definir unreal._teste_cab_nome / unreal._teste_cab_parado antes de rodar
CABELO = getattr(unreal, "_teste_cab_nome", "Hair_itachi")
SO_PARADO = getattr(unreal, "_teste_cab_parado", False)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
body = next(m for m in c.get_components_by_class(unreal.SkeletalMeshComponent) if m.get_name() == "Indra")
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "sprint_action", "jump_action", "dash_action")}
st = {"t": 0.0, "log": [], "done": set(), "cam": None, "view": None}
unreal._teste_cab = st


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def shot(tag):
    log("foto " + tag)
    unreal.SystemLibrary.execute_console_command(w, "Shot")


def put_hair():
    mesh = unreal.load_asset("/Game/Ninja/Characters/Customization/%s/SK_%s" % (CABELO, CABELO))
    # o Python não cria componente no personagem nem atores no Play: o ator "TesteCabelo" é posto no mapa do editor
    # antes do Play (ver PREPARAR no começo) e aqui é preso ao corpo com Leader Pose
    holder = next(x for x in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.SkeletalMeshActor) if x.get_actor_label() == "TesteCabelo")
    hair = holder.skeletal_mesh_component
    hair.set_mobility(unreal.ComponentMobility.MOVABLE)
    hair.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    holder.attach_to_component(body, "", unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET, False)
    hair.set_skinned_asset_and_update(mesh)
    hair.set_leader_pose_component(body)
    # como o HypeAppearanceComponent: tira o MID do cabelo anterior e pinta todos os espaços de material
    for k, slot in enumerate(mesh.get_editor_property("materials")):
        hair.create_dynamic_material_instance(k, slot.get_editor_property("material_interface")).set_vector_parameter_value("BaseColor Tint", unreal.LinearColor(0.025, 0.02, 0.02, 1))
    # couro cabeludo como o HypeAppearanceComponent faz quando há cabelo (Scalp Strength 0,85, cor do cabelo)
    body.set_scalar_parameter_value_on_materials("Scalp Strength", 0.85)
    body.set_vector_parameter_value_on_materials("Scalp Color", unreal.Vector(0.025, 0.02, 0.02))
    st["cam"] = next(x for x in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.CameraActor) if x.get_actor_label() == "TesteCabeloCam")
    st["cam"].camera_component.set_editor_property("field_of_view", 30.0)
    st["cam"].camera_component.set_editor_property("constrain_aspect_ratio", False)
    pc.set_view_target_with_blend(st["cam"], 0.0)
    log("cabelo %s posto (%s)" % (CABELO, mesh.get_name()))


def place_cam(dt):
    # câmera olhando para a cabeça; 'view' = (para a frente do corpo, para a direita, para cima) em cm
    if not st["cam"] or not st["view"]:
        return
    # prevê onde a cabeça vai estar no quadro desenhado (correndo a 8 m/s a câmera ficava para trás)
    head = body.get_socket_location("9ind00t0-head1_0243_150") + c.get_velocity() * dt
    f, r = c.get_actor_forward_vector(), c.get_actor_right_vector()
    a, b, z = st["view"]
    pos = head + f * a + r * b + unreal.Vector(0, 0, z)
    st["cam"].set_actor_location(pos, False, False)
    st["cam"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos, head + unreal.Vector(0, 0, 4)), False)


def view(v):
    st["view"] = v


def move(x, y):
    sub.inject_input_vector_for_action(IA["move_action"], unreal.Vector(x, y, 0), [], [])


def press(k):
    sub.inject_input_vector_for_action(IA[k], unreal.Vector(1, 0, 0), [], [])


def tick(dt):
    try:
        st["t"] += dt
        t = st["t"]
        once("hair", 0.3, put_hair)
        # parado
        once("v1", 0.6, lambda: view((150, 0, 0)))
        once("s1", 1.2, lambda: shot("frente"))
        once("v2", 1.4, lambda: view((0, 150, 0)))
        once("s2", 2.0, lambda: shot("lado"))
        once("v3", 2.2, lambda: view((-150, 0, 0)))
        once("s3", 2.8, lambda: shot("costas"))
        once("v4", 3.0, lambda: view((40, 0, 145)))
        once("s4", 3.6, lambda: shot("cima"))
        once("v5", 3.8, lambda: view((110, 110, 10)))
        once("s5", 4.4, lambda: shot("tres_quartos"))
        # correndo (Shift = corrida ninja), câmera de lado acompanhando
        if SO_PARADO:
            place_cam(dt)
            if t > 4.6:
                log("fim")
                unreal.unregister_slate_post_tick_callback(st["h"])
            return
        once("v6", 4.6, lambda: (view((40, 300, 30)), st["cam"].camera_component.set_editor_property("field_of_view", 45.0)))
        once("sp", 4.7, lambda: press("sprint_action"))
        if 4.8 <= t < 9.0:
            move(0, 1)
        once("s6", 6.0, lambda: shot("corrida"))
        once("j", 6.6, lambda: press("jump_action"))
        once("s7", 6.9, lambda: shot("pulo_subindo"))
        once("s8", 7.25, lambda: shot("pulo_topo"))
        once("d", 8.0, lambda: press("dash_action"))
        once("s9", 8.12, lambda: shot("dash_inicio"))
        once("s10", 8.35, lambda: shot("dash_meio"))
        once("s11", 8.75, lambda: shot("dash_fim"))
        once("v7", 9.2, lambda: view((150, 0, 0)))
        once("s12", 10.2, lambda: shot("parou_frente"))
        place_cam(dt)
        if t > 10.6 or (SO_PARADO and t > 4.6):
            log("fim")
            unreal.unregister_slate_post_tick_callback(st["h"])
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
