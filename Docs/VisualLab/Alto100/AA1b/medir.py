assert V['done']
for c in DIAG:(FRAMES/c[0]/'COMPLETE.json').write_text(json.dumps({'frames':90}))
TUNING=[('peso004',.04,1,True,True,[]),('peso010',.10,1,True,True,[]),('peso020',.20,1,True,True,[]),('peso030',.30,1,True,True,[]),('antighost_q3',.04,3,True,True,[])]
EVENTS=[];JOURNAL=[];ACTIVE=False;ROUTE0=0;T0=time.perf_counter()
cmd('t.OverrideFPS 0')
def begin(name,i):
    global ACTIVE,ROUTE0
    ACTIVE=True;ROUTE0=time.perf_counter();cmd(f'CsvProfile STARTFILE=aa1b_{name}_r{i}.csv');cmd('CsvProfile START')
def end():
    global ACTIVE
    cmd('CsvProfile STOP');ACTIVE=False;pose(0)
for index,case in enumerate(TUNING):
    base=index*130;EVENTS.append((base,'select_'+case[0],lambda c=case:choose(c)))
    for i in range(1,4):
        t=base+25+(i-1)*35
        EVENTS.extend([(t,f'start_{case[0]}_{i}',lambda n=case[0],i=i:begin(n,i)),(t+30,f'stop_{case[0]}_{i}',end)])
def btick(dt):
    try:
        elapsed=time.perf_counter()-T0
        if EVENTS and elapsed>=EVENTS[0][0]:
            _,label,fn=EVENTS.pop(0);fn();JOURNAL.append({'event':label,'time':elapsed});(OUT/'benchmark_status.json').write_text(json.dumps(JOURNAL,indent=2))
        if ACTIVE:pose(time.perf_counter()-ROUTE0)
        if not EVENTS:
            unreal.unregister_slate_post_tick_callback(BH);choose(DIAG[0]);pose(0);(OUT/'benchmark_concluido.json').write_text(json.dumps(state(),indent=2))
    except Exception:
        unreal.unregister_slate_post_tick_callback(BH);cmd('CsvProfile STOP');(OUT/'BENCH_ERRO.txt').write_text(traceback.format_exc())
BH=unreal.register_slate_post_tick_callback(btick)
