import unreal,json,traceback
from pathlib import Path
OUT=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/AnimacaoRestauro')
les=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not les.is_in_play_in_editor()
TIGER=unreal.load_asset('/Game/HypeWorld/Jutsus/Katon/Animacoes/A_Selo_Tigre')
EX={'case':-1,'frame':0,'rows':[]}
CASES=[('esquiva_frente',0,1),('esquiva_direita',1,0),('esquiva_tras',0,-1),('esquiva_esquerda',-1,0),('tigre',0,0)]
def extra_tick(dt):
    try:
        w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:return
        p=unreal.GameplayStatics.get_player_pawn(w,0)
        if not p:return
        pc=unreal.GameplayStatics.get_player_controller(w,0);m=p.get_editor_property('character_movement');a=p.get_editor_property('mesh').get_anim_instance()
        if EX['case']==-1:
            EX['case']=0
            unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 24')
            if not p.is_in_battle_mode():p.do_focus_start();p.do_focus_end()
        n=EX['frame'];name,right,forward=CASES[EX['case']]
        if n==0:
            p.do_sprint_end();p.do_move(0.,0.);m.stop_movement_immediately();p.set_actor_location(unreal.Vector(0,0,50146),False,True)
            p.set_actor_rotation(unreal.Rotator(),True);pc.set_control_rotation(unreal.Rotator())
        p.do_move(float(right) if n<49 else 0.,float(forward) if n<49 else 0.)
        if n==48:
            if name=='tigre':
                EX['tiger_result']=str(a.play_slot_animation_as_dynamic_montage(TIGER,'DefaultSlot',.1,.1,1.,1))
            else:p.do_dash()
        if 49<=n<72:
            montage=a.get_current_active_montage()
            EX['rows'].append({'case':name,'frame':n,'montage':montage.get_path_name() if montage else None,'speed':p.get_velocity().length(),'movement_mode':str(m.get_editor_property('movement_mode')),'custom_mode':m.get_editor_property('custom_movement_mode')})
        EX['frame']+=1
        if EX['frame']>=96:
            EX['frame']=0;EX['case']+=1
            if EX['case']>=len(CASES):
                checks=[]
                for label,_,_ in CASES:
                    rows=[r for r in EX['rows'] if r['case']==label]
                    checks.append({'case':label,'played':any(r['montage'] for r in rows),'montages':sorted({r['montage'] for r in rows if r['montage']})})
                (OUT/'extras_validation.json').write_text(json.dumps({'checks':checks,'rows':EX['rows'],'tiger_result':EX.get('tiger_result')},indent=2))
                unreal.unregister_slate_post_tick_callback(EXTRA_HANDLE);unreal.SystemLibrary.execute_console_command(w,'t.OverrideFPS 0');les.editor_request_end_play()
    except Exception:
        (OUT/'extras_error.txt').write_text(traceback.format_exc());unreal.unregister_slate_post_tick_callback(EXTRA_HANDLE);les.editor_request_end_play()
EXTRA_HANDLE=unreal.register_slate_post_tick_callback(extra_tick)
les.editor_request_begin_play()
print('EXTRAS_STARTED')
