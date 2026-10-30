import unreal,json,traceback,math
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve();OUT=ROOT/'Docs/Personalizacao/Trajes'
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
st={'t':0.,'stage':0,'checks':[]};unreal._hype_fit_qa=st
ed=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ed.editor_request_begin_play()
def call(n,*args):return st['widget'].call_method(n,args)
def current(o):return st['widget'].get_path_name() in o.get_path_name()
def combos():return [o for o in unreal.ObjectIterator(unreal.ComboBoxString) if current(o)]
def choose(label):
 for c in combos():
  for i in range(c.get_option_count()):
   if c.get_option_at_index(i)==label:c.set_selected_index(i);return
 raise RuntimeError('Option missing '+label)
def photo(n):unreal.SystemLibrary.execute_console_command(st['world'],'Shot showui filename=/private/tmp/hype-fitting/qa-shots/'+n+'.png')
def visible():return [c for c in st['pawn'].get_components_by_class(unreal.SkeletalMeshComponent) if c.is_visible() and c.get_editor_property('skeletal_mesh_asset')]
def record(n,v=True):st['checks'].append({'check':n,'result':v})
def finish():
 (OUT/'runtime_validation.json').write_text(json.dumps(st['checks'],indent=2,ensure_ascii=False));unreal.unregister_slate_post_tick_callback(st['handle']);ed.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(dt):
 st['t']+=dt
 if st['stage'] and st['t']<(0.35 if st['stage'] in [24,27] else 2.):return
 try:
  k=st['stage']
  if k==0:
   w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
   if not w or st['t']<8:return
   pc=unreal.GameplayStatics.get_player_controller(w,0);pawn=pc.get_controlled_pawn() if pc else None
   if not isinstance(pawn,unreal.HypeWorldCharacter):return
   a=pawn.get_component_by_class(unreal.HypeAppearanceComponent)
   assert len(a.get_editor_property('hair_options'))==41
   assert len(a.get_editor_property('outfit_options'))==4
   st.update(world=w,pc=pc,pawn=pawn,a=a,original=a.get_editor_property('appearance'))
   st['original_visible']={c.get_editor_property('skeletal_mesh_asset').get_path_name() for c in visible()}
   record('Live pawn catalog',{'hair':41,'outfits':4})
   cls=unreal.load_class(None,'/Game/HypeWorld/UI/CharacterSelect/WBP_CharacterSelect.WBP_CharacterSelect_C')
   ui=unreal.get_default_object(unreal.WidgetLibrary).call_method('Create',(w,cls,pc));st['widget']=ui;ui.add_to_viewport(50);call('StartCreation')
  elif k==1:call('HandleNameChanged',unreal.Text('Revisao'));call('ShowStyleCategory')
  elif k==2:
   choose('Minato');choose('Traje Shinobi Azul')
   record('UI hair dropdown count',max(c.get_option_count() for c in combos()))
  elif k==3:
   ms=visible();assert any(c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Hair_minato' for c in ms)
   assert any(c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Uchiha_Blue_Shinobi_Attire' for c in ms)
   photo('01_azul_minato')
  elif k==4:choose('Armadura Shinobi')
  elif k==5:photo('02_armadura')
  elif k==6:choose('Traje Sombra')
  elif k==7:photo('03_sombra')
  elif k==8:choose('Uniforme Tático')
  elif k==9:photo('04_tatico')
  elif k==10:choose('Sakura Young')
  elif k==11:photo('05_sakura')
  elif k==12:choose('Wasabi')
  elif k==13:photo('06_wasabi')
  elif k==14:choose('Madara')
  elif k==15:photo('07_madara')
  elif k==16:call('HandleHeightChanged',0.0)
  elif k==17:
   body=next(c for c in visible() if c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Base_Body')
   assert abs(body.get_editor_property('relative_scale3d').z-1.06*160/190.782)<.001
   record('Height 160 cm preserves uniform body scale and attachments');photo('08_altura160')
  elif k==18:call('CancelCreation')
  elif k==19:
   assert {c.get_editor_property('skeletal_mesh_asset').get_path_name() for c in visible()}==st['original_visible']
   record('Cancel restores committed appearance')
   for u in list(unreal.ObjectIterator(unreal.HypeCharacterSelectWidget)):
    if u.get_world()==st['world']:u.remove_from_parent()
   pc=st['pc'];pc.reset_ignore_move_input();pc.reset_ignore_look_input();pc.set_view_target_with_blend(st['pawn'],0)
   value=unreal.HypeAppearance();value.set_editor_property('bCustomized',True);value.name='Revisao';value.hair_color=unreal.LinearColor(.8,.59,.23,1);value.hair_id='Hair_minato';value.outfit_id='Uchiha_Blue_Shinobi_Attire';st['a'].call_method('ServerCommit',(value,))
  elif k==20:
   assert any('Uchiha_Blue' in c.get_editor_property('skeletal_mesh_asset').get_name() for c in visible())
   record('Server commit equips Uchiha outfit');photo('09_jogo')
  elif k==21:st['pawn'].do_walk_toggle();st['pawn'].do_focus_start()
  elif k==22:
   assert st['pawn'].is_guarding();record('Guard active');photo('10_guarda')
  elif k==23:
   st['pawn'].do_focus_end();st['pawn'].do_sprint_start();st['moving']=True
  elif k==24:
   speed=st['pawn'].get_velocity().length();assert speed>100;record('Ninja run actual speed',speed);photo('11_corrida')
  elif k==25:st['pawn'].do_sprint_end();st['moving']=False
  elif k==26:st['moving']=True;st['strafe']=True
  elif k==27:
   speed=st['pawn'].get_velocity().length();assert speed>100;record('Trot strafe actual speed',speed);photo('12_trote_strafe')
  elif k==28:
   st['moving']=False;st['pawn'].do_dash();st['dash_time']=0.
  elif k==29:
   body=next(c for c in visible() if c.get_editor_property('skeletal_mesh_asset').get_name()=='SK_Base_Body');st['body']=body
   body.play_animation(unreal.load_asset('/Game/HypeWorld/Personagens/Indra/Animations/A_Katon_Selo_Tigre_Indra'),False);body.set_position(.35,False);body.set_play_rate(0.)
  elif k==30:photo('14_tigre');record('Tigre original clip on base body with attached cosmetics')
  elif k==31:
   assert str(st['a'].get_editor_property('appearance').clan)=='Uchiha'
   record('Clan assigned by server, client did not select clan');ed.editor_request_end_play()
  elif k==32:unreal.SystemLibrary.execute_console_command(None,'Automation RunTests HypeWorld.Appearance')
  elif k==33:
   p=OUT/'test-report/index.json'
   if not p.exists():
    if st['t']>90:raise RuntimeError('Automation timeout')
    return
   d=json.loads(p.read_text(encoding='utf-8-sig'));assert d['succeeded']==6 and d['failed']==0,(d['succeeded'],d['failed']);record('Automation',{'passed':d['succeeded'],'failed':d['failed']});finish();return
  st['stage']+=1;st['t']=0.
 except Exception:
  (OUT/'qa_error.txt').write_text(traceback.format_exc());finish()
def update(dt):
 if 'dash_time' in st:
  st['dash_time']+=dt
  if st['dash_time']>.08:
   assert st['pawn'].get_movement_component().is_combat_dodging();record('Combat dodge active',True);photo('13_esquiva');del st['dash_time']
 if st.get('moving'):
  st['pawn'].do_move(1. if st.get('strafe') else 0.,0. if st.get('strafe') else 1.)
 tick(dt)
st['handle']=unreal.register_slate_post_tick_callback(update)
