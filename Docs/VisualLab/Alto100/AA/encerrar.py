assert V['done'] and not EVENTS
cmd('t.OverrideFPS 0')
NAMES.extend(['r.TSR.ReprojectionField','r.TSR.Resurrection','r.TSR.History.R11G11B10'])
final_states={}
for case in CASES:
    select(case);final_states[case[0]]=state()
(OUT/'estados_finais_apos_video.json').write_text(json.dumps(final_states,indent=2))
select(CASES[0])
cmd('quit')
