---
status: arquivado
atualizado: 2026-11-08
---
# Exportação e importação aprovadas — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Exportação e importação aprovadas

Trabalhar em cópia descartável para exportar: a referência editável continua em metros. O procedimento existente converte rig, malha e shape keys para centímetros, usa `unit_settings.scale_length=.01`, rig `Armature`, modificador Armature real ligado ao rig, e seleciona somente rig + malha final. O parent sozinho não exporta skinning corretamente.

FBX da referência: `axis_forward='-Y'`, `axis_up='Z'`, primary bone Y, secondary X, sem leaf bones, sem bake de animação, sem filtrar os ossos aprovados. Preservar morph targets. Abrir o FBX exportado em uma cena vazia e conferir que existem os 161 ossos, pesos e vínculo de armature, antes de importar na Unreal.

Na importação: skeleton aprovado, `update_skeleton_reference_pose=False`, `use_t0_as_ref_pose=False`, `import_morph_targets=True` quando há mãos/corretivos; sem import automático de animação, materiais, texturas ou novo PhysicsAsset. Reutilizar física/slots da referência compatível. A roupa Ninja usou até oito influências, UVs em precisão completa, tangentes Mikk e normais ponderadas; respeitar normais próprias de outra peça quando necessário. Verificar nomes de ossos, hierarquia e ref pose após importar: os scripts atuais usam erro de posição menor que 0,002 cm e erro de quaternion menor que 1e-7.

Não aceitar regeneração automática do skeleton para contornar um FBX incorreto. Se aparecer esse aviso, conferir exportação, skin clusters, unidades e nomes na cópia. Não salvar os assets base para resolver um erro da roupa.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
