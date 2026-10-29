assert not CE
PE=[];PJ=[];PT0=time.perf_counter();cmd('r.ProfileGPU.ShowUI 0')
for i,c in enumerate(CS):
    PE.extend([(i*6,'apply_'+c[0],lambda c=c:apply_cs(c)),(i*6+4,'profile_'+c[0],lambda n=c[0]:(unreal.log('VISUALLAB_PROFILE_'+n),cmd('ProfileGPU')))])
def ptick(dt):
    elapsed=time.perf_counter()-PT0
    if PE and elapsed>=PE[0][0]:
        _,name,fn=PE.pop(0);fn();PJ.append(name)
    if not PE:
        unreal.unregister_slate_post_tick_callback(PH)
        (SOUT/'perfis_concluidos.json').write_text(json.dumps(PJ))
PH=unreal.register_slate_post_tick_callback(ptick)
