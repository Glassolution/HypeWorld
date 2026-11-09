---
status: arquivado
atualizado: 2026-11-08
---
# Entrada inicial — histórico anterior à personalização (29/10/2026) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
### Entrada inicial — histórico anterior à personalização (29/10/2026)
Os limites desta seção descrevem a primeira versão; o criador acima a substitui.
- Animações da interface: troca entre entrada/criação/resumo com saída de 0,12 s e entrada de 0,22 s, deslocamento máximo 8 px; fundo permanece estável. Interações bloqueadas durante a troca. Hover/foco com realce suave, escala +2% e pressão -3%, interpolação independente do FPS; contorno tracejado suavizado. Seleção de vila/clã não reinicia a transição. Tudo no Tick do widget, sem assets adicionais.
- `UHypeCharacterSelectWidget` agora tem três estados: entrada sem personagem (botão + central, prévia escondida), criação (seleção existente de vila/clã) e resumo (personagem, vila, clã, jutsus equipados e Entrar no jogo). Voltar cancela a criação sem gravar. Confirmar salva e volta ao resumo; falha de gravação mantém a tela e mostra erro.
- Validação: HypeWorldEditor Mac Development compilado com `Result: Succeeded` (105,75 s). Jogo standalone aberto, log registrou mouse UI ativo. Validação visual/cliques e reabertura do save ainda pendentes: ferramenta de controle de tela retornou `Computer Use permissions are not granted`.
- `UHypeProfileSubsystem` carrega/salva `UHypeCharacterSave` no slot local `HypeWorld_Character`, usuário 0; apenas vila e clã. Aparência é a malha já configurada; nome, aparência personalizável, nível e progressão ainda não são parte desta criação. Jutsus exibidos vêm do `JutsuLoadout` atual do pawn, não de progresso salvo.
- `AHypeWorldPlayerController` abre a entrada no jogo standalone após nascer o pawn, uma vez por GameInstance (`bEntryShown`); não abre automaticamente no PIE nem ao conectar em rede. Usa o WBP existente com fallback C++. Nenhum mapa ou preset visual foi alterado. Perfil local de menu não aplica atributos nem poderes no servidor; a integração futura do perfil com multiplayer precisa validação do servidor.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
