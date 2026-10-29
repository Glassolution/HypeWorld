import sys,time,os,json
from pathlib import Path
sys.path.insert(0,r'C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution as rex
expected=int((Path(__file__).parent/'game.pid').read_text())
r=rex.RemoteExecution();r.start();node=None
for _ in range(60):
    for candidate in list(r.remote_nodes):
        try:
            r.open_command_connection(candidate['node_id'])
            answer=r.run_command('import os; print("VISUALLAB_PID",os.getpid())',unattended=True,exec_mode=rex.MODE_EXEC_STATEMENT,raise_on_failure=False)
            if any(str(expected) in x.get('output','') for x in answer.get('output',[])):
                node=candidate['node_id'];break
            r.close_command_connection()
        except Exception:pass
    if node:break
    time.sleep(.5)
if not node:print('NO_MATCHING_GAME');r.stop();sys.exit(2)
if sys.argv[1]=='-c':code=sys.argv[2]
else:
    path=Path(sys.argv[1]).resolve();code=f'exec(compile(open({str(path)!r},encoding="utf-8-sig").read(),{str(path)!r},"exec"))'
answer=r.run_command(code,unattended=True,exec_mode=rex.MODE_EXEC_STATEMENT,raise_on_failure=False)
r.stop()
for out in answer.get('output',[]):print(out.get('output','').rstrip())
if not answer.get('success'):print('FAILED',answer.get('result'));sys.exit(1)
