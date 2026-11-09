---
status: arquivado
atualizado: 2026-11-08
---
# Texturas e acabamento — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Texturas e acabamento

| Mapa | Configuração de referência |
|---|---|
| BaseColor | sRGB ligado; UVs originais preservadas |
| Normal, se fornecido | Compressão de normal, sRGB desligado; conferir convenção do mapa antes de inverter canais |
| Roughness | Linear, sRGB desligado; canal correto se estiver empacotado |
| Metallic | Linear, sRGB desligado; canal correto se estiver empacotado |

Não inventar um normal map ausente: o pacote triangular da roupa Ninja não tinha normal do corpo. Materiais da pele/cabeça usam os mapas da base aprovada. Comparar com a mesma exposição e iluminação do criador; não compensar pele lavada escurecendo arbitrariamente a textura.

A camiseta Ninja aprovada é um caso específico: tecido uniforme opaco azul-marinho, roughness 0,85, specular Unreal 0,25. A máscara anterior deixava pontos claros da cor antiga vazarem. A correção final foi `M_Ninja_Clean_Shirt`, não mais suavização da malha ou pintura em screenshots. A gola usa `M_Ninja_Clean_Collar_v2`.

Slots finais da Ninja: 0 pele/cabeça `M_Test_Head`; 1 parte inferior `M_Test_Body`; 2 pele do doador `M_Test_Skin`; 3 faixas `M_Ninja_Wrap_Fabric`; 4 gola `M_Ninja_Clean_Collar_v2`; 5 camisa `M_Ninja_Clean_Shirt`. Restaurar materiais por **nome de slot**, não confiar na ordem após reimportar. Esses seis slots são desta roupa, não um formato obrigatório para qualquer roupa futura.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
