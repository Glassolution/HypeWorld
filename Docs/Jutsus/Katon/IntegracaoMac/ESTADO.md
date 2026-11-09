# Integração do Mac — 09/11/2026

Estado em jogo: Serpente, Carneiro, Macaco, Javali, Cavalo e Tigre. Durações independentes em Seals: entrada Serpente 0,18 s; cinco trocas 0,14 s; quatro sustentações 0,10 s; Tigre 0,24 s (soma 1,52 s). Biblioteca reutilizável em Animations/GoukakyuuCompleto/PO_Katon_SeisSelos. Manto Uchiha/posição 2 usa a cópia Clothes/GoukakyuuCompleto/SK_Uchiha_Robe; originais preservados. Goukakyuu usa Boca_Goukakyuu, socket da cópia.

VFX v2 recebidos em 862f8dd preservados na ficha. Endan/Hosenka mantêm seus lançamentos; marcadores SilentStance: sopro 0,375 s, Hosenka 0,4375 s. Baseline Play: 1,8958 s até criação do projétil antes do VFX v2; captura antes da revisão das transições registrou ~1,9131 s (amostragem). O VFX v2 tem carga de 0,32 s após o nascimento. Não confundir essas medidas com latência de tecla física ou tempo final de voo.

HUD: slots compactos de 54 px, ícones quadrados próprios, aro irregular, seleção e cooldown. Fontes dos três ícones em SourceAssets/UI/Hotbar; script Tools/editor/atualizar_hotbar_jutsu.py usa caminhos do projeto.

Validação existente: build Mac Development Succeeded; Play com seis selos, disparo, cancelamento GAS e recast; Endan três tiros e Hosenka órbita. Usuário confirmou fogo visível após o pull. Ainda há defeitos de contato/mangas, e as transições foram reprovadas por rigidez; não declarar qualidade final aprovada. Teste de rede caiu antes do jutsu em ServerSetBattleToggle; replicação segue pendente.

Revisão em andamento: SourceAssets/Animations/Katon/TransicoesFluidas/WIP-2026-11-09. Ritmo mais fluido aprovado para a próxima versão, ainda sem alterar a ficha em jogo. Ler o README do checkpoint antes de usar candidatos. Biblioteca de conhecimento portátil: Docs/Hype/00-Inicio.md.
