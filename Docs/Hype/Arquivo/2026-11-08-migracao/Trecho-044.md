---
status: arquivado
atualizado: 2026-11-08
---
# PROMPT_NUVEM.md — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
CONTEXTO: Projeto HypeWorld (jogo ninja co-op, Unreal Engine 5.8.4, C++ + GAS). Você está no Claude Code na NUVEM, com o repositório Glassolution/HypeWorld. Leia o CLAUDE.md da raiz antes de tudo, principalmente a seção "TRABALHO PELA NUVEM": você não tem a Unreal, então escreve código e scripts, faz commit e push direto no main, e eu compilo e testo no meu Mac com o "Atualizar e Abrir". Responda em português, curto e simples (não sou programador), e termine sempre com: o que foi feito / prova (commit) / pendente / o que eu deveria entender.

O QUE EU QUERO FAZER POR AQUI: só coisas leves de interface: HUD, menus, tela de criação/customização de personagem.

ONDE PARAMOS (25/10/2026):
- Fluxo de menus: 1) tela de carregamento (PRONTA) → 2) criação de personagem com escolha de vila e clã (PRÓXIMA) → 3) entrada no jogo.
- Tela de carregamento: o jogo abre em /Game/Maps/Boot/L_Boot (AHypeBootGameMode) → WBP_LoadingScreen (UHypeLoadingScreenWidget, montado em C++) carrega em segundo plano o mapa da variável MapaSeguinte (hoje /Game/Maps/Tutorial/L_Tutorial_Forest) via UHypeLoadingSubsystem, com contagem 3-2-1 e fade. Código em Source/HypeWorld/UI/HypeLoading*, HypeBootGameMode*. Estilo: fundo preto, logo branca, Montserrat (/Game/Ninja/UI/Fontes/F_Montserrat, faces SemiBold/Bold/ExtraBold), minimalista.
- HUD (Source/HypeWorld/UI/NinjaHUDWidget): bloco de vida com as minhas imagens, 6 slots de jutsu vazios. Menu de combate em UI/NinjaCombatMenuWidget.
- Todas as telas são montadas em C++ (o WBP_ só herda a classe para os ajustes ficarem em Class Defaults). Siga esse padrão.

REGRAS QUE NÃO PODEM SER ESQUECIDAS:
- Não mexer em combate, movimento, animações, Input, mapas nem em .uasset existentes.
- Nunca apagar assets (mover para /Content/_ToDelete/ por script e me avisar).
- Asset novo = script em Tools/editor/ que pode rodar de novo + linha em Tools/pendentes.txt.
- Higgsfield só com a minha aprovação (custa créditos).
- Não invente função da engine: se não houver no projeto um exemplo de uso, avise que é suposição.

PRIMEIRA TAREFA: [escreva aqui. Sugestão: "Planeje a tela de criação de personagem com escolha de vila e clã, seguindo o estilo da tela de carregamento. Antes de escrever código, me mostre o plano (layout, telas, o que fica em C++, que assets precisam ser criados) e espere minha aprovação."]

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
