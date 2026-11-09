---
status: cinco-selos-validados
atualizado: 2026-11-08
---
# Katon: sequência, ficha e disparo

> A ficha do Goukakyuu mantém durações independentes por selo e por transição.
> O Carneiro completa a ordem alvo, mas ainda não integra a sequência validada.

**Ordem alvo:** [[Selos/Serpente]] → [[Selos/Carneiro]] → [[Selos/Macaco]] → [[Selos/Javali]] → [[Selos/Cavalo]] → [[Selos/Tigre]]. Atualmente o Play usa a mesma ordem **sem Carneiro**.

**Marcador imutável nos lançamentos:** `NinjaJutsuRelease` em **0,375 s** no sopro Goukakyuu/Endan e **0,4375 s** na abertura Hosenka. São segundos exatos, mesmo sem quadro inteiro. Não regenerar/reamostrar lançamentos antigos; qualquer mistura inicial termina antes do marcador. Endan/Hosenka ficam fora do trabalho dos novos selos do Goukakyuu.

Ficha: `DA_Katon_Goukakyuu`, em `/Game/HypeWorld/Jutsus/Katon`; lista de selos/trechos (`Seals`/`Duration` na API usada pelos scripts). Ritmo atual, em segundos:

| Trecho | Duração |
|---|---:|
| Serpente | 0,19 |
| Serpente → Macaco | 0,14 |
| Macaco | 0,10 |
| Macaco → Javali | 0,18 |
| Javali | 0,10 |
| Javali → Cavalo | 0,18 |
| Cavalo | 0,10 |
| Cavalo → Tigre | 0,18 |
| Tigre | 0,24 |

Soma 1,41 s; não acelerar o ritmo para compensar selo adicionado sem pedido. Ao inserir Carneiro, manter editáveis os dois novos encaixes e a sustentação e **medir novamente**, sem tratar previsão como teste.

| Medição em Play | Início do jutsu → fogo |
|---|---:|
| Antes da mudança de ficha | 0,9258 s |
| Com Javali, quatro selos | 1,5125 s |
| Com Cavalo, cinco selos | **1,7875 s** (+0,2750 s) |
| Com Carneiro, seis selos | **Não medido** |

Comparação pedida para seis selos: informar diferença para 1,5125 s e, como controle recente, 1,7875 s. Medir nascimento real do projétil vs. início do cast, nas mesmas condições; tick explica pequena diferença para a soma teórica.

Fontes WS: `Referencias/Katon/Cavalo/MCP_Blender/Estado.json`, `Play Integracao/Resultado.json`, `Prova Cavalo.md`; prova de quatro selos em `Referencias/Katon/Javali/MCP_Blender/Prova Javali.md`. Biblioteca aprovada mais recente: JSON e PoseAsset produzidos na pasta Cavalo; ler apenas a entrada do selo necessário.

Relacionadas: [[Animacoes]], [[Importacao]], [[Unreal]], [[Estado]].
