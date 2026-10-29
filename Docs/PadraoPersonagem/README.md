# Padrão de personagem: Indra

A referência permanente está em `Content/Python/indra_character_standard.json`: esqueleto, bind pose por osso, tamanho, transformação, cápsula, animações, velocidades e corpo que dirige a locomoção. O padrão é capturado do Indra original; não é redefinido ao importar outro modelo.

## Novo modelo com o mesmo esqueleto e animações

1. Importe `Indra_Referencia.fbx` no Blender/Maya como referência. Preserve nomes, hierarquia, orientação, comprimentos dos ossos, bind pose e unidade em centímetros.
2. Em uma cópia do novo modelo, ajuste a geometria ao corpo e às articulações do Indra. Vincule a malha ao rig e transfira/revise os pesos, especialmente ombros, mãos, roupa e cabeça. Não modifique o rig para encaixá-lo na roupa.
3. Exporte somente a nova malha com o rig original, sem animações, sem ossos adicionais e sem escalas não uniformes. Importe no Unreal selecionando `SKEL_Indra`, sem atualizar sua pose de referência.
4. Selecione a Skeletal Mesh no Content Browser e use **Tools > Hype World > Aplicar padrão Indra ao modelo selecionado**. O comando verifica a bind pose antes de alterar o personagem e aplica a escala, a cápsula e as mesmas classes/clipes do Indra, preservando os materiais próprios da nova malha.
5. Confira parado, guarda, trote com strafe, corrida ninja, esquivas e Tigre antes de aprovar o novo modelo. O comando verifica estrutura; os pesos e a anatomia exigem inspeção visual.

**Tools > Hype World > Restaurar personagem Indra** volta à referência.

Um modelo com outro esqueleto não pode receber o Skeleton do Indra apenas trocando uma propriedade: isso não transfere pesos. Nesse caso, prepare a cópia no rig de referência primeiro. Retarget pode reproduzir movimentos entre esqueletos diferentes, mas não torna seus esqueletos idênticos.

O tamanho padrão usa o mesmo esqueleto e a escala visual 1,06. Cabelo, acessórios e roupas podem ultrapassar os limites do corpo; não se escala o personagem pela ponta do cabelo. Nenhuma animação original, lógica de jutsu, VFX ou mapa é editado por esse fluxo.
