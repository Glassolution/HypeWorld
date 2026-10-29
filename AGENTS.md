# Padrão do personagem base

O usuário definiu o Indra como referência permanente para novos modelos.
- Fonte: `Content/Python/indra_character_standard.json`. Não recapture esse perfil de outro modelo.
- Preserve `SKEL_Indra`, hierarquia, bind pose, tamanho, proporções do rig, animações originais e a escala visual 1,06. A cápsula e as velocidades são as registradas no perfil.
- Prepare a nova malha em cópia e vincule seus pesos ao rig do Indra. Não substitua a propriedade Skeleton de uma malha incompatível para simular conversão. Não deforme o rig ou as animações para ajustar uma roupa.
- Aplique com `indra_character_standard.apply_mesh(mesh)` no Python do Unreal; o menu Tools > Hype World também oferece essa ação. Confira o personagem parado, guarda, trote com strafe, corrida ninja, esquivas e Tigre.
- Não altere jutsus, VFX ou mapas durante troca de modelo. Preserve os modelos anteriores.
- Instruções de preparação e FBX do rig: `Docs/PadraoPersonagem/README.md` e `Indra_Referencia.fbx`.
