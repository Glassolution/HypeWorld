---
status: seis-selos-singleplayer-rede-pendente
atualizado: 2026-11-09
---
# Katon: sequência, ficha e disparo

> Goukakyuu completo está montado e salvo; seis selos passaram no Play de um jogador.
> A prova final de rede foi interrompida por crash antes do jutsu; não declarar a tarefa inteira concluída.

Ordem na ficha: [[Selos/Serpente]] → [[Selos/Carneiro]] → [[Selos/Macaco]] → [[Selos/Javali]] → [[Selos/Cavalo]] → [[Selos/Tigre]]. `DA_Katon_Goukakyuu` mantém cada duração editável em `Seals`, incluindo transições.

| Trecho | Segundos |
|---|---:|
| Serpente com entrada | 0,18 |
| Cada uma das cinco transições | 0,14 |
| Carneiro / Macaco / Javali / Cavalo segurados | 0,10 cada |
| Tigre segurado | 0,24 |
| Soma de selos/trocas | 1,52 |

**Marcadores antigos preservados:** `NinjaJutsuRelease` em 0,375 s no sopro Goukakyuu/Endan e 0,4375 s na abertura Hosenka, segundos exatos. Não regenerar/reamostrar esses clipes. Sopro existente começa no Tigre e termina na postura de entrada. Só Goukakyuu usa novo socket `Boca_Goukakyuu` na cópia da malha; Endan/Hosenka continuam como estavam e funcionaram no Play (três tiros / órbita), hashes intactos.

| Medição em Play | Início da ativação → fogo |
|---|---:|
| Um selo, baseline | 0,9258 s |
| Quatro selos | 1,5125 s |
| Cinco selos | 1,7875 s |
| Seis selos, antes do VFX v2 do GitHub | **1,8958 s** |

Diferença seis vs. quatro: +0,3833 s; vs. cinco: +0,1083 s. Medido pelo nascimento real do projétil, tempo de jogo a 0,35x. Teste normal 60 fps: 1,9000 s, diferença de amostragem. Não é uma medição de latência física do teclado. Ritmo autorizado pelo usuário nesta tarefa; alternativa mais rápida só proposta no relatório, não aplicada.

**Testado:** 17 fases/63 capturas; extremos dos cinco encaixes iguais às poses; spam pelo handler recusado sem fogo extra; movimento não cancela; cancelamento GAS aos 0,75 s impediu fogo; recast após cooldown de 8 s funcionou, sem pose presa. Não houve teste de teclas físicas nem dano real. Há irregularidades/vãos nos punhos/mangas, mais evidentes em Javali e Macaco/Cavalo; não prometer malha totalmente limpa. Cintura não mostrou os rasgos antigos nas vistas examinadas.

**Bloqueio:** Play ListenServer + cliente conectou, mas caiu com SIGBUS/`ServerSetBattleToggle` repetido ao ativar postura no cliente, antes do jutsu. Nenhum código de rede alterado; sem nova tentativa após erro. Editores fechados e preferências de Play restauradas para um jogador. Carregamento automático da nova malha aparece no log pós-build; a prova visual final automática e a replicação da sequência continuam pendentes.

**Pull 09/11:** `main` em `862f8dd`, VFX v2 recebidos e build Mac aprovado. Ficha combina VFX novos com os seis selos/socket locais; hold do efeito recebido 0,32 s. O timing após esse pull **não foi medido**: 1,8958 s é da versão anterior. Provas e backup em `WS/Backups/GitHub-2026-11-09/Estado.json`; tutorial recebido em `UE/Docs/Jutsus/Katon/GokakyuV2/TUTORIAL_CODEX_MAC.md`. Não rodar scripts de geração só para usar os assets recebidos.

**Relato pós-pull:** usuário viu selos/chakra sem fogo. Nos testes de 09/11 o fogo apareceu em `Shot` do viewport real, inclusive em velocidade normal; nenhum asset/código alterado. Usuário confirmou: “Agora aparece”. Causa inicial não isolada; não registrar como correção de código. Prova: `WS/Referencias/Katon/Regressao pos-pull-2026-11-09/Estado.json`.

**Fontes:** estado/provas da integração em `WS/Referencias/Katon/Goukakyuu completo/Integracao-2026-11-09/`. Biblioteca: `Animations/GoukakyuuCompleto/PO_Katon_SeisSelos` no SilentStancePreview. Revisão das transições em [[Animacoes]]: usuário aprovou 1,86 s de selos (notify previsto 2,235 s); **ainda não aplicada à ficha**. Checkpoint e estado publicados em `UE/SourceAssets/Animations/Katon/TransicoesFluidas/WIP-2026-11-09/`.

Relacionadas: [[Animacoes]], [[Importacao]], [[Unreal]], [[Estado]], [[Customizacao]].
