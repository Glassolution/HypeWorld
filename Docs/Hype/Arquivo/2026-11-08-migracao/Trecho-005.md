---
status: arquivado
atualizado: 2026-11-08
---
# TRABALHO PELA NUVEM (Claude Code em claude.ai/code) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## TRABALHO PELA NUVEM (Claude Code em claude.ai/code)

Se você é o Claude rodando na nuvem, valem estas regras além das outras:
- **Você não tem a Unreal.** Não dá para compilar, abrir o editor, rodar `Tools/ue_exec.py`, criar ou editar `.uasset`/`.umap` nem tirar fotos. Os `.uasset` são do Git LFS e podem estar só como ponteiros: não abra, não mude, não apague.
- **O que você faz:** C++ em `Source/HypeWorld/` (HUD, menus e telas são montados em C++: ver `UI/NinjaHUDWidget`, `UI/NinjaCombatMenuWidget`, `UI/HypeLoadingScreenWidget`), `.ini` em `Config/` e scripts Python do editor em `Tools/editor/`.
- **C++ sem compilador:** copie padrões que já existem no projeto (includes, tipos, funções da engine já usadas). Se usar uma função da engine que o projeto ainda não usa, diga no commit e na resposta que é uma suposição a confirmar na compilação. Módulo novo → `HypeWorld.Build.cs`.
- **Assets novos** (material, textura, WBP_, mapa, fonte): escreva um script em `Tools/editor/` no estilo dos existentes (ex.: `criar_tela_carregamento.py`; Python da UE 5.8, `unreal.*`), que possa rodar de novo sem duplicar nada, e acrescente o caminho em `Tools/pendentes.txt`. Arquivos de entrada (PNG etc.) vão em `Referencias/`, e o script lê pelo caminho do Mac: `/Users/luisfelipexavier/Desktop/Hype World/<mesmo caminho do repositório>`.
- **Entrega:** commit com mensagem em português explicando o que mudou e push direto no `main` (o Felipe autorizou isso para o trabalho da nuvem). Depois o Felipe roda o "Atualizar e Abrir" e te conta o resultado (erros de compilação, fotos). Na nuvem, a prova de conclusão é o commit: diga claramente que não foi compilado nem testado.
- **Não mexer pela nuvem** (a não ser que o Felipe peça): combate, movimento, animações, Input, mapas e qualquer `.uasset` existente.



~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
