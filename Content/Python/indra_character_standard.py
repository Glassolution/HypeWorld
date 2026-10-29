"""Padrão persistente do personagem. Execute pelo menu Tools > Hype World."""
import json
from pathlib import Path
import unreal

PROFILE = Path(__file__).with_name('indra_character_standard.json')
BP = '/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter'
REFERENCE = '/Game/HypeWorld/Personagens/Indra/SK_Indra'


def _body(bp):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = library.get_data(handle)
        if str(library.get_variable_name(data)) == 'Indra':
            return library.get_associated_object(data)
    raise RuntimeError('Componente visual Indra ausente.')


def _vector(v):
    return [v.x, v.y, v.z]


def _reference_bones(mesh):
    # A bind pose pertence à malha; o Skeleton compartilhado pode ter outra pose.
    rig = unreal.new_object(unreal.IKRigDefinition)
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(mesh)
    pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property('skeleton'))
    result = {}
    for bone in unreal.AnimPoseExtensions.get_bone_names(pose):
        t = controller.get_ref_pose_transform_of_bone(bone)
        result[str(bone)] = {'position_cm': _vector(t.translation),
                             'rotation_xyzw': [t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w],
                             'scale': _vector(t.scale3d)}
    return result


def capture_reference():
    """Inicialização explícita; o padrão nunca é recapturado ao trocar o modelo."""
    if PROFILE.exists():
        raise RuntimeError('Padrão já definido. Não sobrescrever com outro personagem.')
    bp = unreal.load_asset(BP)
    body = _body(bp)
    mesh = body.get_editor_property('skeletal_mesh_asset')
    assert mesh.get_path_name() == REFERENCE + '.SK_Indra'
    cdo = unreal.get_default_object(bp.generated_class())
    capsule = cdo.get_editor_property('capsule_component')
    driver = cdo.get_editor_property('mesh')
    bounds = mesh.get_bounds()
    movement = cdo.get_editor_property('character_movement')
    profile = {
        'version': 1, 'name': 'Personagem base — Indra', 'blueprint': BP,
        'reference_mesh': mesh.get_path_name(),
        'skeleton': mesh.get_editor_property('skeleton').get_path_name(),
        'anim_class': body.get_editor_property('anim_class').get_path_name(),
        'ik_rig': '/Game/HypeWorld/Personagens/Indra/IK_Indra',
        'retargeter': '/Game/HypeWorld/Personagens/Indra/RTG_Manequim_Para_Indra',
        'body_scale': _vector(body.get_editor_property('relative_scale3d')),
        'body_location': _vector(body.get_editor_property('relative_location')),
        'body_rotation': {k: getattr(body.get_editor_property('relative_rotation'), k) for k in ['pitch','yaw','roll']},
        'mesh_height_cm': bounds.box_extent.z * 2,
        'display_height_cm': bounds.box_extent.z * 2 * body.get_editor_property('relative_scale3d').z,
        'capsule_radius_cm': capsule.get_unscaled_capsule_radius(),
        'capsule_half_height_cm': capsule.get_unscaled_capsule_half_height(),
        'driver_mesh': driver.get_editor_property('skeletal_mesh_asset').get_path_name(),
        'driver_anim_class': driver.get_editor_property('anim_class').get_path_name(),
        'movement': {k: movement.get_editor_property(k) for k in ['run_speed','run_back_speed','run_strafe_speed','sprint_speed']},
        'body_animations': {k.get_path_name(): v.get_path_name() for k,v in cdo.get_editor_property('jutsu_body_animations').items()},
        'reference_bones': _reference_bones(mesh),
    }
    PROFILE.write_text(json.dumps(profile, indent=2, ensure_ascii=False), encoding='utf-8')
    return profile


def validate_mesh(mesh):
    profile = json.loads(PROFILE.read_text(encoding='utf-8'))
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError('Selecione uma Skeletal Mesh no Content Browser.')
    if mesh.get_editor_property('skeleton').get_path_name() != profile['skeleton']:
        raise RuntimeError('Malha com outro esqueleto. Vincule uma cópia ao rig do Indra no Blender/Maya e importe com SKEL_Indra. Nenhum arquivo foi alterado.')
    bones = _reference_bones(mesh)
    if set(bones) != set(profile['reference_bones']):
        raise RuntimeError('Hierarquia incompatível com o padrão Indra.')
    for name, expected in profile['reference_bones'].items():
        actual = bones[name]
        distance = sum((a-b)**2 for a,b in zip(actual['position_cm'],expected['position_cm'])) ** .5
        dot = abs(sum(a*b for a,b in zip(actual['rotation_xyzw'],expected['rotation_xyzw'])))
        scale_error = max(abs(a-b) for a,b in zip(actual['scale'],expected['scale']))
        if distance > .1 or dot < .99999 or scale_error > .001:
            raise RuntimeError('Bind pose diferente no osso ' + name + '. Ajuste a cópia da malha ao rig original antes de aplicar.')
    return profile


def apply_mesh(mesh):
    profile = validate_mesh(mesh)  # Validar tudo antes de modificar o Blueprint.
    if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
        raise RuntimeError('Encerre o Play antes de trocar o personagem.')
    bp = unreal.load_asset(profile['blueprint'])
    with unreal.ScopedEditorTransaction('Aplicar padrão de personagem Indra'):
        bp.modify()
        body = _body(bp)
        body.modify()
        body.set_editor_property('skeletal_mesh_asset', mesh)
        body.set_editor_property('anim_class', unreal.load_class(None, profile['anim_class']))
        body.set_editor_property('animation_mode', unreal.AnimationMode.ANIMATION_BLUEPRINT)
        body.set_editor_property('relative_scale3d', unreal.Vector(*profile['body_scale']))
        body.set_editor_property('relative_location', unreal.Vector(*profile['body_location']))
        body.set_editor_property('relative_rotation', unreal.Rotator(**profile['body_rotation']))
        body.set_editor_property('override_materials', [])
        body.set_visibility(True, False)
        body.set_hidden_in_game(False, False)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.modify()
        cdo.set_editor_property('jutsu_body_animations', {unreal.load_asset(k):unreal.load_asset(v) for k,v in profile['body_animations'].items()})
        capsule = cdo.get_editor_property('capsule_component')
        capsule.modify()
        capsule.set_capsule_size(profile['capsule_radius_cm'], profile['capsule_half_height_cm'], False)
        movement = cdo.get_editor_property('character_movement')
        movement.modify()
        for key, value in profile['movement'].items():
            movement.set_editor_property(key, value)
        driver = cdo.get_editor_property('mesh')
        driver.modify()
        driver.set_editor_property('skeletal_mesh_asset', unreal.load_asset(profile['driver_mesh']))
        driver.set_editor_property('anim_class', unreal.load_class(None, profile['driver_anim_class']))
        driver.set_visibility(False, False)
        driver.set_hidden_in_game(True, False)
        driver.set_editor_property('visibility_based_anim_tick_option', unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
        assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log('Padrão Indra aplicado: ' + mesh.get_path_name())


def apply_selected():
    selected = unreal.EditorUtilityLibrary.get_selected_assets()
    if len(selected) != 1:
        raise RuntimeError('Selecione apenas uma Skeletal Mesh no Content Browser.')
    apply_mesh(selected[0])


def restore_indra():
    apply_mesh(unreal.load_asset(REFERENCE))


def register_menu():
    menus = unreal.ToolMenus.get()
    menu = menus.extend_menu('LevelEditor.MainMenu.Tools')
    submenu = menu.add_sub_menu('HypeWorld.IndraStandard','HypeWorld','HypeWorld.CharacterStandard','Hype World')
    for name,label,command in [('Apply','Aplicar padrão Indra ao modelo selecionado','apply_selected'),('Restore','Restaurar personagem Indra','restore_indra')]:
        entry = unreal.ToolMenuEntry(name='HypeWorld.Indra.'+name, type=unreal.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON,'',f'import indra_character_standard; indra_character_standard.{command}()')
        submenu.add_menu_entry('Character',entry)
    menus.refresh_all_widgets()
