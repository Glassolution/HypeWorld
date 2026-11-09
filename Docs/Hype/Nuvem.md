---
status: condicional
atualizado: 2026-11-08
---
# Agentes na nuvem e portabilidade

> Sem acesso ao editor local, código escrito não equivale a compilação ou Play.
> A cópia versionada fica em Docs/Hype e pode ser lida sem Obsidian aberto.

O projeto usa Git/Git LFS. Esta cópia do cofre é um snapshot de WS/Hype publicado em 09/11/2026 junto às alterações. Leia 00-Inicio.md e só as notas da tarefa. As fontes WS e seus backups podem existir apenas no Mac; não fingir acesso a arquivos ausentes. Histórico arquivado não é leitura inicial.

Agente sem engine não deve editar binários `.uasset/.umap` nem tratar ponteiro LFS como asset válido. Pode preparar C++/configuração/scripts quando autorizado, seguindo padrões existentes, e registrar APIs não compiladas como suposições. Scripts de editor devem ser repetíveis; entradas/execução local precisam estar disponíveis antes de anunciar resultado.

`Tools/pendentes.txt` pertence ao fluxo “Atualizar e Abrir”; alterar a fila e fazer push requer escopo da tarefa atual. A autorização histórica de push no main foi preservada no arquivo, mas não concede nova permissão por si só.

Prova remota: commit quando solicitado, com aviso explícito de que não compilou/testou. Mudanças no jogo só estão validadas após execução local correspondente.

Relacionadas: [[Ambiente]], [[Regras]], [[Projeto]].
