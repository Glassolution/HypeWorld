"""Teste do Indra modular no personagem do Play (cabeça careca SK_Indra_Head no corpo visível + SK_Outfit_Indra e
SK_Hair_Indra presos por Leader Pose, como o HypeAppearanceComponent faz com as peças).
Fotos (comando Shot, Saved/Screenshots): careca de frente/lado/costas/cima/três quartos e close da emenda; careca
parado (idle), correndo, pulando, no dash e atacando; com o cabelo do Indra nas mesmas vistas; e o SK_Indra original
nas mesmas vistas, para comparar. Ordem das fotos em unreal._teste_im['log'].
PREPARAR (editor parado, antes do Play): ue_exec com PREPARAR abaixo (cria TesteOutfit, TesteHair e TesteIndraCam
longe do mapa). Depois: apagar os três atores e não salvar o mapa.
PREPARAR = 'import unreal;s=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);[s.spawn_actor_from_class(k,unreal.Vector(0,0,-5000)).set_actor_label(n) for k,n in ((unreal.SkeletalMeshActor,"TesteOutfit"),(unreal.SkeletalMeshActor,"TesteHair"),(unreal.CameraActor,"TesteIndraCam"))]'
"""
import unreal

PASTA = "/Game/HypeWorld/Personagens/Indra/Modular/"
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
c = pc.get_controlled_pawn()
body = next(m for m in c.get_components_by_class(unreal.SkeletalMeshComponent) if m.get_name() == "Indra")
sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == w)
IA = {k: c.get_editor_property(k) for k in ("move_action", "sprint_action", "jump_action", "dash_action", "attack_action")}
st = {"t": 0.0, "log": [], "done": set(), "cam": None, "view": None, "original": unreal.load_asset("/Game/HypeWorld/Personagens/Indra/SK_Indra")}
unreal._teste_im = st
HEAD_BONE = "9ind00t0-head1_0243_150"


def log(msg):
    st["log"].append("[%.2fs] %s" % (st["t"], msg))


def once(k, t, fn):
    if st["t"] >= t and k not in st["done"]:
        st["done"].add(k)
        fn()


def shot(tag):
    log("foto " + tag)
    unreal.SystemLibrary.execute_console_command(w, "Shot")


def holder(label):
    return next(x for x in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.SkeletalMeshActor) if x.get_actor_label() == label)


def montar(modular, cabelo):
    """modular=True: cabeça careca + traje + (cabelo); False: SK_Indra original sozinho."""
    out, hair = holder("TesteOutfit"), holder("TesteHair")
    # o perfil salvo (personalização antiga) põe peças, material e escala no corpo: tira tudo para o teste
    for m in c.get_components_by_class(unreal.SkeletalMeshComponent):
        if m != body:
            m.set_visibility(False)
    body.set_relative_scale3d(unreal.Vector(1.06, 1.06, 1.06))
    if modular:
        body.set_skinned_asset_and_update(unreal.load_asset(PASTA + "SK_Indra_Head"))
        for h, nome in ((out, "SK_Outfit_Indra"), (hair, "SK_Hair_Indra")):
            comp = h.skeletal_mesh_component
            comp.set_mobility(unreal.ComponentMobility.MOVABLE)
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            h.attach_to_component(body, "", unreal.AttachmentRule.SNAP_TO_TARGET, unreal.AttachmentRule.SNAP_TO_TARGET,
                                  unreal.AttachmentRule.SNAP_TO_TARGET, False)
            comp.set_skinned_asset_and_update(unreal.load_asset(PASTA + nome))
            comp.set_leader_pose_component(body)
        out.set_actor_hidden_in_game(False)
        hair.set_actor_hidden_in_game(not cabelo)
    else:
        body.set_skinned_asset_and_update(st["original"])
        out.set_actor_hidden_in_game(True)
        hair.set_actor_hidden_in_game(True)
    # materiais do próprio asset (sem os da personalização antiga)
    for i, slot in enumerate(body.get_skinned_asset().get_editor_property("materials")):
        body.set_material(i, slot.get_editor_property("material_interface"))
    log("montado modular=%s cabelo=%s" % (modular, cabelo))


def camera():
    st["cam"] = next(x for x in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.CameraActor) if x.get_actor_label() == "TesteIndraCam")
    st["cam"].camera_component.set_editor_property("field_of_view", 30.0)
    st["cam"].camera_component.set_editor_property("constrain_aspect_ratio", False)
    pc.set_view_target_with_blend(st["cam"], 0.0)


def place_cam(dt):
    # 'view' = (para a frente do corpo, para a direita, para cima) em cm, olhando para a cabeça (ou corpo)
    if not st["cam"] or not st["view"]:
        return
    a, b, z, alvo_z = st["view"]
    head = body.get_socket_location(HEAD_BONE) + c.get_velocity() * dt
    alvo = head + unreal.Vector(0, 0, alvo_z)
    f, r = c.get_actor_forward_vector(), c.get_actor_right_vector()
    pos = alvo + f * a + r * b + unreal.Vector(0, 0, z)
    st["cam"].set_actor_location(pos, False, False)
    st["cam"].set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos, alvo), False)


def view(a, b, z, alvo_z=4, fov=30.0):
    st["view"] = (a, b, z, alvo_z)
    st["cam"].camera_component.set_editor_property("field_of_view", fov)


def move(x, y):
    sub.inject_input_vector_for_action(IA["move_action"], unreal.Vector(x, y, 0), [], [])


def press(k):
    sub.inject_input_vector_for_action(IA[k], unreal.Vector(1, 0, 0), [], [])


# vistas da cabeça: (frente, direita, cima, alvo acima da junta da cabeça)
VISTAS = [("frente", (110, 0, 0, 4)), ("lado", (0, 110, 0, 4)), ("costas", (-110, 0, 0, 4)), ("cima", (30, 0, 110, 6)),
          ("tres_quartos_costas", (-75, 75, 25, 4))]


def roteiro():
    eventos = [(0.3, "setup", lambda: (camera(), montar(True, False)))]
    t = 0.8
    for prefixo in ("careca", "cabelo", "original"):
        if prefixo == "cabelo":
            eventos.append((t, "m_" + prefixo, lambda: montar(True, True))); t += 0.4
        if prefixo == "original":
            eventos.append((t, "m_" + prefixo, lambda: montar(False, False))); t += 0.4
        for nome, v in VISTAS:
            eventos.append((t, "v_%s_%s" % (prefixo, nome), lambda v=v: view(*v))); t += 0.5
            eventos.append((t, "s_%s_%s" % (prefixo, nome), lambda p=prefixo, n=nome: shot("%s_%s" % (p, n)))); t += 0.2
    # careca de novo: close da emenda (têmpora, nuca por cima da gola) e o anel do pescoço sem o traje
    eventos.append((t, "m_careca2", lambda: montar(True, False))); t += 0.4
    for nome, v in (("emenda_tempora", (55, 45, 10, 8)), ("emenda_nuca", (-45, 25, 35, 4))):
        eventos.append((t, "v_" + nome, lambda v=v: view(*v, fov=30.0))); t += 0.5
        eventos.append((t, "s_" + nome, lambda n=nome: shot("careca_" + n))); t += 0.2
    eventos.append((t, "sem_traje", lambda: holder("TesteOutfit").set_actor_hidden_in_game(True))); t += 0.1
    for nome, v in (("anel_frente", (60, 15, -10, -14)), ("anel_costas", (-60, 15, -10, -14))):
        eventos.append((t, "v_" + nome, lambda v=v: view(*v, fov=30.0))); t += 0.5
        eventos.append((t, "s_" + nome, lambda n=nome: shot("careca_" + n))); t += 0.2
    eventos.append((t, "com_traje", lambda: holder("TesteOutfit").set_actor_hidden_in_game(False))); t += 0.1
    # 5 animações por último (o dash pode grudar numa parede e virar o corpo), corpo inteiro de lado
    eventos.append((t, "v_idle", lambda: view(0, 300, 25, -45, fov=48.0))); t += 0.6
    eventos.append((t, "s_idle", lambda: shot("careca_anim_idle"))); t += 0.2
    st["corre_ini"] = t + 0.1
    eventos.append((t, "sp", lambda: press("sprint_action"))); t += 1.3
    eventos.append((t, "s_corrida", lambda: shot("careca_anim_corrida"))); t += 0.4
    eventos.append((t, "j", lambda: press("jump_action"))); t += 0.35
    eventos.append((t, "s_pulo", lambda: shot("careca_anim_pulo"))); t += 0.9
    eventos.append((t, "d", lambda: press("dash_action"))); t += 0.25
    eventos.append((t, "s_dash", lambda: shot("careca_anim_dash"))); t += 0.4
    st["corre_fim"] = t
    t += 1.2
    eventos.append((t, "a", lambda: press("attack_action"))); t += 0.3
    eventos.append((t, "s_ataque", lambda: shot("careca_anim_ataque"))); t += 1.0
    eventos.append((t + 0.3, "fim", None))
    return eventos


EVENTOS = roteiro()


def tick(dt):
    try:
        st["t"] += dt
        for tt, k, fn in EVENTOS:
            if k == "fim" and st["t"] >= tt:
                log("fim")
                unreal.unregister_slate_post_tick_callback(st["h"])
                return
            if fn:
                once(k, tt, fn)
        if st.get("corre_ini", 99) <= st["t"] < st.get("corre_fim", 0):
            move(0, 1)
        place_cam(dt)
    except Exception as e:
        log("ERRO %r" % e)
        unreal.unregister_slate_post_tick_callback(st["h"])


st["h"] = unreal.register_slate_post_tick_callback(tick)
