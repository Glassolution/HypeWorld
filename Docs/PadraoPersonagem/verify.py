import unreal,json,hashlib
from pathlib import Path
import indra_character_standard as standard
root=Path(unreal.Paths.project_dir()).resolve()
f=root/'Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset'
before=hashlib.sha256(f.read_bytes()).hexdigest()
rejected=False
try:
    standard.apply_mesh(unreal.load_asset('/Game/HypeWorld/Personagens/Shisui/SK_Shisui'))
except RuntimeError as error:
    rejected=True
    print('INCOMPATIBLE_REJECTED',str(error))
assert rejected and before==hashlib.sha256(f.read_bytes()).hexdigest()
p=standard.validate_mesh(unreal.load_asset(standard.REFERENCE))
bp=unreal.load_asset(standard.BP)
body=standard._body(bp)
cdo=unreal.get_default_object(bp.generated_class())
assert body.get_editor_property('anim_class').get_path_name()==p['anim_class']
assert abs(body.get_editor_property('relative_scale3d').z-1.06)<.0001
cap=cdo.get_editor_property('capsule_component')
assert cap.get_unscaled_capsule_half_height()==p['capsule_half_height_cm']
assert (root/'Docs/PadraoPersonagem/Indra_Referencia.fbx').stat().st_size>1000
report={'profile_valid':True,'reference_bones':len(p['reference_bones']),'display_height_cm':p['display_height_cm'],'capsule_height_cm':cap.get_unscaled_capsule_half_height()*2,'incompatible_rejected_without_mutation':True,'fbx_exported':True}
(root/'Docs/PadraoPersonagem/verification.json').write_text(json.dumps(report,indent=2))
print('STANDARD_VERIFIED',report)
