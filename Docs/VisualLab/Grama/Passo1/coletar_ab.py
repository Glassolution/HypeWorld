# Execute after setup_runtime.py in the same dedicated -game Python session.
import time,json,traceback
EVENTS=[]; JOURNAL=[]; T0=time.perf_counter()
def event(t,label,fn):EVENTS.append((t,label,fn))
for i,d in enumerate([.4,1.,.4,1.,.4,1.]):
    t=i*49.; name='grass_'+('40' if d==.4 else '100')+'_r'+str(i//2+1)
    event(t,'density_'+str(d),lambda d=d:density(d))
    event(t+15,'start_'+name,lambda name=name:profile(name))
    event(t+45,'stop_'+name,lambda:cmd('CsvProfile STOP'))
    if i<2:
        event(t+47,'shot_'+name,lambda name=name:shot(name))
event(294,'restore_grass40',lambda:density(.4))
PP=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.PostProcessVolume)[0]
TOON=unreal.load_asset('/Game/Maps/Tutorial/Tutorial_Assets/Materiais/MI_PP_Personagem')
ORIGINAL_BLENDABLES=str(PP.get_editor_property('settings').get_editor_property('weighted_blendables'))
event(310,'toon_on_shot',lambda:shot('indra_toon_ligado'))
event(313,'toon_off',lambda:PP.add_or_update_blendable(TOON,0.0))
event(323,'toon_off_shot',lambda:shot('indra_toon_desligado'))
event(326,'toon_restore',lambda:PP.add_or_update_blendable(TOON,1.0))
def tick(dt):
    global HANDLE
    try:
        elapsed=time.perf_counter()-T0
        while EVENTS and elapsed>=EVENTS[0][0]:
            target,label,fn=EVENTS.pop(0);fn()
            JOURNAL.append({'event':label,'elapsed':elapsed,'scheduled':target})
            (OUT/'coleta_status.json').write_text(json.dumps(JOURNAL,indent=2))
        if not EVENTS:
            unreal.unregister_slate_post_tick_callback(HANDLE)
            (OUT/'coleta_concluida.json').write_text(json.dumps({'state':state(),'blendables_original':ORIGINAL_BLENDABLES,'blendables_final':str(PP.get_editor_property('settings').get_editor_property('weighted_blendables'))},indent=2))
    except Exception:
        unreal.unregister_slate_post_tick_callback(HANDLE)
        cmd('CsvProfile STOP');PP.add_or_update_blendable(TOON,1.0)
        (OUT/'ERRO_COLETA.txt').write_text(traceback.format_exc())
HANDLE=unreal.register_slate_post_tick_callback(tick)
print('COLETA_INICIADA',len(EVENTS),'eventos; 3 x 30 s por densidade; depois toon A/B.')
