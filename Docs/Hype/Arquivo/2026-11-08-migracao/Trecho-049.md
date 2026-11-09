---
status: arquivado
atualizado: 2026-11-08
---
# Fluxo para cada nova roupa — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Fluxo para cada nova roupa

1. Criar pasta de trabalho própria, manter o arquivo original intacto e salvar cópia do Blender antes de editar geometria ou pesos. Registrar assets, slots, morphs e hashes antes da importação. Conferir as referências congeladas com `conferir_referencias.py`.
2. Inspecionar peças, UVs, mapas, unidades, orientação, pose e rig do modelo recebido. Começar da fonte original; não acumular adaptações de uma versão deformada. Normalizar unidades e usar escala proporcional como primeiro ajuste.
3. Usar a anatomia, as articulações e a silhueta do Silent Stance como medida. Preservar cabeça, pescoço, mãos e regiões expostas aprovadas. Retirar as regiões cobertas somente na cópia do visual da roupa, escondendo as emendas sob barras, gola ou punhos. Não duplicar corpos ou camadas de roupa que aumentem a espessura.
4. Medir peito, cintura, quadril, meio da coxa, barra da bermuda, joelho, canela, tornozelo e antebraços. A base contém camiseta e short; distinguir superfície vestida de pele real ao medir folgas. Não usar um render de conceito com outra anatomia para alterar as proporções da base.
5. Se for a mesma categoria de camiseta justa, reutilizar a superfície superior aprovada foi o método que deu certo. Se o novo design tiver outro corte, preservar esse design e ajustar a peça localmente na cópia; não forçar todo tecido a ser a camiseta Ninja nem apagar detalhes sem necessidade. Bermuda justa acompanha a coxa, sem abertura artificial em A; faixas e sandálias não devem engrossar o membro.
6. Vincular à cópia exata do rig aprovado. Transferir pesos da região anatômica correspondente da base por projeção/interpolação da superfície, revisar bordas e normalizar, com até oito influências. Separar corpo, tecido, bolsa e peças rígidas quando necessário para não misturar pesos de regiões vizinhas.
7. Preservar UVs e aparência do modelo novo onde funcionam. Usar os materiais e parâmetros aprovados como referência de qualidade, não como obrigação de pintar qualquer nova roupa de azul. Exportar/importar para um novo asset e ID, mantendo a roupa aprovada disponível.
8. Validar no Blender e no jogo antes de entregar. Corrigir autonomamente o que aparecer nas animações reais, salvar as comparações e deixar o criador utilizável. Perguntar apenas quando faltar uma decisão de design relevante que não possa ser inferida do pedido e das referências.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
