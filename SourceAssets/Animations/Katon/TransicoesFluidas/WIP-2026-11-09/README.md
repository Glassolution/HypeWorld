# Rascunho das transições — não integrado na Unreal

Checkpoint publicado por pedido do usuário em 09/11/2026. A ficha em jogo continua usando as animações anteriores de GoukakyuuCompleto. Os 13 clipes candidatos são dados JSON; ainda não foram importados, validados nem salvos como assets Unreal. O .blend preserva a postura candidata do Javali e o personagem completo em cópia; ainda não contém a animação final aprovada.

Ritmo proposto aprovado: 1,86 s de selos; notify previsto em 2,235 s e voo do VFX atual cerca de 0,32 s depois. São previsões, não medição final. Durações detalhadas em Estado.json.

Diagnóstico inicial: entrada até 11,12 cm por amostra a 60 fps, Macaco→Javali até 6,01 cm; ombros estáveis durante os selos, planos dos cotovelos diferentes (Javali 11–13 cm mais alto). Há suavização cúbica nos clipes antigos, mas eles foram comprimidos para 0,14 s. Não se encontrou inversão instantânea do plano >90 graus.

Primeiro candidato com polo estável: trocas até ~1,62 cm por amostra, entrada ainda 6,84 cm. Não é prova de qualidade final. Entrada e retorno precisam de revisão: o neutro antigo é assimétrico e difere da postura real de idle. A série nova ainda tem que passar por inspeção visual, twists, clipping, importação e Play. Não usar automaticamente as poses candidatas em produção.

Scripts/motion.py e Dependencies contêm o gerador em desenvolvimento para executar no MCP do Blender com __file__ definido. Gerar somente com o .blend desta cópia aberto e a Unreal fechada. Scripts não são executados ao abrir o projeto Unreal. Diagnostico/ e Blender/ guardam as amostras numéricas. As capturas completas anteriores continuam no workspace local; este checkpoint não adiciona fotos ao cofre.
