import sys,time,os
sys.path.insert(0,r'C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution as rex
r=rex.RemoteExecution();r.start()
for _ in range(30):
    if r.remote_nodes:break
    time.sleep(.5)
target=int(sys.argv[1]);selected=False
for node in list(r.remote_nodes):
    try:
        r.open_command_connection(node['node_id'])
        result=r.run_command("import os; print('EDITOR_PID',os.getpid())",unattended=True,exec_mode=rex.MODE_EXEC_STATEMENT,raise_on_failure=False)
        pidline=' '.join(o.get('output','') for o in result.get('output',[]))
        print(pidline.strip())
        if f'EDITOR_PID {target}' in pidline:selected=True;break
        r.close_command_connection()
    except Exception as e:print(type(e).__name__);r.close_command_connection()
if not selected:r.stop();raise SystemExit('TARGET_EDITOR_NOT_FOUND')
if sys.argv[2]=='-c':code=sys.argv[3]
else:
    path=os.path.abspath(sys.argv[2]).replace('\\','/');code=f"exec(compile(open({path!r},encoding='utf-8').read(),{path!r},'exec'))"
result=r.run_command(code,unattended=True,exec_mode=rex.MODE_EXEC_STATEMENT,raise_on_failure=False)
for o in result.get('output',[]):print(o.get('output','').rstrip())
r.stop()
if not result.get('success'):raise SystemExit(result.get('result'))
