---
status: referencia
atualizado: 2026-11-08
---
# Ambiente e caminhos

> WS é a pasta de fontes; UE é o projeto executado. Não são cópias equivalentes.
> Os caminhos nas notas usam esses dois prefixos para evitar repetir diretórios longos.

- **WS:** `/Users/luisfelipexavier/Desktop/Hype World`.
- **UE:** `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld`.
- **Cofre:** `WS/Hype`; é uma pasta Markdown local, sem necessidade de abrir o Obsidian.
- **Engine instalada:** `/Users/Shared/Epic Games/UE_5.8`. Conferir versão em uso quando depender de API.
- **Blender:** `/Applications/Blender.app/Contents/MacOS/Blender`.
- **Máquina:** Mac M1, 8 GB de RAM. Fluxo dos selos: Unreal fechada durante Blender; fechar Blender antes de importar/testar na Unreal.
- **Repositório Git:** UE. WS não é um repositório Git nesta auditoria. Não executar commit/pull/build só para consultar as notas.
- **Sincronização:** `Tools/atualizar_e_abrir.sh` copia o CLAUDE do UE para WS. As duas entradas agora apontam ao mesmo cofre. O script não foi alterado; o cofre local não foi publicado nem sincronizado com a nuvem.
- **Mapas configurados:** confirmar em `UE/Config/DefaultEngine.ini` apenas as linhas `GameDefaultMap` e `EditorStartupMap`; detalhes e proteção visual em [[Mapa]].

Relacionadas: [[00-Inicio]], [[Unreal]], [[Blender]], [[Nuvem]].
