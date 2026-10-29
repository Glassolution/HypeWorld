from pathlib import Path
from PIL import Image
import json,re,hashlib,subprocess
out=Path(__file__).resolve().parent;root=out.parents[3]
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
before=json.loads((out/'config_hashes_antes.json').read_text(encoding='utf-8-sig'))
config_changed=[r['Path'] for r in before if digest(r['Path'])!=r['Hash']]
settings_equal=digest(root/'Saved/Config/WindowsEditor/GameUserSettings.ini')==digest(out/'GameUserSettings_antes.ini')
assert not config_changed and settings_equal
names=['grama_40','grama_100','indra_toon_ligado','indra_toon_desligado']
states={n:json.loads((out/(n+'.json')).read_text()) for n in names}
norm=lambda x:re.sub('0x[0-9A-Fa-f]+','ADDR',str(x))
camera_keys=['camera_location','camera_rotation','fov','player_location','player_rotation','control']
camera_equal=all(norm(states[n][k])==norm(states[names[0]][k]) for n in names for k in camera_keys)
assert camera_equal
base=states['grama_40']['cvars'];other=states['grama_100']['cvars']
changed_cvars={k:[v,other[k]] for k,v in base.items() if other[k]!=v}
assert changed_cvars=={'grass.DensityScale':['0.4','1']}
images={}
for n in names:
    p=out/(n+'.png');im=Image.open(p);assert im.size==(1920,1080)
    images[n]={'dimensions':list(im.size),'sha256':digest(p)}
done=json.loads((out/'coleta_concluida.json').read_text())
toon_restored=norm(done['blendables_original'])==norm(done['blendables_final'])
assert toon_restored
report={'config_project_unchanged':not config_changed,'game_user_settings_restored_byte_identical':settings_equal,'camera_and_player_transform_identical':camera_equal,'only_different_cvar_in_grass_AB':changed_cvars,'toon_restored':toon_restored,'screenshots':images}
(out/'integridade_final.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
