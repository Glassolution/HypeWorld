---
status: interrompido-nao-validado
atualizado: 2026-11-08
---
# Selo do Carneiro: alvo e retomada

> É o selo pendente entre Serpente e Macaco; não foi integrado ou salvo na Unreal.
> A execução parou numa proteção do corretivo; dados parciais da sessão não são uma referência aprovada.

**Alvo:** partir da postura de braços do Tigre, mudando só o necessário. Mãos em bloco vertical estreito/prayer, bordas internas coladas do punho às pontas, palmas tocando, centro no zíper. Antebraços horizontais, cotovelos dobrados, punhos no estômago acima do cinto; mangas separadas estimadas em 0,85 mão.

Indicador **e médio** de cada mão esticados, retos e juntos, pilar de dois dedos por mão, cerca de 8° para dentro. Pontas com alturas ligeiramente diferentes, as centrais mais altas; mais altas no peito acima da mudança de cor da roupa. Anelar/mínimo dobrados e entrelaçados na parte inferior do bloco; polegares dobrados junto deles, uma ponta visível embaixo. Dorsos para câmera, punho sem inchaço.

**Sete verificações obrigatórias:** (1) braços/punhos acima do cinto; (2) mãos coladas sem vão; (3) direção para cima/~8°; (4) dois dedos retos juntos por mão/alturas diferentes; (5) anelar/mínimo entrelaçados abaixo; (6) polegares dobrados; (7) punho suave, sem atravessamento de mãos/roupa. Conferir frente do tronco com ambos cotovelos, perfil, cima e closes; marcar certo/errado em cada vista. Ainda **não passaram**; nenhum desvio foi aprovado nesta etapa.

**Última falha registrada:** `acabar_dual.py` interrompeu na proteção: vértice 3921 exigiu 1,5346 m no espaço original (limite 0,75 m). Não elevar o limite. A inversão da deformação ponderada pode amplificar deslocamentos; isso é uma hipótese a diagnosticar, não causa comprovada por esta auditoria. O novo corretivo ficou em zero durante a montagem parcial. Não exportar nem reutilizar o shape parcialmente escrito como validado.

**Arquivos WS:** `Referencias/Katon/Carneiro/MCP_Blender/Estado.json` e `ERRO_CORRETIVO.md` registram a interrupção. Scripts `pose_carneiro.py` e `acabar_dual.py` não são resultado aprovado. `.blend` de trabalho no mesmo diretório não recebeu o último estado; sessão pode conter dados voláteis. Antes de abrir/trocar, checar MCP/viewport e preservar o que houver, sem supor que a memória ainda exista.

**Backup:** `Backup/Cavalo validado como origem.blend`, hash em `Backup/Hashes.json`. Não tratar a cópia inicial desse arquivo como Carneiro pronto.

**Retomada proposta, não executada:** descartar apenas dados parciais novos e voltar à orientação mais próxima do Tigre, com ajustes pequenos/estáveis. Validar a silhueta antes do acabamento. Depois entrada normal, Serpente→Carneiro e Carneiro→Macaco, transições lentas, ficha de seis selos, Play e medição conforme [[Katon]]. Não alterar selos aprovados nem o ritmo existente; salvar na Unreal somente depois da validação.

Relacionadas: [[Estado]], [[Katon]], [[Selos/Tigre]], [[Blender]], [[Erros]].
