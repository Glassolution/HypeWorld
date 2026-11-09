"""Verifica notas e links sem abrir editores ou executar código do jogo."""
from pathlib import Path
import json,re,sys,hashlib
from urllib.parse import unquote
V=Path(__file__).resolve().parents[1];WS=V.parent
errors=[];rows=[];all_md=list(V.rglob('*.md'))
def fail(message):errors.append(message)
def outside_fences(text):
 out=[];fence=None
 for line in text.splitlines():
  match=re.match(r'^(`{3,}|~{3,})',line)
  if match:
   token=match.group(1)
   if fence is None:fence=token
   elif token[0]==fence[0] and len(token)>=len(fence):fence=None
   continue
  if fence is None:out.append(line)
 return '\n'.join(out)
def wiki_target(raw,origin):
 target=raw.split('|')[0].split('#')[0]
 if not target:return origin
 options=[origin.parent/(target+'.md'),V/(target+'.md')]
 for p in options:
  if p.is_file():return p
 matches=[p for p in all_md if p.stem==target]
 return matches[0] if len(matches)==1 else None
for p in all_md:
 text=p.read_text();rel=str(p.relative_to(V));archive='Arquivo' in p.relative_to(V).parts
 max_chars,max_lines=(6000,120) if archive else ((1600,40) if p.name=='00-Inicio.md' else (4000,80))
 if len(text)>max_chars or len(text.splitlines())>max_lines:fail(f'Tamanho excedido: {rel}')
 if not re.match(r'^---\nstatus: .+\natualizado: \d{4}-\d{2}-\d{2}\n---\n',text):fail(f'Metadados ausentes: {rel}')
 summary=re.findall(r'^> .+',outside_fences(text),re.M)
 if not 2<=len(summary)<=3:fail(f'Resumo deve ter 2–3 linhas: {rel}')
 body=outside_fences(text)
 if not archive and re.search(r'!\[|!\[\[',body):fail(f'Mídia embutida: {rel}')
 links=re.findall(r'(?<!!)\[\[([^\]]+)\]\]',body)
 if not links:fail(f'Sem relacionadas: {rel}')
 for target in links:
  if wiki_target(target,p) is None:fail(f'Wikilink inválido: {rel} → {target}')
 for match in re.finditer(r'(?<!!)\[[^\]]+\]\((?:<([^>]+)>|([^\)]+))\)',body):
  target=(match.group(1) or match.group(2)).split('#')[0]
  if not target or re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:',target):continue
  resolved=Path(unquote(target));resolved=resolved if resolved.is_absolute() else p.parent/resolved
  if not resolved.exists():fail(f'Link de arquivo inválido: {rel} → {target}')
 rows.append({'file':rel,'chars':len(text),'lines':len(text.splitlines()),'tokens_est':len(text)/4,'archive':archive})
index=(V/'00-Inicio.md').read_text()
expected={p for p in all_md if 'Arquivo' not in p.relative_to(V).parts and p.name!='00-Inicio.md'}
seen=[wiki_target(raw,V/'00-Inicio.md') for raw in re.findall(r'^- \[\[([^\]]+)\]\]',index,re.M)]
if set(seen)!=expected or len(seen)!=len(expected):fail('Índice não tem exatamente uma linha por nota ativa.')
archive=V/'Arquivo/2026-11-08-migracao';manifest_path=archive/'Originais.json'
if manifest_path.exists():
 manifest=json.loads(manifest_path.read_text())
 for source in manifest['sources']:
  content=''
  for piece in source['pieces']:
   t=(archive/piece['file']).read_text()
   payload=t.split(manifest['payload_prefix'],1)[1].split(manifest['payload_suffix'],1)[0]
   if hashlib.sha256(payload.encode()).hexdigest()!=piece['sha256']:fail('Trecho histórico alterado: '+piece['file'])
   content+=payload
  if hashlib.sha256(content.encode()).hexdigest()!=source['sha256']:fail('Reconstrução divergente: '+source['id'])
  # Backups físicos WS podem estar só no Mac; os trechos e hashes versionados são sempre verificados.
  backup=Path(source['backup'])
  if backup.is_file() and hashlib.sha256(backup.read_bytes()).hexdigest()!=source['sha256']:fail('Backup divergente: '+source['id'])
report={'ok':not errors,'active_notes':sum(not r['archive'] for r in rows),'archive_notes':sum(r['archive'] for r in rows),'max_active_chars':max((r['chars'] for r in rows if not r['archive']),default=0),'errors':errors,'notes':rows}
if '--json' in sys.argv:print(json.dumps(report,ensure_ascii=False,indent=2))
else:print(json.dumps({k:v for k,v in report.items() if k!='notes'},ensure_ascii=False,indent=2))
sys.exit(1 if errors else 0)
