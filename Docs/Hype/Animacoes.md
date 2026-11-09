---
status: procedimento
atualizado: 2026-11-08
---
# Animações e transições de selos

> Cada selo é independente e reutilizável; encadear poses não pode produzir saltos.
> Inspecionar entrada, contato, troca e saída; preservar os clipes aprovados.

- Partir do personagem e biblioteca aprovados, não de um estado parcial não salvo. Gerar apenas os clipes novos a 30 fps pelo fluxo de [[Importacao]].
- Criar entrada da postura normal, sustentação e transições necessárias. Duração configurável na ficha, não um único tempo fixo embutido para todos os selos.
- Extremos da transição reproduzem as poses de origem/destino; planejar pequena separação das mãos antes de girar/abrir dedos, depois fechar o contato. Não encadear diretamente clipes cujas poses iniciais diferem.
- Conferir lentamente mãos/dedos, corpo e mangas. Morfologia/curvas exclusivas de um selo devem estar zeradas nos demais; distinguir curva de material de morph target.
- Prova: frente do tronco inteiro, perfil, cima, três quartos e closes; entrada, selo segurado, meio das trocas, instante anterior ao disparo, disparo e retorno quando a tarefa incluir lançamento.
- Contagem BVH de interseções ajuda a localizar problemas, mas contatos aprovados existentes podem ter sobreposições. Comparar a baseline, não “consertar” silenciosamente um selo aprovado; câmera/roupa podem ocultar regiões.
- No Play medir o evento real e reabrir para conferir integração automática. `get_morph_target` pode não mostrar a curva avaliada pela animação; inspecionar curva e resultado visual antes de concluir que o corretivo está desligado.

**Revisão ativa 09/11:** as transições integradas foram reprovadas por rigidez. Diagnóstico completo e checkpoint em `UE/SourceAssets/Animations/Katon/TransicoesFluidas/WIP-2026-11-09/README.md`. Polo comum reduziu deslocamentos candidatos, mas entrada e retorno ainda usam um neutro assimétrico diferente do idle real; corrigir isso antes de importar. Candidatos não são clipes Unreal validados. Ritmo mais fluido aprovado está no Estado.json do checkpoint; a ficha em jogo ainda mantém o ritmo anterior.

Referência de scripts: `WS/Referencias/Katon/Cavalo/MCP_Blender/{gerar_clipes,conferir_transicoes,validar_play_unreal,validar_integracao_unreal}.py`. Adaptar a nova pasta, não sobrescrever os clipes desses scripts.

Contexto antigo de dash/Higgsfield/DeepMotion foi arquivado; não lê-lo para mexer num selo. Pesquisar “CONTEXTO_ANIMACOES” no manifesto do arquivo só se a tarefa for locomocão/acrobacia.

Relacionadas: [[Katon]], [[Rig]], [[Importacao]], [[Blender]].
