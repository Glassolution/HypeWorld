---
status: procedimento
atualizado: 2026-11-08
---
# Exportar malha e criar novos clipes

> Validar a cópia e o arquivo exportado antes de importar ou salvar na Unreal.
> Duplicar um clipe compatível evita recriar a associação de Skeleton do zero.

**Malha:** exportar uma cópia em centímetros (`scale_length=.01`), rig `Armature`, armature modifier real, malha final + rig selecionados. FBX: forward `-Y`, up `Z`, primary bone Y, secondary X; sem leaf bones, sem bake de animação nem filtro que remova ossos; preservar morphs. Reabrir em cena temporária e conferir vínculo, ossos e pesos antes de importar.

**Unreal:** escolher o Skeleton aprovado; `update_skeleton_reference_pose=False`, `use_t0_as_ref_pose=False`, importar morphs quando usados. Não criar automaticamente materiais, texturas ou PhysicsAsset. Restaurar slots por nome, física/cloth compatíveis; conferir hierarquia e ref pose (tolerâncias já usadas: posição <0,002 cm, quaternion <1e-7).

**Novos selos:** 30 fps. Duplicar um clipe existente com Skeleton atual (ex.: `Macaco_Fixo`), limpar tracks/curvas só na cópia e escrever a nova animação. Não reamostrar nem regenerar clipes antigos. `get_editor_property('skeleton')` evita depender de atributo direto ausente.

Os controladores usam `RawCurveTrackTypes.RCT_FLOAT` / `RCT_TRANSFORM` e `AnimationCurveIdentifierExtensions.set_curve_identifier`; consultar a API instalada antes de presumir enums/construtores. Curvas de morph e de material têm metadados diferentes no Skeleton.

Biblioteca de poses: clipe de poses + PoseAsset reutilizável e JSON fonte, sem duplicar assets a cada execução. Extremos das transições devem reproduzir as poses aprovadas. Novos assets recebem pastas próprias; salvar apenas os pacotes validados.

Scripts de referência (ler/adaptar, nunca rodar cegamente): `WS/Referencias/Katon/Cavalo/MCP_Blender/{exportar_fbx,verificar_fbx,importar_previa_unreal,montar_previa_unreal}.py`.

Relacionadas: [[Rig]], [[Animacoes]], [[Katon]], [[Unreal]].
