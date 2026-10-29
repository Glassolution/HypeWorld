from pathlib import Path
source=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/GIAO/preparar.py').read_text()
source=source.replace('Alto100/GIAO\')','Alto100/Integrado\')')
exec(source)
cmd('r.Lumen.FinalGatherMethod 0')
NAMES.extend(['r.Lumen.ScreenProbeGather.ShortRangeAO','r.Lumen.ScreenProbeGather.ShortRangeAO.DownsampleFactor','r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.SliceCount','r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.StepsPerSlice','r.AmbientOcclusionLevels','r.FidelityFX.FSR.Enabled','r.FidelityFX.FI.Enabled'])
def base_ao():
    for s in ['r.Lumen.ScreenProbeGather.ShortRangeAO 1','r.Lumen.ScreenProbeGather.ShortRangeAO.DownsampleFactor 2','r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.SliceCount 2','r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.StepsPerSlice 3']:cmd(s)
def choose_ao(name):
    base_ao()
    if name=='ao_leve':
        cmd('r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.SliceCount 1')
        cmd('r.Lumen.ScreenProbeGather.ShortRangeAO.HorizonSearch.StepsPerSlice 2')
    if name=='ao_off':cmd('r.Lumen.ScreenProbeGather.ShortRangeAO 0')
    (OUT/(name+'_estado.json')).write_text(json.dumps(state(),indent=2))
def start_suite(cases,selector,prefix='int'):
    global BENCH_EVENTS,BENCH_LOG,BENCH_START,BENCH_HANDLE
    BENCH_EVENTS=[];BENCH_LOG=[];BENCH_START=time.perf_counter()
    for ix,name in enumerate(cases):
        t=ix*145
        BENCH_EVENTS.append((t,'select_'+name,lambda n=name:selector(n)))
        for i in range(1,4):
            start=t+30+(i-1)*35
            BENCH_EVENTS.extend([(start,f'start_{name}_{i}',lambda n=name,i=i:(cmd(f'CsvProfile STARTFILE={prefix}_{n}_r{i}.csv'),cmd('CsvProfile START'))),(start+30,'stop',lambda:cmd('CsvProfile STOP'))])
        BENCH_EVENTS.append((t+133,'shot_'+name,lambda n=name:cmd('Shot filename="'+str(OUT/(n+'.png'))+'" -nosuffix')))
        BENCH_EVENTS.append((t+137,'profile_'+name,lambda n=name:(unreal.log('VISUALLAB_PROFILE_'+n),cmd('r.ProfileGPU.ShowUI 0'),cmd('ProfileGPU'))))
    def bench_tick(dt):
        try:
            elapsed=time.perf_counter()-BENCH_START
            if BENCH_EVENTS and elapsed>=BENCH_EVENTS[0][0]:
                _,label,fn=BENCH_EVENTS.pop(0);fn();BENCH_LOG.append({'event':label,'time':elapsed});(OUT/(prefix+'_status.json')).write_text(json.dumps(BENCH_LOG,indent=2))
            if not BENCH_EVENTS:
                unreal.unregister_slate_post_tick_callback(BENCH_HANDLE);(OUT/(prefix+'_concluido.json')).write_text(json.dumps({'done':True}))
        except Exception:
            unreal.unregister_slate_post_tick_callback(BENCH_HANDLE);cmd('CsvProfile STOP');(OUT/(prefix+'_ERRO.txt')).write_text(traceback.format_exc())
    BENCH_HANDLE=unreal.register_slate_post_tick_callback(bench_tick)
start_suite(['ao_atual','ao_leve','ao_off'],choose_ao,'ao')
