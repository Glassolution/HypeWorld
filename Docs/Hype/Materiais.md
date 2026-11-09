---
status: referencia
atualizado: 2026-11-08
---
# Texturas, slots e acabamento

> Preservar UVs e mapas aprovados; conferir materiais com exposição comparável.
> Mudar a cor da textura não corrige uma máscara ou deformação defeituosa.

- BaseColor: sRGB; normal: compressão própria e sRGB desligado, respeitando a convenção do mapa. Roughness/metallic: lineares e canal correto.
- Não inventar normal ausente nem usar AgX na extração de BaseColor para a Unreal. Respeitar materiais OPAQUE de cabelo; não criar recortes a partir de alfa ignorado na fonte.
- Ninja aprovada: tecido opaco azul-marinho `M_Ninja_Clean_Shirt` (roughness 0,85, specular 0,25); gola `M_Ninja_Clean_Collar_v2`. Pontos claros antigos eram da máscara, não da geometria.
- Seus seis slots: `M_Test_Head`, `M_Test_Body`, `M_Test_Skin`, `M_Ninja_Wrap_Fabric`, `M_Ninja_Clean_Collar_v2`, `M_Ninja_Clean_Shirt`. Não impor essa ordem/lista a outra roupa.
- Reimportação/troca: restaurar materiais por nome de slot, limpar overrides incompatíveis e conferir cache; não escurecer pele para compensar exposição diferente.
- SOLID do Blender não prova opacidade/cloth/material do jogo. Validar no Play os corretivos e máscaras usados pelos clipes novos.

Fonte completa: guia anterior da etapa 17, preservado no arquivo desta migração.

Relacionadas: [[Rig]], [[Importacao]], [[Customizacao]], [[Blender]].
