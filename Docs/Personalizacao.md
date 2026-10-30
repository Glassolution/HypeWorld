# Criador de personagem

Fluxo da primeira entrada: + → identidade → aparência → estilo → confirmação → jogo.
Ao abrir novamente com personagem salvo: escolha da vila e exibição do clã atribuído → resumo → entrar. O clã não é selecionável. O jogo atribui o clã ao criar o personagem; atualmente o catálogo contém somente Uchiha.
Primeiro nome: 2 a 24 caracteres, sem espaços ou sobrenome. O nome completo exibido é PrimeiroNome + Clã, por exemplo Felipe Uchiha; tom de pele: paleta e ajuste contínuo; altura visual: 160–200 cm. Voltar preserva o rascunho entre etapas. Cancelar restaura o personagem original; só a confirmação grava o perfil. Saves anteriores de vila/clã continuam válidos e mantêm o modelo anterior.

O corpo base existente `SK_Base_Body` foi validado contra os 286 ossos, bind pose e esqueleto do padrão Indra (`indra_character_standard.validate_mesh`). A aparência usa o material existente `MI_Base_Body`, parâmetro `BaseColor Tint`, em instância dinâmica. Nada altera os assets originais. A altura aplica uma escala uniforme sobre a escala visual padrão, em tempo de jogo; não altera o arquivo de referência, cápsula, velocidade, ossos, animações ou lógica de jutsu.

## Adicionar cabelos e roupas

1. Prepare cada malha no rig original do Indra, conforme `Docs/PadraoPersonagem/README.md`, incluindo bind pose e pesos. Não substitua apenas a propriedade Skeleton em uma malha incompatível. Preserve modelos anteriores.
2. No componente **Appearance** do `BP_ThirdPersonCharacter`, preencha **Hair Options** e **Outfit Options**. Cada item precisa de Id único e estável, Label e Mesh. Evite renomear IDs depois de usados em saves.
3. As peças são Skeletal Meshes modulares, alinhadas ao corpo e animadas por `SetLeaderPoseComponent`. Não use uma cópia completa de corpo/rosto como roupa. Materiais de cabelo devem expor `BaseColor Tint` para aceitar a paleta. O corpo base permanece por baixo; preparar roupas sem interseções é parte do trabalho de arte.
4. Sem itens, o menu informa que não há modelos disponíveis; a cor do cabelo permanece desativada sem cabelo selecionado. Não há formas de rosto/olhos, morph targets, tatuagens ou seleção de sexo implementados: faltam modelos preparados para essas opções.

## Salvamento e rede

`FHypeAppearance` contém primeiro nome, clã atribuído, altura, pele, cor de cabelo e IDs de cabelo/roupa. O catálogo em `HypeIdentity::AvailableClans()` contém somente Uchiha; a atribuição é feita ao criar e preservada na carga. O servidor valida o clã recebido e conserva o já atribuído durante a partida. O perfil local continua no slot `HypeWorld_Character`, usuário 0. IDs inexistentes ou de outro esqueleto são rejeitados no componente. Ao entrar numa partida, o proprietário envia o perfil local uma vez após possuir o pawn. O servidor limita altura/cores/nome e seleciona apenas modelos do catálogo do próprio servidor; replica os dados cosméticos para os demais. Não aplica atributos, habilidades ou progressão a partir do save. A prévia do criador é local e só é enviada ao confirmar.

## Validação

Testes `HypeWorld.Appearance`: saneamento de valores inválidos, serialização/deserialização completa do perfil e compatibilidade dos assets/replicação do componente. A inspeção visual da pele, alturas e roupas exige o jogo renderizado. Testar uma partida com dois jogadores continua necessário para validar visualmente a aparência remota.
