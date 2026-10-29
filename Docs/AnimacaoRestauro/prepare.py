from audit import *
import shutil
OUT.mkdir(exist_ok=True)
BACK=OUT/'BackupAntes'
assert not BACK.exists(), 'Backup already exists'
BACK.mkdir()
targets=['Content/Ninja/Characters/Animations/ABP_Ninja.uasset','Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset','Source/HypeWorld/Ninja/NinjaAnimInstance.cpp','Source/HypeWorld/Ninja/NinjaBodyAnimInstance.cpp','Source/HypeWorld/Ninja/NinjaBodyAnimInstance.h','Content/HypeWorld/Personagens/Shisui/ABP_Shisui.uasset','Content/HypeWorld/Animacoes/A_Pose_Prontidao_Base.uasset']
for p in targets:
    if (ROOT/p).exists():
        dst=BACK/p;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,dst)
protected=[]
for p in git('ls-files','-co','--exclude-standard','Content','Source','Config').decode().splitlines():
    if p in targets:continue
    f=ROOT/p
    if f.is_file():protected.append(dict(path=p,sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
(OUT/'protected_before.json').write_text(json.dumps(protected,indent=2))
animation_paths=git('ls-tree','-r','--name-only','f091ba9','Content/Ninja/Characters/Animations','Content/HypeWorld/Animacoes','Content/HypeWorld/Combate/Esquivas').decode().splitlines()
animation_paths+= [p for p in git('ls-tree','-r','--name-only','f091ba9','Content').decode().splitlines() if any(s in Path(p).stem for s in ['IK_','RTG_'])]
report=[]
for p in animation_paths:
    data=content('f091ba9',p)
    report.append(dict(path=p,available=data is not None,matches=data is not None and (ROOT/p).exists() and (ROOT/p).read_bytes()==data))
(OUT/'baseline_check.json').write_text(json.dumps(report,indent=2))
assert all(r['available'] for r in report)
print('BACKUP',len(targets),'PROTECTED',len(protected),'ANIMATION_ASSETS',len(report),'DIFFERENCES',[r['path'] for r in report if not r['matches']])
