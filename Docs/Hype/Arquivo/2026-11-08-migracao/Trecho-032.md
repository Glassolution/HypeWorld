---
status: arquivado
atualizado: 2026-11-08
---
# Trajes modulares do Indra (03/11/2026) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
### Trajes modulares do Indra (03/11/2026)
- Processo: `Tools/customization/traje_itachi_piloto.py -- --personagem <Nome>` (Blender) → `Tools/editor/importar_traje_modular.py` → `Tools/editor/registrar_traje_catalogo.py` (DT_CustomizationParts, slot `Outfit`/Traje, `bOculto`; o personagem lê o DT ao carregar: reabrir o editor) → teste `Tools/editor/teste_traje.py`. Fotos: `Docs/Personalizacao/IndraModular/Trajes/`.
- Kakashi Anbu fica `bOculto` (igual ao Itachi). Costuras do rip: arestas abertas com par nas duas pontas (até 4 mm) passam a ter mesma posição e pesos, sem soldar (1.167 no Anbu): fechou fios e frestas. Teste do menu: `-TrocaTrajeShots` no jogo (chama a função das setas, registra as malhas no log; não grava). `teste_traje.py` com `_teste_traje_modo="congelado"` para o golpe parado no mesmo instante.
- Grupo 2 (03/11/2026): origem exportada da Unreal por `Tools/editor/exportar_origem_traje.py` para `SourceAssets/Customization/IndraModular/Origem/`. Sasuke aprovado e no menu. **Shisui reprovado após 2 tentativas:** cabeça na mesma malha do corpo; gola alta em cacos e pedaços brancos na camisa; importado, mas sem linha no catálogo.
- O anel real é de 36 vértices em 1,50 m (`anel_pescoco.json`), não 42 em 1,485.
- **Kakashi Anbu = mesmo corpo do Itachi Anbu:** GLBs com vértices, UVs, ossos e texturas iguais; os dois trajes só diferem na faixa do pescoço (1,46–1,49 m). Fios no peito e fresta no braço em poses extremas vêm dessa malha.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
