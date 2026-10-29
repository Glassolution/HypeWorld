# Auditoria dos selos — primeira etapa do Katon

## Resultado para confirmação

Foram lidas as 379 AnimSequences disponíveis em `/Game/ANAP`. Não foi possível confirmar nenhum dos 12 selos canônicos pronto para uso, inclusive os seis exigidos para o Katon. Isso descreve os assets instalados neste projeto; não afirma que outras versões ou partes do pack comercial não contenham selos.

As pastas instaladas contêm Base (81), Misc (85), Pose (56) e Attack/HandBlades (157). Há também um montage. Não há pasta específica de ninjutsu ou de selos nessa instalação. Os candidatos renderizados mostram mãos em gestos de chakra, pegadas de armas, proteção e braços cruzados.

`Prancha_Selos_ANAP.png` é a prancha resumida para revisão. Os 12 itens estão marcados como não localizados/não confirmados: não existem quadros verificados de cada um para apresentar como selos prontos.

## Evidência e limites

- `auditoria_pack.json`: caminhos, duração, esqueleto e amostras dos candidatos. Triagem pela proximidade das mãos (<32 cm) e altura acima da pelve (>15 cm); avaliação a aproximadamente 24 Hz, ou 12 Hz nas poses, com pelo menos 18 amostras por animação. Essa heurística pode perder gestos baixos ou muito breves; a classificação não equivale a uma revisão manual de todos os frames.
- `quadros_manifesto.json` e `quadros/`: 18 quadros de candidatos prioritários, incluindo nomes potencialmente associados a ninjutsu.
- `triagem_completa/`: 68 quadros adicionais dos candidatos terrestres detectados, organizados nas pranchas `triagem_0.png`, `triagem_20.png`, `triagem_40.png` e `triagem_60.png`. Inclui poses repetidas/transições. Clipes com Aerial no nome não entraram nessa segunda renderização, embora tenham entrado na triagem de poses.
- Quadros renderizados pela Unreal em SceneCapture2D, com Quinn e material cinza de inspeção. O material foi aplicado apenas ao componente de inspeção, sem alterar o SkeletalMesh ou os materiais do pack.
- `originais_sha256.json`: hashes SHA-256 dos 380 uassets originais, para conferência de integridade.

## Candidatos próximos e tempos

| Animação original | Quadro mostrado | Classificação visual |
|---|---:|---|
| Base_Anim_Armature_CustomMan_RecoveryChakra_Start | 0,319 / 0,368 / 0,417 s | Transição para um gesto de chakra com uma mão apoiando o punho da outra. Não confirmado como um dos 12 selos. |
| Base_Anim_Armature_CustomMan_RecoveryChakra_Loop | 0,250 s | Mesmo gesto sustentado. Base possível para ajustar braços/mãos. |
| Misc_Anim_Armature_CustomMan_Beaten_ShadowPossession_Loop | 0,750 s | Mãos sobrepostas; dedos não formam um selo confirmado. |

Esses tempos são amostras de inspeção, não inícios de selos. Não foi identificada uma sequência multisselo verificável. Não foram adicionados Anim Notifies nem criadas animações `A_Selo_*` com identificação não confirmada.

## Alternativa proposta: ajustar pose em cópias

Priorizar os seis selos do Katon, mantendo uma base corporal compatível entre eles e ajustando braços, punhos e falanges. A proposta requer avaliação de poses novas antes da montagem do jutsu.

| Selo | Asset proposto | Ajuste a construir e revisar |
|---|---|---|
| Cobra | A_Selo_Cobra | Aproximar as mãos e entrelaçar os dedos. |
| Carneiro | A_Selo_Carneiro | Formar os dedos elevados e a assimetria específica das duas mãos. |
| Macaco | A_Selo_Macaco | Reorientar palmas e punhos para contato horizontal oposto. |
| Javali | A_Selo_Javali | Reorientar as mãos juntas para os dedos apontarem para baixo. |
| Cavalo | A_Selo_Cavalo | Ajustar os indicadores elevados e o contato dos demais dedos. |
| Tigre | A_Selo_Tigre | Erguer indicadores e médios; ajustar dedos restantes e polegares. |

Rato, Boi, Coelho, Dragão, Galo e Cão também permanecem sem correspondência confirmada; alternativa proposta: modelar as respectivas poses em cópias da mesma base e apresentar nova prancha para revisão. Não proponho substituir silenciosamente um selo por outro.

Referência de nomenclatura e comparação visual: [Hand Seal — Narutopedia](https://naruto.fandom.com/wiki/Hand_Seal). As correspondências dos assets foram avaliadas nos renders locais; os nomes dos clipes não foram usados como prova de identidade.

## Estado de entrega

Somente a etapa de auditoria foi executada. O cenário separado é `/Game/HypeWorld/Jutsus/Auditoria/L_Selos_Auditoria`; o material local de inspeção é `M_Selos_Clay`. Não foram montados selos novos, sistema de jutsu, socket, VFX, projétil, dano, chakra, cooldown ou replicação. Não há vídeo nem medição de GPU nesta etapa. A meta de 1,5 ms permanece sem validação.

Próxima decisão solicitada: confirmar a criação das seis poses ajustadas para uma nova prancha, antes de montar o restante do jutsu.
