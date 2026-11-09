---
status: arquivado
atualizado: 2026-11-08
---
# Personalização de personagem (29/10/2026) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
### Personalização de personagem (29/10/2026)
- Validação desta correção: compilação `Result: Succeeded`; 5/5 testes automatizados aprovados após o pull. Prévia visual confirmou títulos separados e sublinhado alinhado; navegação por categorias testada, nome obrigatório e rascunho preservado. Evidências em `Docs/Personalizacao/Categorias/`.
- Categorias do criador corrigidas: `UHorizontalBox` com larguras naturais, espaço de 22 px entre botões e fonte 9, em vez de quatro posições fixas estreitas. Títulos e sublinhado pertencem ao mesmo botão; área de clique de 40 px. Categorias clicáveis preservam rascunho e bloqueiam avanço sem primeiro nome válido.
- GitHub integrado por fast-forward de `e37c3ab` até `1886b56` (commits `2561117` e `1886b56`), seguido de `git lfs pull origin main`. Catálogo/peças modulares, corpo/material/retarget atualizados e fontes de cabelos/olhos recebidos. 50 fontes verificadas pelo manifesto SHA-256, sem divergências nem ponteiros LFS pendentes. Trabalho local preservado (147 arquivos comparados; exceções previstas: UI/CLAUDE). Backup em `/private/tmp/hype-tabs/local-antes-github.tgz`. Sem push.
- O novo `NinjaCustomization` e `DT_CustomizationParts` vieram do GitHub; este passe atualiza e compila, mas não substitui o fluxo atual `HypeAppearanceComponent` por esse catálogo nem importa automaticamente os GLBs de fonte. Integrar peças novas ao menu é tarefa separada.

- Redesign validado: `Result: Succeeded`; prévias renderizadas de Identidade/Aparência/Estilo/Confirmar e testes de slider (175 cm/62%) e setas (174 cm/57%) aprovados. Evidências em `Docs/Personalizacao/Design/`.
- Redesign do criador seguindo a referência de 29/10: navegação compacta no topo, sem logo grande durante as etapas; textos à esquerda, coluna de 344 px; paleta em cápsulas com seleção; linhas escuras de 78 px com título, badge de valor, slider fino e setas funcionais. Footer com Voltar preto e Próximo translúcido quente/tracejado. Identidade, Estilo e confirmação compartilham alinhamentos e tipografia. Nenhuma alteração no mapa ou presets aprovados.
- Sliders sincronizam valor, thumb e seleção de cor; nuances contínuas preservadas ao reconstruir o menu. Na UE 5.8, `USlider::SetValue` dispara OnValueChanged: o callback do controle é removido/recolocado durante a sincronização. Check da paleta usa fonte composta padrão da Unreal, cujo fallback contém U+2713; Montserrat não contém esse glifo.

- Nome/clã validado: compilação `Result: Succeeded`; 5/5 testes `HypeWorld.Appearance` passaram, incluindo primeiro nome com acento, corte de sobrenome, clã indisponível e nome completo salvo. Evidência: `Docs/Personalizacao/nome_cla_validacao.json`.
- Regra de identidade atual: usuário escolhe somente o primeiro nome (2–24 caracteres, uma palavra; sobrenomes após espaços são cortados no primeiro espaço/tab/quebra de linha). Clã atribuído pelo jogo a partir do catálogo, atualmente só Uchiha; não há escolha manual de clã. Campo `FHypeAppearance.Clan` salvo e replicado, servidor impõe a atribuição. HUD e resumo compõem PrimeiroNome + Clã. Perfis existentes são normalizados na carga preservando a aparência. No retorno escolhe apenas vila e vê o clã recebido. Ao ampliar o catálogo de clãs, revisar persistência/validação da atribuição em rede antes de ativar novos.

- Correção de ordem validada: compilação `Result: Succeeded` (93,51 s), 4/4 testes aprovados incluindo `CharacterWithoutAffiliation`. Criação começa na identidade, confirmação entra direto no jogo e a seleção de vila/clã abre na sessão seguinte. Evidência: `Docs/Personalizacao/fluxo_validacao.json`.
- Correção de fluxo: o + leva diretamente à identidade (nome 2–24 caracteres) → aparência (pele, altura visual 160–200 cm) → estilo (catálogos de cabelo/roupa, vazios inicialmente) → confirmação. Só confirmar grava e entra diretamente no jogo; voltar preserva o rascunho e cancelar restaura a aparência anterior. A seleção de vila/clã abre apenas numa nova sessão com personagem salvo, sem atribuição automática durante a criação. Câmera aproxima suavemente na etapa Estilo.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
