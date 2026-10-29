# Encerramento

Base aprovada aplicada e commitada em **5130bb0**, com Visual_Aprovado.md atualizado.

Alto-60 experimental: 99 proxies opacos de sombra, 215 componentes de rocha sem Nanite, FSR Native AA, PCSS desligado, alcance menor, névoa volumétrica e SSR desligados, água simplificada, velocity da grama desligada. Não adotado globalmente.

Diagnóstico preliminar 3×30 s: 44,82 FPS, 1% low 31,55, GPU 21,75 ms. O personagem ficou fora do enquadramento, portanto **não é uma medição final válida**. Dados preservados em preliminar.json e alto60_r1..3.csv. Neste diagnóstico faltavam 5,15 ms para 16,6 ms; maiores passes: FSR dispatch 3,81 ms, BasePass convencional 2,57 ms, Nanite VisBuffer 1,94 ms. Preparação do FSR e BasePass Nanite são passes adicionais; não confundir esses valores com custos completos dos subsistemas.

A série corrigida e os vídeos foram interrompidos por outro controlador Python na mesma sessão de Unreal (pie_sasuke.py). Não há vídeos completos, análise de ghosting ou quadros de movimento validados. Tentativas incompletas não entram como resultado final.

O print agressivo mostra sombras de copa simplificadas em bolhas e com borda mais dura, menor contato detalhado e menos profundidade atmosférica. Água não aparece no enquadramento e não foi validada visualmente. Não recomendar remover velocity da grama sem a validação de rastros.

A sessão estava em uso por outra automação e foi preservada. Não restaurar preferências por cima de alterações concorrentes. Backup inicial em ConfigAntes. Nenhuma animação ou layout de produção foi alterado por esta tarefa.
