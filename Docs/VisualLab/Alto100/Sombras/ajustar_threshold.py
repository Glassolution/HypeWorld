# Preserve the completed original control. Restart only the three candidate cases.
unreal.unregister_slate_post_tick_callback(CH);cmd('CsvProfile STOP')
(SOUT/'status_pre_correcao_threshold.json').write_text(json.dumps(CJ,indent=2))
source=(SOUT/'testar_cascatas.py').read_text()
exec('def apply_cs'+source.split('def apply_cs',1)[1].split('\nCE=[]')[0])
CE=[];CJ=[];CT0=time.perf_counter()
for index,case in enumerate(CS[1:]):
    t=index*135;CE.append((t,'select_'+case[0],lambda c=case:apply_cs(c)))
    for i in range(1,4):
        s=t+25+(i-1)*35
        CE.extend([(s,f'start_{case[0]}_{i}',lambda n=case[0],i=i:(cmd(f'CsvProfile STARTFILE=shadow_{n}_r{i}.csv'),cmd('CsvProfile START'))),(s+30,'stop',lambda:cmd('CsvProfile STOP'))])
    CE.append((t+128,'shot_'+case[0],lambda n=case[0]:cmd('Shot filename="'+str(SOUT/(n+'.png'))+'" -nosuffix')))
CH=unreal.register_slate_post_tick_callback(ctick)
