---
status: baseline-documentada
atualizado: 2026-11-08
---
# Mapa e otimização

> Preservar o visual aprovado do Tutorial; otimização exige comparação controlada.
> Os números abaixo são evidências anteriores, não uma nova medição desta sessão.

Mapa de produção/editor configurado: `/Game/Maps/Tutorial/L_Tutorial`; entrada do jogo: `/Game/Maps/Boot/L_Boot`. Confirmado pelas duas chaves de `UE/Config/DefaultEngine.ini`. `Lvl_Treino` em textos antigos não é o editor startup atual.

**Proteção visual:** `UE/Docs/Visual_Aprovado.md` continua sendo a fonte dos valores. Mudanças no visual protegido exigem autorização do usuário. Abrir só a seção afetada por busca/cabeçalho; não alterar o pack StyleHex. Preservar layout, vento/animações e aparência da grama/terreno.

**Baseline documentada em 26/10:** Alto a 1080p, resolução interna 100%, dinâmica desligada, TSR; TAA descartado. Lumen Irradiance Volume, sombras convencionais/PCSS e cortes pequenos aprovados. Para valores exatos, consultar seção “Base Alto aprovada” e somente o material/camada alterado.

**Última decisão de desempenho encontrada (05/11):** Lote 1, 11 malhas opacas, permanece **sem Nanite**, restaurado aos bytes originais. Mac caiu para `METAL_SM5`: mediu fallback, não Nanite real em PC. Variação de sessões iguais foi maior que a diferença A/B; não alegar ganho por reduzir triângulos. Fonte curta: `UE/Docs/Desempenho/Nanite_Lote1/README.md`, seções “O que os números dizem” e “O que falta”.

**Próximos trabalhos documentados, não iniciados aqui:** LODs/distâncias/foliage caso a caso; colisão simples com caminhada no mapa; quadro comparativo final. Conferir quem controla o editor e o estado corrente antes de retomá-los.

**Protocolo para comparar:** mesmas câmeras, resolução/preset e processo; editor recém-aberto, estabilizar, descartar aquecimento, várias rodadas A/B alternadas; registrar Frame/Game/Draw/GPU em ms, carga/shaders/foreground e aspecto visual. Não comparar sessão contaminada/segundo plano com sessão limpa nem usar FPS do editor como previsão de Windows. Colisão precisa teste andando, não só screenshot.

Para investigação detalhada, buscar um passe em `UE/Docs/VisualLab/Alto100` ou `UE/Docs/Visual_Auditoria_2026-10-26/Relatorio.md`; não abrir a pasta inteira de relatórios/logs.

Relacionadas: [[Ambiente]], [[Unreal]], [[Regras]], [[Erros]].
