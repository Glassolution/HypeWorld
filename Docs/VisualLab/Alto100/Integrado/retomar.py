from pathlib import Path
D=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/VisualLab/Alto100/Integrado')
exec((D/'preview.py').read_text())
BENCH_EVENTS=[]
exec((D/'testar_fsr.py').read_text().split("start_suite(['tsr_q1'")[0])
def select_final(name):
    choose_veg('copas_opacas' if name=='copas_opacas' else 'controle')
    choose_fsr('fsr_native' if name in ['fsr_native','combinacao'] else 'tsr_q1')
    if name=='combinacao':set_bush(True,True)
    (OUT/(name+'_estado.json')).write_text(json.dumps(state(),indent=2))
start_suite(['tsr_q1','copas_opacas','fsr_native','combinacao'],select_final,'final')
PHASE={'recording':False}
def next_phase(dt):
    if BENCH_EVENTS:return
    if not PHASE['recording']:
        PHASE['recording']=True
        choose_veg('controle')
        exec((D/'gravar_fsr.py').read_text(),globals())
    elif V['done']:
        unreal.unregister_slate_post_tick_callback(NEXT_HANDLE)
        choose_veg('controle');choose_fsr('tsr_q1')
        (OUT/'retomada_concluida.json').write_text(json.dumps({'done':True}))
NEXT_HANDLE=unreal.register_slate_post_tick_callback(next_phase)
