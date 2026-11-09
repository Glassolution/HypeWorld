---
status: referencia
atualizado: 2026-11-08
---
# Erros e decisões que não repetir

> Esta nota guarda lições transversais; o erro concreto e a retomada ficam no tema responsável.
> Não aplicar um conserto geral ao personagem para esconder um problema de uma pose.

- “Criado” não é “validado/salvo”: conferir arquivo, resultado de Play e integração automática. Logs antigos não demonstram o estado atual.
- Skeleton não associado em AnimSequence novo: primeiro duplicar clipe compatível; procedimento em [[Importacao]]. Não reconstruir rigs ou reamostrar clipes antigos para contornar a fábrica.
- Braços em T: partir da postura de braços aprovada; conferir espaço local/absoluto, eixos e bind antes de recalcular. Captura frontal cortada não verifica cotovelos.
- Punho “papel de bala”/bola: separar torção de antebraço e flexão de punho; conferir influência dos twists, distribuir o giro e respeitar anatomia. Pesos só quando comprovadamente necessário, com backup e prova de que selos aprovados continuam iguais.
- Matriz 3×3/4×4, inversão mal condicionada e deslocamento excessivo não se resolvem elevando limites ou aceitando uma malha aparentemente boa só de frente. Retomada específica em [[Selos/Carneiro]].
- Não confundir volume-preserving do Blender com skinning que será exportado. Validar a deformação efetiva da cópia/FBX e na Unreal; preview diferente não é prova de correção.
- Malha de massa cinza, mangas ocultando contato ou viewport sem máscara não validam sozinhos a mão final. Se usar diagnóstico sem roupa, identificar a limitação.
- Abertura de punho/manga do Macaco foi aceita como preservação do selo; não alterá-la ao consertar trecho novo. A barra irregular do Manto já existia: avisar antes de uma correção global.
- Mais triângulos removidos não equivale a menos ms; consultar [[Mapa]] antes de reabrir a hipótese Nanite neste Mac.
- Referência Indra histórica e “MCP ainda pendente” de outubro são informação superada para novos selos. Conferir capacidade atual sem reexecutar setups históricos.

Relacionadas: [[Rig]], [[Blender]], [[Importacao]], [[Selos/Macaco]], [[Selos/Carneiro]], [[Mapa]].
