"""Gera o relatório de texto da Fase 1 a partir dos JSONs coletados (somente leitura)."""
import json
import os

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
SAVED = os.path.join(RAIZ, "Saved")
SAIDA = os.path.join(RAIZ, "RELATORIO_Fase1_Otimizacao_Mapa.txt")

L1 = json.load(open(os.path.join(SAVED, "auditoria_fase1_L_Tutorial.json"), encoding="utf-8"))
L2 = json.load(open(os.path.join(SAVED, "auditoria_fase1_L_CharacterCreation.json"), encoding="utf-8"))
BL = json.load(open(os.path.join(SAVED, "baseline_fase1.json"), encoding="utf-8"))
B2 = json.load(open(os.path.join(SAVED, "baseline_fase1_draws.json"), encoding="utf-8"))


def pkg(p):
    if ":" in p:
        return None
    i = p.rfind("/")
    nome = p[i + 1:]
    return p[:i + 1] + nome.split(".")[0] if "." in nome else p


def nome_de(p):
    return p.split("/")[-1].split(".")[0]


def custos(d):
    """[(custo_total, tri, instancias, metrica, path)] so para o que esta no mapa."""
    idx = {p: v for p, v in d["metricas"].items()}
    linhas = []
    for p, n in d["instancias_por_mesh"].items():
        k = pkg(p)
        v = idx.get(k) if k else None
        if not v:
            continue
        tri = v["tri_lod0"] or 0
        linhas.append((tri * n, tri, n, v, p))
    linhas.sort(reverse=True)
    return linhas


def flag_colisao(v):
    f = str(v.get("collision_flag") or "")
    if "COMPLEX" in f:
        return "COMPLEX_AS_SIMPLE (usa o mesh de render)"
    if "DEFAULT" in f:
        return "DEFAULT"
    if not f or f == "None":
        return "SEM_BODY_SETUP"
    return f


# chaves normalizadas (sem o sufixo ".Nome") para cruzar metricas x uso no mapa
def _norm_counter(c):
    out = {}
    for k, v in c.items():
        n = pkg(k) or k
        out[n] = out.get(n, 0) + v
    return out


INST_N = _norm_counter(L1["instancias_por_mesh"])
ATORES_N = _norm_counter(L1["atores_por_mesh"])


C1 = custos(L1)
C2 = custos(L2)
idx1 = {p: v for p, v in L1["metricas"].items()}


def alvo_nanite(v, inst):
    """Classificacao de candidato a Nanite (heuristica da Fase 2)."""
    tri = v["tri_lod0"] or 0
    blend = " ".join(v.get("blends") or [])
    if v.get("tem_translucido"):
        return "NAO (material translucido/aditivo)"
    if v.get("tem_mascara") and nome_de(v["path"]).startswith(("SM_Fol_", "SM_Tree_")):
        return "AVALIAR (foliage mascarada — caso a caso)"
    if v.get("raio_cm") is not None and v["raio_cm"] < 40 and inst < 20:
        return "NAO (pequeno e poucas instancias)"
    if tri < 300 and inst < 8:
        return "NAO (geometria muito simples)"
    if v.get("nanite"):
        return "JA USA NANITE"
    if tri * max(inst, 1) >= 50000 or inst >= 100 or tri >= 1500:
        return "SIM (candidato forte)"
    if tri >= 600 or inst >= 20:
        return "SIM (candidato medio)"
    return "AVALIAR (ganho pequeno)"


L = []
w = L.append

w("=" * 100)
w("RELATORIO FASE 1 — AUDITORIA SOMENTE LEITURA DE OTIMIZACAO DO MAPA (Hype World)")
w("Data: 05/11/2026    Engine: %s" % L1["engine"])
w("Escopo: SOMENTE assets de ambiente (niveis, static meshes, foliage, props,")
w("        materiais de cenario). NENHUM asset foi alterado nesta fase.")
w("Fora de escopo (nao tocados): personagens, rig, animacoes, blueprints de")
w("        personagem e logica do criador de personagem.")
w("=" * 100)
w("")
w("1. NIVEIS IDENTIFICADOS")
w("-" * 100)
w("   Nivel principal do jogo : Content/Maps/Tutorial/L_Tutorial.umap")
w("        (unico mapa de gameplay; vale estilo Zenkai, ponte, rio, trilha)")
w("        atores = %d | componentes StaticMesh distintos = %d | instancias somadas = %d"
  % (L1["atores_total"], len(L1["instancias_por_mesh"]), sum(L1["instancias_por_mesh"].values())))
w("   Nivel do criador        : Content/Maps/CharacterCreation/L_CharacterCreation.umap")
w("        atores = %d | componentes StaticMesh distintos = %d | instancias somadas = %d"
  % (L2["atores_total"], len(L2["instancias_por_mesh"]), sum(L2["instancias_por_mesh"].values())))
w("   Observacao: nao existe mapa de 'vila' separado. O fluxo do jogo e")
w("        L_Boot -> L_CharacterCreation -> L_Tutorial.")
w("   ALERTA (fora do escopo, so avisando): Source/HypeWorld/UI/HypeLoadingScreenWidget.cpp:35")
w("        aponta MapaSeguinte para 'L_Tutorial_Forest', que nao existe mais.")
w("   Os dois niveis usam o MESMO conjunto de assets de ambiente (127 static meshes).")
w("   O L_Tutorial tem alem disso placeholders: SM_Placeholder_Ponte10,")
w("   SM_Placeholder_PosteLanterna, SM_Placeholder_Barril, SM_Placeholder_Toco,")
w("   SM_Placeholder_BonecoTreino, SM_Placeholder_Portal.")
w("")

w("2. SUPORTE A NANITE NAS CONFIGURACOES DO PROJETO")
w("-" * 100)
w("   r.Nanite.ProjectEnabled = %s   (Config/DefaultEngine.ini:70)  -> LIGADO"
  % L1["nanite_project_enabled"])
w("   r.Nanite.Foliage        = %s   (Config/DefaultEngine.ini:192) -> foliage NAO usa Nanite"
  % L1["nanite_foliage"])
w("   Nao ha nada a ligar: o suporte ja esta ativo. So falta habilitar por mesh.")
w("")

n_total = len(L1["metricas"])
n_nanite = sum(1 for v in L1["metricas"].values() if v["nanite"])
n_lods = sum(1 for v in L1["metricas"].values() if (v["num_lods"] or 0) > 1)
n_nolod = sum(1 for v in L1["metricas"].values() if (v["num_lods"] or 1) <= 1)

w("3. SITUACAO ATUAL DOS MESHES DE AMBIENTE (%d auditados)" % n_total)
w("-" * 100)
w("   Com Nanite habilitado        : %d / %d" % (n_nanite, n_total))
w("   Sem Nanite                   : %d / %d" % (n_total - n_nanite, n_total))
w("   Com mais de 1 LOD (reducao)  : %d" % n_lods)
w("   Com apenas 1 LOD (sem reducao): %d  <- se nao ganharem Nanite, nao reduzem em distancia" % n_nolod)
w("   Quem ja usa Nanite (lista):")
w("      " + ", ".join(sorted(nome_de(p) for p, v in L1["metricas"].items() if v["nanite"])))
w("")

w("4. MESHES COM MAIS TRIANGULOS E INSTANCIAS (nivel principal L_Tutorial)")
w("-" * 100)
w("   Custo total na cena = triangulos LOD0 x numero de instancias.")
w("   %-34s %7s %7s %10s  %s" % ("MESH", "TRI_LOD0", "INST", "CUSTO", "NANITE/LODS"))
for tot, tri, n, v, p in C1[:20]:
    w("   %-34s %7d %7d %10d  nanite=%-5s lods=%s"
      % (nome_de(p)[:34], tri, n, tot, v["nanite"], v["num_lods"]))
w("")
w("   Mesmos dados para o criador (L_CharacterCreation) — top 10:")
for tot, tri, n, v, p in C2[:10]:
    w("   %-34s %7d %7d %10d  nanite=%-5s lods=%s"
      % (nome_de(p)[:34], tri, n, tot, v["nanite"], v["num_lods"]))
w("")
soma_c1 = sum(t for t, *_ in C1)
soma_c2 = sum(t for t, *_ in C2)
w("   Soma de 'custo' (tri x instancias) no L_Tutorial  : %d" % soma_c1)
w("   Soma de 'custo' (tri x instancias) no L_CharCreate: %d" % soma_c2)
w("   Nota: e um indicador relativo, nao o total renderizado (que na Fase 4 sai")
w("   da mesma forma nos 3 pontos de camera fixos antes/depois).")
w("")

w("5. CANDIDATOS A ATIVAR NANITE / NAO CANDIDATOS")
w("-" * 100)
w("   Criterio: SIM = mesh opaco denso ou com muitas instancias (rochas, paredoes,")
w("   predios, pontes, troncos, props grandes). NAO = simples, pequeno, translucido")
w("   ou com material que o Nanite nao suporta bem.")
w("")
w("   %-34s %7s %7s %10s  %s" % ("MESH", "TRI_LOD0", "INST", "CUSTO", "VEREDITO"))
fortes, medios, naos = [], [], []
for tot, tri, n, v, p in C1:
    ver = alvo_nanite(v, n)
    if ver.startswith("SIM (forte)"):
        fortes.append((nome_de(p), tri, n, tot, ver))
    elif ver.startswith("SIM"):
        medios.append((nome_de(p), tri, n, tot, ver))
    else:
        naos.append((nome_de(p), tri, n, tot, ver))
for nm, tri, n, tot, ver in fortes:
    w("   %-34s %7d %7d %10d  %s" % (nm[:34], tri, n, tot, ver))
for nm, tri, n, tot, ver in medios:
    w("   %-34s %7d %7d %10d  %s" % (nm[:34], tri, n, tot, ver))
w("")
w("   Nao recomendados neste lote (criterios de exclusao):")
vistos = set()
for nm, tri, n, tot, ver in naos:
    if nm in vistos:
        continue
    vistos.add(nm)
    w("   %-34s %7d %7d %10d  %s" % (nm[:34], tri, n, tot, ver))
w("")
trans = [nome_de(p) for p, v in L1["metricas"].items() if v.get("tem_translucido")]
masc = [nome_de(p) for p, v in L1["metricas"].items() if v.get("tem_mascara")]
w("   Materiais TRANSLUCIDOS (Nanite nao suporta bem — excluir):")
w("      " + (", ".join(sorted(set(trans))) or "nenhum"))
w("   Foliage com material MASCARADO (%d meshes) — Nanite suporta, mas exigem" % len(set(masc)))
w("   avaliacao caso a caso (r.Nanite.Foliage esta False no projeto):")
w("      " + ", ".join(sorted(set(masc))))
w("")

w("6. COLISAO DE CADA MESH")
w("-" * 100)
ctf3 = [(p, v) for p, v in L1["metricas"].items()
        if v.get("collision_flag") and "COMPLEX" in str(v["collision_flag"])]
sem_simples = [(p, v) for p, v in L1["metricas"].items() if not (v.get("collision_simples") or 0)]
com_simples = [(p, v) for p, v in L1["metricas"].items() if (v.get("collision_simples") or 0) > 0]
w("   Total auditado                       : %d" % n_total)
w("   Usando COMPLEX_AS_SIMPLE (render como colisao): %d" % len(ctf3))
w("   Sem NENHUMA primitiva simples        : %d" % len(sem_simples))
w("   Com primitivas simples (caixa/capsula/convexa/esfera): %d" % len(com_simples))
w("")
w("   6.1 Piores casos — usam a geometria de renderizacao como colisao E nao tem")
w("       primitiva simples nenhuma (o pior dos dois mundos):")
pior = [(p, v) for p, v in ctf3 if not (v.get("collision_simples") or 0)]
for p, v in sorted(pior, key=lambda x: -(x[1]["tri_lod0"] or 0)):
    uso = INST_N.get(p) or ATORES_N.get(p)
    situ = ("no mapa: %d" % uso) if uso else "FORA DO MAPA (asset nao colocado)"
    w("      %-34s tri=%6d  %-36s raio=%s cm"
      % (nome_de(p)[:34], v["tri_lod0"] or 0, situ, v["raio_cm"]))
w("      (FORA DO MAPA = existe o asset, com colisao complexa, mas nao esta")
w("       colocado no L_Tutorial — resolve-se mudando o asset ou nao importa.)")
w("")
w("   6.2 COMPLEX_AS_SIMPLE, mas JA tem primitiva simples (a flag pode ser")
w("       removida mantendo a colisao existente):")
ok = [(p, v) for p, v in ctf3 if (v.get("collision_simples") or 0) > 0]
for p, v in sorted(ok, key=lambda x: -(x[1]["tri_lod0"] or 0)):
    w("      %-34s tri=%6d  primitivas_simples=%d"
      % (nome_de(p)[:34], v["tri_lod0"] or 0, v["collision_simples"]))
w("")
w("   6.3 SEM colisao nenhuma (ja estao como queremos para decorativos):")
fol_sem = [(p, v) for p, v in sem_simples
           if nome_de(p).startswith(("SM_Fol_", "Petal_", "Clover", "Grass_"))]
for p, v in sorted(fol_sem, key=lambda x: -(x[1]["tri_lod0"] or 0))[:12]:
    w("      %-34s tri=%6d  flag=%s" % (nome_de(p)[:34], v["tri_lod0"] or 0,
                                        (v.get("collision_flag") or "None")))
w("      ... %d meshes de foliage decorativo ja sem colisao" % len(fol_sem))
w("")
w("   Poderiam ficar SEM colisao (decorativos, sem papel no gameplay):")
cand_sem = [(p, v) for p, v in L1["metricas"].items()
            if (v.get("raio_cm") or 0) < 70
            and nome_de(p).startswith(("SM_Fol_", "SM_Stone_", "Petal_", "Clover", "Pebble", "RockPath"))]
for p, v in sorted(cand_sem, key=lambda x: -(x[1]["tri_lod0"] or 0))[:15]:
    w("      %-34s tri=%6d  raio=%-6s  colisao_simples=%s"
      % (nome_de(p)[:34], v["tri_lod0"] or 0, v["raio_cm"], v.get("collision_simples")))
w("      ... %d no total (ver lista completa no JSON Saved/auditoria_fase1_L_Tutorial.json)" % len(cand_sem))
w("")
w("   Ja corretos: SM_Fol_* (foliage StyleHex) estao com CTF_USE_DEFAULT e")
w("   0 primitivas — ou seja, sem colisao de jogo.")
w("")

w("7. MATERIAIS E FOLIAGE")
w("-" * 100)
uso = L1["materiais_uso"]
w("   Materiais distintos usados pelo cenario: %d" % L1["materiais_distintos"])
un = sorted([(v, k) for k, v in uso.items() if v == 1])
w("   Usados por UM unico mesh (%d) — candidatos a consolidar, mas checar visual:" % len(un))
for v, k in un:
    w("      %-45s (1 mesh)" % k.split("/")[-1])
w("")
w("   Foliage ja usando instanciamento (ISMs):")
inst_fol = sorted([(v, k) for k, v in L1["instancias_por_mesh"].items()
                   if nome_de(k).startswith(("SM_Fol_", "SM_Stone_", "SM_Tree_", "SM_Zen_"))
                   and v > 5], reverse=True)
for v, k in inst_fol[:12]:
    w("      %-34s %d instancias" % (nome_de(k)[:34], v))
w("")
fol_nao_inst = []
for p, n in L1["instancias_por_mesh"].items():
    a = L1["atores_por_mesh"].get(p, 0)
    if nome_de(p).startswith(("SM_Fol_", "SM_Placeholder_", "SM_Zen_")) and a >= 2 and a == n:
        fol_nao_inst.append((nome_de(p), n))
w("   Foliage/props que NAO estao instanciados (1 componente por ator):")
if fol_nao_inst:
    for nm, n in sorted(fol_nao_inst, key=lambda x: -x[1]):
        w("      %-34s %d atores separados" % (nm[:34], n))
else:
    w("      Nenhum relevante — a folha ja usa FoliageInstancedStaticMeshComponent")
    w("      (54 componentes de foliage + 59 de grass instanciados no L_Tutorial).")
w("   Componentes por tipo no L_Tutorial:")
for k, v in sorted(L1["tipos_componente"].items(), key=lambda x: -x[1]):
    w("      %-45s %d" % (k, v))
w("")

w("8. LINHA DE BASE DE DESEMPENHO (3 pontos de camera fixos, L_Tutorial)")
w("-" * 100)
w("   Medicoes tomadas SEM alterar nada. Screenshots: Saved/Screenshots/MacEditor/")
w("   (cada ponto tem screenshot proprio).")
w("")
w("   %-18s %8s %12s %14s %14s" % ("PONTO", "FPS", "FRAME_MS", "TRI_VISIVEIS", "DRAWS_PRIM"))
ponto_info = {}
for i, pt in enumerate(BL["pontos"]):
    p = pt["ponto"]
    fps = (pt.get("fps_ticker") or {}).get("fps")
    ms = (pt.get("fps_ticker") or {}).get("ms")
    pr = pt.get("primitivas") or {}
    draws = pr.get("draw_calls_visiveis")
    w("   %-18s %8s %12s %14s %14s"
      % (p, fps, ms, pr.get("triangulos_visiveis"), draws))
    ponto_info[p] = pt
w("")
w("   CSV Profiler (media de %s frames por ponto):"
  % ((B2["pontos"][0].get("frame_csv") or {}).get("frames", "?")))
w("   %-18s %14s %14s %14s" % ("PONTO", "FRAME_MS_MEDIA", "FRAME_MS_MAX", "FPS_CALCULADO"))
for pt in B2["pontos"]:
    f = pt.get("frame_csv") or {}
    media = f.get("frame_time_ms_media")
    fps_calc = round(1000.0 / media, 1) if media else None
    w("   %-18s %14s %14s %14s"
      % (pt["ponto"], media, f.get("frame_time_ms_max"), fps_calc))
w("")
w("   Screenshots da baseline:")
for pt in BL["pontos"]:
    w("      Saved/Screenshots/MacEditor/%s" % pt["screenshot"])
w("")
w("   Limitacoes desta baseline (para registrar):")
w("   a) TRIANGULOS: medidos como soma de 'Triangles' das primitivas VISIVEIS")
w("      na camera, via comando DumpDetailedPrimitives (CSV em Saved/Profiling/Primitives/).")
w("      E a contagem de geometria apresentada pela cena, nao o que a GPU processa")
w("      em rasterizacao Nanite. Serve como antes/depois comparavel.")
w("   b) DRAW CALLS: o CSV Profiler tem a coluna RHI/DrawCalls, mas ela grava 0")
w("      nesta build (exige r.EmitDrawEvents, que so liga via")
w("      'r.RHISetGPUCaptureOptions 1'). Por isso a coluna 'DRAWS_PRIM' acima vem do")
w("      mesmo dump de primitivas (NumDraws somado). Para a Fase 4 repetir")
w("      'r.RHISetGPUCaptureOptions 1' antes de medir, para fechar o numero real.")
w("   c) O HUD de 'stat rhi'/'stat unit' nao aparece nos screenshots desta build")
w("      (take_high_res_screenshot nao captura UI e 'Shot' so funciona em PIE).")
w("   d) FPS medido no EDITOR, com viewport ativo — numeros piores que em jogo")
w("      (25-35 ms). Serve para comparar antes/depois nos MESMOS pontos, nao como")
w("      previsao de FPS final.")
w("")

w("9. RESUMO DOS MAIORES PROBLEMAS E GANHO ESPERADO")
w("-" * 100)
w("   PROBLEMA 1 — Foliage/grama e rochas carregam a cena sem Nanite:")
w("      As 3 gramas (SM_Fol_Grass_Clump_1/2/3) somam ~3.800 instancias e ~343 mil")
w("      triangulos de custo; as rochas SM_Stone_* somam ~906 instancias e ~567 mil.")
w("      NENHUM deles tem Nanite. As rochas sao candidatas FORTES (opacas, densas,")
w("      muitas instancias). A grama/arbusto/folha usa material MASCARADO, entao")
w("      entra na lista 'AVALIAR caso a caso' — Nanite suporta mascara, mas o")
w("      escopo pede para conferir visual e desempenho antes de manter.")
w("      GANHO ESPERADO: maior do projeto, por concentrar a maior parte dos")
w("      triangulos em objetos pequenos e repetidos.")
w("")
w("   PROBLEMA 2 — Colisao complexa onde nao deveria:")
w("      %d meshes usam COMPLEX_AS_SIMPLE. Os piores: SM_Placeholder_Ponte10 (6368 tri)," % len(ctf3))
w("      SM_Placeholder_Portal (3360), SM_Placeholder_BonecoTreino (3008),")
w("      SM_Placeholder_PosteLanterna (1680) e SM_Placeholder_Barril (1536) — todos")
w("      com 0 primitivas simples. A ponte inteira usa a malha de render como colisao.")
w("      GANHO ESPERADO: fisica/movimento mais baratos e estaveis; necessario para o")
w("      passo de colisao simples da Fase 3.")
w("")
w("   PROBLEMA 3 — Placeholders ainda no nivel principal:")
w("      Ponte, postes, barris, tocos, boneco de treino e portal sao placeholders de")
w("      baixa qualidade COM colisao complexa. GANHO ESPERADO: duplo (visual e")
w("      desempenho) se substituidos, mas isso e decisao de conteudo — fora da Fase 1.")
w("")
w("   PROBLEMA 4 — %d dos %d meshes nao tem Nanite e %d tem so 1 LOD" % (n_total - n_nanite, n_total, n_nolod))
w("      (nao ha reducao de detalhe em distancia). GANHO ESPERADO: com Nanite nos")
w("      densos + LODs/cull distance nos demais, cai o custo em cenas abertas como a")
w("      vista geral (P1) e a do treino (P3).")
w("")
w("   PROBLEMA 5 — r.Nanite.Foliage = False e foliage com mascara:")
w("      %d meshes de folha/grama usam material mascarado. GANHO ESPERADO: incerto;" % len(set(masc)))
w("      exige avaliar caso a caso com screenshot, como manda o escopo.")
w("")
w("   CENARIOS DE REFERENCIA DA BASELINE (nao otimizar):")
for pt in B2["pontos"]:
    f = pt.get("frame_csv") or {}
    w("      %-18s frame_ms=%s  (antes)" % (pt["ponto"], f.get("frame_time_ms_media")))
w("")
w("   ESTIMATIVA HONESTA: nao ha como prometer numero antes da Fase 2/3.")
w("   O que se pode dizer: a maior parte do custo esta em foliage e rochas que nao")
w("   usam Nanite, e em colisao complexa em placeholders. Sao os dois primeiros")
w("   lotes previstos, e os que devem dar o maior ganho medivel nos 3 pontos.")
w("")

w("10. O QUE FOI FEITO / PROVA / PENDENTE")
w("-" * 100)
w("   FEITO: auditoria somente leitura nos 2 niveis, 127 meshes de ambiente,")
w("          44 materiais, colisao de cada um, baseline em 3 pontos com screenshots.")
w("   PROVA: Saved/auditoria_fase1_L_Tutorial.json,")
w("          Saved/auditoria_fase1_L_CharacterCreation.json,")
w("          Saved/baseline_fase1.json, Saved/baseline_fase1_draws.json,")
w("          Saved/Profiling/CSV/Profile(20261105_053*.csv) e ..._054242.csv,")
w("          Saved/Profiling/Primitives/PrimitivesDetailed-*.csv,")
w("          Saved/Screenshots/MacEditor/Fase1_Baseline_*.png")
w("   PENDENTE: NENHUMA alteracao foi feita (fase somente leitura, como pedido).")
w("   ENTENDIDO: proximo passo e a Fase 2 (Nanite + triangulos), que SO comeca")
w("          depois da sua autorizacao explicita.")
w("")
w("   Arquivos de apoio criados (scripts Python, somente leitura):")
w("      Tools/editor/auditoria_otimizacao_fase1.py")
w("      Tools/editor/baseline_fase1.py / baseline_driver.py / baseline_ler.py")
w("      Tools/editor/baseline_driver2.py / baseline_ler2.py")
w("=" * 100)
w("FIM DA FASE 1 — AGUARDANDO AUTORIZACAO PARA A FASE 2")
w("=" * 100)

with open(SAIDA, "w", encoding="utf-8") as f:
    f.write("\n".join(L) + "\n")
print("GRAVADO:", SAIDA)
print("linhas:", len(L))
