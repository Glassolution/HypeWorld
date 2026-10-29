# Bloco 1b — TAA

Alto, 1080p nativo, mesmo percurso, 3×30 s por opção. Nenhuma mudança global ou commit.

| Opção | FPS | 1% low | GPU |
|---|---:|---:|---:|
| Peso 0,04 — padrão | 33,35 | 30,62 | 29,38 ms |
| Peso 0,10 | 33,33 | 30,09 | 29,40 ms |
| Peso 0,20 | 33,35 | 30,87 | 29,39 ms |
| Peso 0,30 | 33,37 | 30,47 | 29,38 ms |
| Anti-ghosting Quality 3, peso 0,04 | 33,24 | 30,78 | 29,49 ms |

[Vídeos e prints lado a lado, incluindo isolamentos](C:/Users/lucas/Downloads/Hype%20World/HypeWorld/Docs/VisualLab/Alto100/AA1b/comparar.html).

**Origem:** o rastro persiste sem chakra, sem toon e no cabelo mesmo sem as seções da túnica. Os testes apontam a acumulação/rejeição de histórico do TAA como problema; nenhum desses elementos foi identificado como causa única. Não há evidência para alterar Output Velocity/Responsive AA dos materiais do chakra. A remoção completa da túnica também oculta partes do corpo porque compartilham seção.

**Recomendação:** ficar com TSR qualidade 1, conforme o fallback combinado. Peso 0,30 reduz o rastro, mas não elimina os contornos residuais; Quality 3 também não resolveu, e não considero validada a estabilidade da grama com os pesos altos. Nenhuma variante TAA testada atende ao conjunto do critério; o maior acréscimo de custo foi só 0,11 ms, sem opção acima de +0,5 ms.

As medições são em tempo real; vídeos nativos usam passos fixos de 1/30 s. Código da engine confirmou `r.TemporalAACurrentFrameWeight` e padrão 0,04. CSVs e limitações detalhadas em `Detalhes.md`.
