"""Auditoria Fase 1 (SOMENTE LEITURA, 05/11/2026): Nanite, triângulos e colisão do cenário.

Não altera nenhum asset. Coleta dados e grava JSON em Saved/auditoria_fase1.json.

Rodar com o mapa do jogo aberto (L_Tutorial):
    python3 -I Tools/ue_exec.py -f Tools/editor/auditoria_otimizacao_fase1.py

Depois, abrir L_CharacterCreation e rodar de novo com:
    python3 -I Tools/ue_exec.py -f Tools/editor/auditoria_otimizacao_fase1.py -- mapa=L_CharacterCreation
(o argumento vira apenas o nome do campo de saída; a contagem usa o mapa ABERTO).
"""
import collections
import json
import os
import time

import unreal

# Caminhos de ambiente (fora de personagens/personalização).
PASTAS_AMBIENTE = (
    "/Game/HypeWorld/Environment/",
    "/Game/Maps/",
    "/Game/StyleHex_Studio/",
    "/Game/Fab/",
    "/PCG/",
    "/Game/LevelPrototyping/",
)
PASTAS_FORA = (
    "/Game/HypeWorld/Personagens/",
    "/Game/HypeWorld/Characters/",
    "/Game/Ninja/Characters/",
    "/Game/HypeWorld/Customization/",
)


def eh_ambiente(path):
    p = str(path)
    if any(p.startswith(x) for x in PASTAS_FORA):
        return False
    return any(p.startswith(x) for x in PASTAS_AMBIENTE)


def contar_primitivas_simples(mesh):
    """Conta primitivas de colisão simples (esferas, caixas, cápsulas, convexas)."""
    try:
        bs = mesh.get_editor_property("body_setup")
    except Exception:
        return None, None
    if bs is None:
        return None, None
    try:
        flag = str(bs.get_editor_property("collision_trace_flag"))
    except Exception:
        flag = None
    try:
        agg = bs.get_editor_property("AggGeom")
        total = 0
        for a in ("SphereElems", "BoxElems", "SphylElems", "ConvexElems",
                  "TaperedCapsuleElems"):
            try:
                total += len(agg.get_editor_property(a))
            except Exception:
                pass
        return flag, total
    except Exception:
        return flag, None


def blend_do_material(mat):
    """Blend mode do material (instância ou pai)."""
    if mat is None:
        return None
    try:
        return str(mat.get_blend_mode())
    except Exception:
        pass
    try:
        parent = mat.get_editor_property("parent")
        if parent:
            return str(parent.get_blend_mode())
    except Exception:
        pass
    return None


def metricas_mesh(path):
    try:
        mesh = unreal.EditorAssetLibrary.load_asset(path)
    except Exception:
        mesh = None
    if mesh is None:
        return None
    r = {"path": str(path)}
    try:
        r["tri_lod0"] = mesh.get_num_triangles(0)
    except Exception:
        r["tri_lod0"] = None
    try:
        r["num_lods"] = mesh.get_num_lods()
    except Exception:
        r["num_lods"] = None
    try:
        ns = mesh.get_editor_property("nanite_settings")
        r["nanite"] = bool(ns.enabled) if ns else None
    except Exception:
        r["nanite"] = None
    try:
        r["nanite_tri"] = mesh.get_num_nanite_triangles()
    except Exception:
        r["nanite_tri"] = None
    flag, simples = contar_primitivas_simples(mesh)
    r["collision_flag"] = flag
    r["collision_simples"] = simples
    try:
        mats = mesh.get_editor_property("static_materials")
        blends = []
        nomes = []
        for m in mats:
            mi = m.get("material_interface") if isinstance(m, dict) else getattr(m, "material_interface", None)
            nomes.append(str(mi.get_path_name()) if mi else None)
            blends.append(blend_do_material(mi))
        r["materiais"] = nomes
        r["blends"] = blends
        r["tem_translucido"] = any(b and ("TRANSLUCENT" in b or "ADDITIVE" in b) for b in blends)
        r["tem_mascara"] = any(b and "MASKED" in b for b in blends)
    except Exception:
        r["materiais"] = []
        r["blends"] = []
        r["tem_translucido"] = False
        r["tem_mascara"] = False
    try:
        b = mesh.get_bounds()
        r["raio_cm"] = round(float(b.sphere_radius), 1)
    except Exception:
        r["raio_cm"] = None
    return r


def main():
    saida = {}
    # 1) configuração do projeto
    saida["nanite_project_enabled"] = bool(
        unreal.SystemLibrary.get_console_variable_bool_value("r.Nanite.ProjectEnabled"))
    saida["nanite_foliage"] = bool(
        unreal.SystemLibrary.get_console_variable_bool_value("r.Nanite.Foliage"))
    saida["engine"] = unreal.SystemLibrary.get_engine_version()

    # 2) mapa aberto + instâncias por mesh
    world = unreal.EditorLevelLibrary.get_editor_world()
    saida["mapa_aberto"] = world.get_path_name() if world else None
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = sub.get_all_level_actors()
    inst = collections.Counter()
    tipos = collections.Counter()
    atores_por_mesh = collections.Counter()
    for a in actors:
        for comp in a.get_components_by_class(unreal.StaticMeshComponent):
            m = comp.get_editor_property("static_mesh")
            if not m:
                continue
            tipos[comp.get_class().get_name()] += 1
            try:
                n = comp.get_instance_count()
            except Exception:
                n = 1
            n = max(int(n), 1)
            inst[m.get_path_name()] += n
            atores_por_mesh[m.get_path_name()] += 1
    saida["atores_total"] = len(actors)
    saida["tipos_componente"] = dict(tipos)
    saida["instancias_por_mesh"] = dict(inst)
    saida["atores_por_mesh"] = dict(atores_por_mesh)

    # 3) todos os StaticMesh de ambiente do projeto (asset registry)
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    cpath = unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")
    datas = ar.get_assets_by_class(cpath, True)
    alvos = []
    for d in datas:
        p = str(d.package_name)
        if eh_ambiente(p):
            alvos.append(p)
    saida["meshes_ambiente_total"] = len(alvos)

    # 4) métricas (limite para não estourar o tempo de resposta do remote)
    metricas = {}
    for i, p in enumerate(alvos):
        m = metricas_mesh(p)
        if m:
            metricas[p] = m
        if i % 25 == 24:
            time.sleep(0.05)
    saida["metricas"] = metricas

    # 5) materiais únicos usados pelo cenário
    mats_unicos = collections.Counter()
    for m in metricas.values():
        for nome in m.get("materiais") or []:
            if nome:
                mats_unicos[nome] += 1
    saida["materiais_distintos"] = len(mats_unicos)
    saida["materiais_uso"] = dict(mats_unicos)

    # 6) meshes usados no mapa mas fora do filtro (para não perder nada)
    fora = [p for p in inst if p not in metricas]
    saida["fora_do_filtro_usados_no_mapa"] = fora

    out = os.path.join(os.path.expanduser("~"),
                       "Documents/Unreal Projects/HypeWorld/Saved/auditoria_fase1.json")
    with open(out, "w", encoding="utf-8") as f:
        json.dump(saida, f, ensure_ascii=False, indent=1)
    print("OK gravado:", out)
    print("mapa:", saida["mapa_aberto"])
    print("nanite_project:", saida["nanite_project_enabled"], "| nanite_foliage:", saida["nanite_foliage"])
    print("meshes ambiente:", saida["meshes_ambiente_total"], "| materiais distintos:", saida["materiais_distintos"])
    print("instancias somadas:", sum(inst.values()))


main()
