"""Vegetação da mini floresta do tutorial por PCG (25/10/2026). Rodar depois de criar_floresta_tutorial.py, com o
L_Tutorial_Forest aberto e o editor fora do Play.
Grafo /Game/Maps/Tutorial/PCG_Floresta, um ramo por tipo. Cada ramo lê o landscape e usa uma camada pintada como máscara:
- Árvores (camada Mata: fora da clareira, dos caminhos e da borda), sem encostar umas nas outras (Self Pruning)
- Grama densa e flores (camada Vegetacao: tudo menos caminhos), somem de longe (cull distance)
- Arbustos (camada Borda: perto das pedras)
- Pedrinhas (camada Vegetacao)
DENSIDADE: mude points_per_squared_meter do Surface Sampler de cada ramo no grafo (ou em DENS abaixo e rode de novo).
Pintar as camadas Mata/Borda/Vegetacao no modo Landscape também muda onde as coisas nascem (depois: Generate no PCG Volume).
"""
import unreal

GRAFO = "/Game/Maps/Tutorial/PCG_Floresta"
MESH = "/Game/Ninja/Ambiente/Floresta/Meshes/"
#        nome      camada       pontos/m²  extensão  escala       malhas (peso)                                               cull (cm)   colisão  prune
RAMOS = [("Arvores", "Mata", 0.02, 260.0, (0.75, 1.15), [("SM_Arvore_Grande_A", 3), ("SM_Arvore_Grande_B", 2), ("SM_Arvore_Media_A", 3)], None, True, True),
         ("Grama", "Vegetacao", 1.6, 35.0, (0.8, 1.45), [("SM_Grama_Tufo_A", 3), ("SM_Grama_Tufo_B", 2)], (3000.0, 4500.0), False, False),
         ("Flores", "Vegetacao", 0.12, 30.0, (1.6, 2.4), [("SM_Flor_Rosa", 3), ("SM_Flor_Branca", 2)], (2500.0, 4000.0), False, False),
         ("Arbustos", "Borda", 0.09, 110.0, (0.8, 1.5), [("SM_Arbusto_A", 1)], (7000.0, 9000.0), False, True),
         ("Pedrinhas", "Vegetacao", 0.025, 40.0, (0.6, 2.2), [("SM_Pedra_Pequena_A", 1), ("SM_Pedra_Pequena_B", 1)], (4000.0, 6000.0), False, False)]

tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
if eal.does_asset_exist(GRAFO):
    graph = unreal.load_asset(GRAFO)
    for n in list(graph.get_editor_property("nodes")):
        graph.remove_node(n)
else:
    graph = tools.create_asset("PCG_Floresta", "/Game/Maps/Tutorial", unreal.PCGGraph, unreal.PCGGraphFactory())
entrada = graph.get_input_node()


def pino(no, saida=True):
    return [str(p.get_editor_property("properties").get_editor_property("label")) for p in no.get_editor_property("output_pins" if saida else "input_pins")]


def no(cls):
    n, s = graph.add_node_of_type(cls)
    return n, s


print("entrada:", pino(entrada))
for i, (nome, camada, dens, ext, esc, malhas, cull, colisao, prune) in enumerate(RAMOS):
    land, s = no(unreal.PCGGetLandscapeSettings)
    samp, s = no(unreal.PCGSurfaceSamplerSettings)
    s.set_editor_property("points_per_squared_meter", dens)
    s.set_editor_property("point_extents", unreal.Vector(ext, ext, ext))
    s.set_editor_property("looseness", 1.0)
    s.set_editor_property("seed", 1000 + i * 77)
    filt, s = no(unreal.PCGAttributeFilteringSettings)
    sel = s.get_editor_property("target_attribute")
    sel.import_text("PCGBegin(%s)PCGEnd" % camada)   # set_attribute_name não grava pelo Python
    s.set_editor_property("target_attribute", sel)
    s.set_editor_property("operator", unreal.PCGAttributeFilterOperator.GREATER)
    s.set_editor_property("use_constant_threshold", True)
    k = s.get_editor_property("attribute_types")
    k.set_editor_property("double_value", 0.5)
    s.set_editor_property("attribute_types", k)
    tr, s = no(unreal.PCGTransformPointsSettings)
    s.set_editor_property("rotation_min", unreal.Rotator(pitch=0, yaw=0, roll=0))
    s.set_editor_property("rotation_max", unreal.Rotator(pitch=0, yaw=360, roll=0))
    s.set_editor_property("absolute_rotation", True)       # em pé (não deita com o chão) e girado ao acaso
    s.set_editor_property("scale_min", unreal.Vector(esc[0], esc[0], esc[0]))
    s.set_editor_property("scale_max", unreal.Vector(esc[1], esc[1], esc[1]))
    s.set_editor_property("seed", 2000 + i * 31)
    spawn, s = no(unreal.PCGStaticMeshSpawnerSettings)
    msel = s.get_editor_property("mesh_selector_parameters")
    entradas = []
    for m, peso in malhas:
        e = unreal.PCGMeshSelectorWeightedEntry()
        e.set_editor_property("weight", peso)
        d = e.get_editor_property("descriptor")
        d.set_editor_property("static_mesh", unreal.load_asset(MESH + m))
        if cull:
            d.set_editor_property("instance_start_cull_distance", int(cull[0]))
            d.set_editor_property("instance_end_cull_distance", int(cull[1]))
        corpo = d.get_editor_property("body_instance")   # árvores batem (dá para subir); o resto atravessa
        corpo.set_editor_property("collision_enabled", unreal.CollisionEnabled.QUERY_AND_PHYSICS if colisao else unreal.CollisionEnabled.NO_COLLISION)
        corpo.set_editor_property("collision_profile_name", "BlockAll" if colisao else "NoCollision")
        d.set_editor_property("body_instance", corpo)
        if nome == "Grama":
            d.set_editor_property("cast_shadow", False)      # grama sem sombra: bem mais leve
        e.set_editor_property("descriptor", d)
        entradas.append(e)
    msel.set_editor_property("mesh_entries", entradas)
    if i == 0:
        print("pinos landscape", pino(land), "| sampler in", pino(samp, False), "out", pino(samp), "| filtro out", pino(filt), "| spawner in", pino(spawn, False))
    graph.add_edge(land, "Out", samp, "Surface")
    graph.add_edge(entrada, pino(entrada)[0], samp, "Bounding Shape")
    graph.add_edge(samp, "Out", filt, "In")
    graph.add_edge(filt, "InsideFilter", tr, "In")
    ultimo = tr
    if prune:
        pr, s = no(unreal.PCGSelfPruningSettings)
        graph.add_edge(tr, "Out", pr, "In")
        ultimo = pr
    graph.add_edge(ultimo, "Out", spawn, "In")
print("arestas:", len(graph.get_all_edges()) if hasattr(graph, "get_all_edges") else "?")
eal.save_loaded_asset(graph, False)

# Volume do PCG cobrindo o oval (130 x 90 m)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "PCG_Floresta":
        eas.destroy_actor(a)
vol = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PCGVolume, unreal.Vector(0, 0, 500), unreal.Rotator(0, 0, 0))
vol.set_actor_label("PCG_Floresta")
vol.set_folder_path("Vegetacao")
o, ext = vol.get_actor_bounds(False)
vol.set_actor_scale3d(unreal.Vector(65.0, 45.0, 30.0))   # pincel de 200 cm -> 130 x 90 x 60 m
o, ext = vol.get_actor_bounds(False)
print("volume:", o, ext)
comp = vol.get_component_by_class(unreal.PCGComponent)
comp.set_graph(graph)
comp.set_editor_property("generation_trigger", unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)   # gera no editor e fica salvo no mapa (não refaz ao abrir o jogo)
comp.set_editor_property("seed", 42)
comp.generate_local(True)
print("pedido de geração")
