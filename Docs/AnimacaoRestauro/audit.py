from pathlib import Path
import subprocess,re,hashlib,json
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).parent
def git(*args): return subprocess.check_output(['git',*args],cwd=ROOT)
def content(commit,path):
    data=git('show',f'{commit}:{path}')
    if data.startswith(b'version https://git-lfs'):
        oid=re.search(rb'oid sha256:(\w+)',data)[1].decode()
        obj=ROOT/'.git/lfs/objects'/oid[:2]/oid[2:4]/oid
        if not obj.exists(): return None
        data=obj.read_bytes()
        assert hashlib.sha256(data).hexdigest()==oid
    return data
if __name__=='__main__':
    paths=git('ls-tree','-r','--name-only','HEAD','Content/HypeWorld/Animacoes','Content/Ninja/Characters/Animations','Content/HypeWorld/Jutsus/Katon/Animacoes','Content/HypeWorld/Personagens','Content/Characters/Mannequins').decode().splitlines()
    rows=[]
    for path in paths:
        if not any(s in path for s in ['ABP_','Prontidao','BS_','IK_','RTG_','Selo_Tigre','SKEL_']):continue
        data=content('HEAD',path); old=content('e3f369d',path) if git('ls-tree','e3f369d','--',path) else None
        now=(ROOT/path).read_bytes() if (ROOT/path).exists() else None
        rows.append(dict(path=path,head_available=data is not None,local_matches_head=now==data,head_matches_e3=old==data,head_size=len(data or b''),markers=[m for m in ['LayeredBoneBlend','TwoBoneIK','NinjaBodyAnimInstance','A_Pose_Prontidao','A_Selo_Tigre'] if m.encode() in (data or b'')],local_markers=[m for m in ['LayeredBoneBlend','TwoBoneIK','NinjaBodyAnimInstance','A_Pose_Prontidao','A_Selo_Tigre'] if m.encode() in (now or b'')]))
    (OUT/'audit.json').write_text(json.dumps(rows,indent=2))
    print(json.dumps(rows,indent=2))
