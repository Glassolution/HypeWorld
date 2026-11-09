---
status: arquivado
atualizado: 2026-11-08
---
# Rig, retarget e mãos: reutilizar a correção existente — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Rig, retarget e mãos: reutilizar a correção existente

- Os **161 ossos**, nomes, hierarquia, posições, orientações locais e bind pose da base são a referência. Não mover clavículas, ombros, pescoço ou falanges para fazer uma roupa caber; mover a malha da roupa. Não trocar apenas a propriedade Skeleton em uma malha incompatível.
- A animação do Manny passa por `Retarget Pose From Mesh` e pelo retargeter aprovado. Novas roupas sobre este skeleton usam o mesmo ABP/retarget; não aplicar clipes do Manny diretamente nem refazer a pose de retarget do corpo para cada roupa. Transformar uma roupa recebida em T para a pose de referência alvo não implica mudar o rig aprovado.
- Manter os pesos e a lógica de twist de braços, antebraços e coxas da referência. Não excluir esses grupos na transferência para a roupa.
- Preferir as **mãos já corrigidas**, com os cinco dedos separados de cada lado, polegar/metacarpos e punho alinhado. Não substituir pelas mãos IA da roupa nem refazer a rig de dedos sem necessidade explícita.
- Preservar dez morphs `Corr_thumb_l`, `Corr_index_l`, `Corr_middle_l`, `Corr_ring_l`, `Corr_pinky_l`, e seus equivalentes `_r`. Ao copiar shape keys, criar cada uma da `Basis` neutra, com `from_mix=False` e valor inicial zero. Não acumular a mistura dos corretivos anteriores: isso já gerou deltas gigantes no teste da etapa 11. O maior delta da referência atual é cerca de 0,8 cm.
- As faixas de antebraço seguem antebraço/twist/punho; não recebem pesos de dedos nem deltas de seus corretivos. Mangas seguem a transição de tronco/clavícula/braço; não recebem grupos de pescoço ou mão por proximidade equivocada.
- O relaxamento dos punhos/dedos é condicionado à locomoção em solo. Montagens, combate e selos precisam manter a amplitude integral. Preservar essa lógica em `SilentStanceAnimInstance.cpp`, incluindo os drivers de corretivos e a correção do olhar.
- Para luvas novas, transferir os pesos dedo a dedo e testar cada dedo isoladamente. Não espalhar pesos de um dedo para o vizinho nem fechar vãos ao reduzir triângulos.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
