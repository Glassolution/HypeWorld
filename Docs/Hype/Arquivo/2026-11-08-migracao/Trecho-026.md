---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
- [x] Blender 5.2.3 instalado (24/10/2026, `brew install --cask blender`; app em `/Applications/Blender.app`, comando `blender`). Uso sem MCP, em segundo plano por script: `blender -b --factory-startup --python script.py -- <args>` (testado: importa FBX do Mixamo, exporta FBX, tem glTF). Papel: limpar/corrigir animações geradas (tremidos, poses erradas, pés escorregando, emendas, tempo) antes de ir para a Unreal. Evitar Blender + Unreal pesados abertos juntos (8 GB).
- [x] Conexão com o editor via Python Remote Execution testada
- [ ] Blender MCP conectado
- [x] Higgsfield MCP conectado (24/10/2026: `claude mcp add --transport http --scope user higgsfield https://mcp.higgsfield.ai/mcp` + login do Felipe via /mcp; `claude mcp list` = ✔ Connected). Ferramentas só aparecem em sessões abertas depois disso. 3D (pesquisado 24/10/2026, ainda não testado): image_to_3d / multi_image_to_3d (Meshy), tripo_3d (texto→3D), sam_3_3d, 3d_rigging (esqueleto humanoide automático; pedir personagem em A-pose/T-pose), animação = biblioteca FIXA de ~678 clipes (animation_actions busca e mostra GIF; escolhidos por animation_action_id) — NÃO cria animação nova a partir de texto/vídeo. Saída GLB (FBX não mencionado). Custo: get_cost estima antes; malha é o mais barato, textura/rig/animação somam.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
