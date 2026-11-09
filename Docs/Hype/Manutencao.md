---
status: vigente
atualizado: 2026-11-08
---
# Como ler e manter a biblioteca

> A economia vem da seleção de notas e da atualização do estado, não do Obsidian.
> Cada fato tem uma nota responsável; histórico superado fica fora da leitura padrão.

**Leitura:** índice → nota da tarefa → trecho da fonte, somente se necessário. Não ler todas as notas relacionadas em cascata. Antes de abrir arquivo desconhecido, consultar tamanho/cabeçalhos; usar `rg -n -m 8 'termo' arquivo` e uma faixa curta com `sed -n 'inicio,fimp' arquivo`. Limitar também a largura das linhas: os CLAUDE antigos tinham linhas com milhares de caracteres. Não carregar logs, históricos extensos ou binários por inteiro.

**Ao concluir uma tarefa:**
1. Atualizar em 2–5 linhas a nota responsável: resultado, decisão/medição com unidade, prova e próximo passo. Alterar `status` e `atualizado`.
2. Substituir o estado superado; não adicionar um diário no rodapé. Detalhes de execução permanecem na fonte/log, com caminho e seção/consulta precisa.
3. Se houver informação única que sairá, mover a versão anterior para `Arquivo/AAAA-MM-DD/`, marcada `arquivado`. Se ela já estiver integralmente preservada em outra fonte, apontar a fonte em vez de duplicar.
4. Atualizar [[Estado]] apenas se mudar a prioridade ou o próximo passo. O índice ganha só uma linha se houver uma nova nota ativa.

**Tamanhos máximos:** nota ativa 4.000 caracteres / 80 linhas; índice 1.600 caracteres / 40 linhas; nota de arquivo 6.000 caracteres / 120 linhas. Ao atingir qualquer limite, dividir por assunto antes de salvar. Resumos têm 2–3 linhas, além de status, data e ligações relacionadas.

**Responsabilidade:** timing/ficha em [[Katon]]; erro e retomada de uma pose na nota do selo; rig em [[Rig]]; catálogo em [[Customizacao]]; resultados do mapa em [[Mapa]]. Resumos e links podem apontar fatos, mas não copiar tabelas ou procedimentos para outra nota.

**Arquivo:** não é lido por padrão. Pesquisar nele explicitamente quando houver lacuna; texto arquivado não renova autorizações nem torna uma proposta antiga uma decisão atual. Não embutir fotos; registrar descrições e caminhos de provas já existentes.

Verificação local, sem editor: `python3 Hype/Ferramentas/verificar_biblioteca.py` a partir de WS.

Relacionadas: [[00-Inicio]], [[Estado]], [[Arquivo/2026-11-08-migracao/Indice]].
