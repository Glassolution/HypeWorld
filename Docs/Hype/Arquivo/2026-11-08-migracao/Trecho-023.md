---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
  - **Estilos de luta / variedade / menu (24/10/2026):** `UNinjaFightingStyle` (DataAsset em `/Game/Ninja/Characters/Estilos/DA_Estilo_*`, código `Source/HypeWorld/Combat/NinjaFightingStyle.h`): base de luta (BattleIdle, misturada no modo de batalha), `Combos` (cada um uma sequência de golpes), `Finishers` (F), `SprintAttacks`, `AirAttacks`, `RequiredWeapon`. O personagem tem `FightingStyles` (Punho de Chakra, Taijutsu Ninja, Punho Blindado + Katana/Garras/Lança/Kunai trancados — `CanUseFightingStyle` exige a arma em `EquippedWeapon` e combos; ainda não existe arma no jogo) e `FightingStyleIndex` replicado (`SelectFightingStyle` + RPC). Ids de golpe com estilo: leve = combo×16 + posição, forte 100+i, correndo 200+i, ar 300+i; o cliente sorteia (`PickVariant`, nunca repete o anterior) e manda o id no evento. Criados por `Tools/editor/criar_estilos_luta.py` (golpes medidos por `Tools/editor/analisar_golpes_pack.py` → `Fab/anime_ninja_golpes.json`; montagens `/Game/Ninja/Characters/Animations/ANAP/Golpes/AM_G_*`, sufixo `_Combo` = com janela de combo — golpe solto e golpe de combo NÃO podem dividir a montagem; atordoamento calculado para a sequência encaixar). `ComboInputWindow` 0,8. **Dash por direção** (`DashVariants`: Front/Back/Left/Right/Air, montagens em `.../ANAP/Dash/`): tecla de lado/trás em relação à câmera (> `DashStrafeAngle` 50°) no chão = corpo de frente para a câmera e dash de lado (passo lateral DashStep_Left/Right ou desvio Dodge_Run) / de costas (mortal DashStep_Behind); no ar = Aerial_Dash. Variação = contador de dashes embaralhado (igual em todas as máquinas). Seção "End" das montagens de dash é chamada no fim do dash (se ainda não estiver nela). **Menu de combate** (`Source/HypeWorld/UI/NinjaCombatMenuWidget`, Tab / botão Menu do controle, `IA_CombatMenu`): réplica da referência (categorias Naturezas/Kekkei Genkai/Clã/Taijutsu/Jutsus Especiais/Especializações; Favoritos com Edição/Modo Normal e espaços de jutsu); Taijutsu = escolher pose de batalha; abre com mouse e câmera parada, esconde o HUD; botões descobrem o clique por IsHovered. Teste: `Tools/editor/teste_estilos.py`.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
