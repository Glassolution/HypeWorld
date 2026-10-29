assert not PE and not CE and not SEVENTS and not EVENTS and V['done']
apply_cs(CS[0]);choose(DIAG[0]);pose(0)
verify={'shadows_restored':all(bool(c.get_editor_property('cast_shadow'))==v for c,v in ORIG_SH.items()),'fx_hidden_restored':all(c.get_editor_property('hidden_in_game')==v for c,v in FX_ORIGINAL.items()),'light_distance_restored':float(LIGHT.get_editor_property('dynamic_shadow_distance_movable_light'))==L_ORIG,'toon':str(PP.get_editor_property('settings').get_editor_property('weighted_blendables')),'time_override':unreal.SystemLibrary.get_console_variable_string_value('t.OverrideFPS')}
assert verify['shadows_restored'] and verify['fx_hidden_restored'] and verify['light_distance_restored'] and verify['time_override']=='0'
(OUT/'restauracao_runtime.json').write_text(json.dumps(verify,indent=2))
cmd('quit')
