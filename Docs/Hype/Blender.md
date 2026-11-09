---
status: procedimento
atualizado: 2026-11-08
---
# Blender: MCP e execução sem interface

> O MCP dirige o Blender aberto; o modo background é um processo separado.
> Escolher um único caminho e respeitar a memória e a sessão já em andamento.

**MCP preferido para poses:** procurar as ferramentas do Blender, confirmar resposta com captura do viewport antes de abrir outro arquivo. Servidor local configurado na porta **9876**. Em caso de queda, tentar reconectar uma vez; se falhar, avisar. Não iniciar outro Blender ou agente em paralelo.

Abrir a cópia do personagem final com malha/roupa e rig aprovados; backup `.blend` antes de editar. Ajustar por código em modo Pose, em ciclos curtos: silhueta → mãos/punhos → transições. Avaliar frente do tronco inteiro com cotovelos, perfil, cima, três quartos e closes. Câmera fixa, campo estreito, boa iluminação.

Viewport: MATERIAL é opcional. Se falhar, SOLID com `color_type='TEXTURE'` e luz de estúdio; depois `MATERIAL`; se captura falhar, câmera/render leve. Falha só de visualização não invalida a pose. O modo sólido não reproduz máscaras de opacidade; conferir esse limite na Unreal.

**Background, somente quando fizer sentido na tarefa:**
```sh
/Applications/Blender.app/Contents/MacOS/Blender -b /copia/personagem.blend --python /script.py
```
O script cria diretórios, é repetível e não usa operadores de interface sem contexto. Preferir API de dados. Conferir o resultado do comando; não depender de `ps`. Crash 139: verificar crash log temporário e isolar etapas; não insistir no mesmo passo nem abrir vários processos.

No MCP, o namespace Python pode ser novo a cada chamada: carregar o script/estado explicitamente. Multiplicar matrizes/vetores com `@`, dimensões compatíveis; não aumentar limites de segurança para esconder erro de deformação.

Relacionadas: [[Ambiente]], [[Rig]], [[Erros]], [[Selos/Carneiro]].
