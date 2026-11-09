---
status: procedimento
atualizado: 2026-11-08
---
# Conexão e trabalho na Unreal

> O fluxo local comprovado usa Python Remote Execution com ue_exec.py.
> Esta consulta não conecta ao editor; só executar comandos durante a tarefa autorizada.

Ferramenta: `UE/Tools/ue_exec.py`. No editor, Python Remote Execution deve estar habilitado. A ferramenta usa a implementação oficial da engine e ajuste de loopback do Mac; a descoberta usa `127.0.0.1:6766`. Não confundir com a porta do Blender.

A partir de UE, teste somente leitura:
```sh
python3 Tools/ue_exec.py 'print(unreal.SystemLibrary.get_engine_version())'
python3 Tools/ue_exec.py -f /caminho/absoluto/script.py
```

Esperar o Python do editor ficar pronto. Conferir retorno e resultado gravado pelo script: código de saída zero do comando não garante `error: null` num teste de Play. Não executar novamente importação/mutação se uma dependência falhou ou expirou aprovação.

Antes de fechar: verificar ausência de PIE e de pacotes/mapas sujos. Não encerrar descartando trabalho alheio. Build com editor e Blender fechados, usando o `Build.sh` da instalação e `-MaxParallelActions=1` quando adequado à memória; não compilar para uma mudança só de notas.

Importação de selos: seguir [[Importacao]]; conferir a pose em Play antes de salvar novos assets. Não usar `save_all` que alcance pacotes alheios. Na sequência, reabrir e conferir integração automática, sem troca manual de malha mascarando um catálogo errado.

Fonte: cabeçalho e argumento `-f` de `UE/Tools/ue_exec.py`; scripts de prova nos diretórios de selos apontados em [[Katon]].

Relacionadas: [[Ambiente]], [[Regras]], [[Importacao]], [[Katon]].
