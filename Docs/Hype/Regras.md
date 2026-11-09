---
status: vigente
atualizado: 2026-11-08
---
# Regras de trabalho

> Os arquivos de entrada contêm só os limites essenciais e o caminho do índice.
> Aqui ficam os cuidados operacionais; a autorização da tarefa atual define o escopo.

- Fazer backup com hashes ou commit antes de alterar. Preservar fontes, versões aprovadas e trabalho local alheio.
- Assets que precisarem sair vão para `/Content/_ToDelete/`, com aviso. Nunca apagar assets nem substituir originais aprovados para consertar uma cópia.
- Um agente controla a Unreal por vez; um controla o Blender por vez. Não enviar comandos a um editor ocupado por outro agente. Com 8 GB, seguir a alternância de aplicativos em [[Ambiente]].
- Concluir só com prova real e com o alcance da prova declarado: log, captura ou commit. Build não substitui Play; uma captura não prova todos os quadros.
- Scripts de criação/importação precisam poder rodar de novo sem duplicar assets ou pastas.
- Nos selos, parar e avisar quando houver erro que afete o trabalho ou expirar aprovação de Blender/ue_exec. Consulta auxiliar negada não interrompe o trabalho; usar outra checagem.
- Não supor que APIs, assets ou ferramentas existem: consultar a API local e caminhos necessários. Suposições devem ser identificadas.
- Créditos pagos de geração: informar modelo/custo e aguardar autorização antes de gastar; não gerar lotes sem pedido.
- Explicar em português, curto e simples: mudança, prova, pendência e implicação. Não adicionar novos fluxos de confirmação quando a tarefa já autoriza o passo.

Relacionadas: [[00-Inicio]], [[Manutencao]], [[Unreal]], [[Blender]].
