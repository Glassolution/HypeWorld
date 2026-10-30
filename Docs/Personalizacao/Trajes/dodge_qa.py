import unreal,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
OUT=Path('/private/tmp/hype-fitting');s={'t':0.,'stage':0,'trace':[],'captures':0};unreal._hype_dodge_qa=s
ed=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ed.editor_request_begin_play()
def finish():
 (OUT/'dodge_validation.json').write_text(json.dumps(s['trace'],indent=2));unreal.unregister_slate_post_tick_callback(s['handle']);ed.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(dt):
 try:
  s['t']+=dt
  if s['stage']==0:
   w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
   if not w or s['t']<6:return
   pc=unreal.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
   if not isinstance(p,unreal.HypeWorldCharacter):return
   for ui in list(unreal.ObjectIterator(unreal.HypeCharacterSelectWidget)):
    if ui.get_owning_player()==pc:ui.remove_from_parent()
   pc.reset_ignore_move_input();pc.reset_ignore_look_input();pc.set_view_target_with_blend(p,0);p.set_actor_hidden_in_game(False)
   a=p.get_component_by_class(unreal.HypeAppearanceComponent);v=unreal.HypeAppearance();v.set_editor_property('bCustomized',True);v.name='Teste';v.hair_id='Hair_minato';v.outfit_id='Uchiha_Blue_Shinobi_Attire';v.hair_color=unreal.LinearColor(.8,.59,.23,1);a.call_method('ServerCommit',(v,))
   s.update(world=w,pawn=p,move=p.get_movement_component(),stage=1,t=0.,direction=0)
   s['trace'].append({'definitions':[str(x) for x in p.get_editor_property('combat_dodges')]})
  elif s['stage']==1:
   if s['t']<1:return
   directions=[(0,1),(1,0),(0,-1),(-1,0)];r,f=directions[s['direction']]
   s['pawn'].do_sprint_end();s['pawn'].do_move(r,f);s['pawn'].do_dash();s['stage']=2;s['t']=0.;s['captured']=False
  elif s['stage']==2:
   m=s['move'];active=m.is_dashing();combat=m.is_combat_dodging()
   s['trace'].append({'direction':s['direction'],'time':round(s['t'],3),'running':m.is_running(),'sprinting':m.is_sprinting(),'ground':m.is_moving_on_ground(),'dash':active,'combat_dodge':combat})
   if active and not s['captured']:
    unreal.SystemLibrary.execute_console_command(s['world'],'Shot showui filename=/private/tmp/hype-fitting/qa-shots/15_esquiva_'+str(s['direction'])+'.png');s['captured']=True;s['captures']+=1
   if s['t']>1.2:
    s['direction']+=1
    if s['direction']==4:s['trace'].append({'directions_captured':s['captures']});finish();return
    s['stage']=1;s['t']=0.
 except Exception:
  (OUT/'dodge_error.txt').write_text(traceback.format_exc());finish()
s['handle']=unreal.register_slate_post_tick_callback(tick)
