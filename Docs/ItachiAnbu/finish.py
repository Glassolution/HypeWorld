from pathlib import Path
import json,hashlib,zlib
OUT=Path(__file__).parent;ROOT=OUT.parents[1]
checks=json.loads((OUT/'protected.json').read_text())
changed=[r['path'] for r in checks if hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()!=r['sha256']]
assert not changed,changed
validation=json.loads((OUT/'validation.json').read_text());assert validation['passed'] and len(validation['cases'])==5
validation['protected_assets_unchanged']=len(checks);validation['assembly']=json.loads((OUT/'assembly.json').read_text());(OUT/'verification.json').write_text(json.dumps(validation,indent=2))
data=(OUT/'parado.png').read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n';pos=8
while pos<len(data):
    n=int.from_bytes(data[pos:pos+4],'big');kind=data[pos+4:pos+8];chunk=data[pos+8:pos+8+n];assert zlib.crc32(kind+chunk)==int.from_bytes(data[pos+8+n:pos+12+n],'big');pos+=12+n
    if kind==b'IEND':break
(OUT/'Itachi_ANBU.png').write_bytes(data[:pos])
print('ITACHI_VALIDATED','PROTECTED_ASSETS',len(checks))
