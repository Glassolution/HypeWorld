---
status: referencia
atualizado: 2026-11-08
---
# Visão do jogo e pontos de entrada

> RPG ninja de ação em mundo aberto e co-op entre amigos; não é MMO.
> Esta nota aponta arquitetura e código, sem promover o plano antigo a estado atual.

- Multiplayer previsto: listen server, 4–8 jogadores; servidor autoritativo e predição onde suportada. Plataforma final pretendida: Windows; um build Mac não prova funcionamento em Windows.
- C++ com Blueprints para configuração; GAS para habilidades/atributos; Enhanced Input para controles; Animation Blueprint/Montages/Notifies para animação e eventos.
- Pontos de busca em UE: `Source/HypeWorld/Ninja`, `Combat`, `UI`; configurações em `Config`; scripts do editor em `Tools/editor`.
- Interface predominantemente montada em C++; não presumir que o HUD seja um WBP editável em vez de código. Pesquisar o widget específico.
- Movimento, combate e HUD têm históricos longos de substituições no antigo CLAUDE. Para uma tarefa nessas áreas, pesquisar a palavra/classe no arquivo e conferir o código atual; a biblioteca não declara esses sistemas novamente testados.
- Fonte histórica: blocos “ARQUITETURA”, “PLANO POR FASES” e “Combate” em [[Arquivo/2026-11-08-migracao/Indice]].

Relacionadas: [[00-Inicio]], [[Animacoes]], [[Customizacao]], [[Mapa]].
