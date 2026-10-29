BODY=next(c for c in P.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=='Indra')
SE=[(i*2, i) for i in range(2,7)];ST=time.perf_counter();SI=0;HIDDEN=None;PENDING=None
def slot_tick(dt):
    global SI,HIDDEN,PENDING
    t=time.perf_counter()-ST
    if SI<5 and t>=SI*2:
        if HIDDEN is not None:BODY.show_material_section(HIDDEN,HIDDEN,True,0)
        HIDDEN=SI+2;BODY.show_material_section(HIDDEN,HIDDEN,False,0)
        PENDING=(t+.8,HIDDEN);SI+=1
    if PENDING and t>=PENDING[0]:
        cmd('Shot filename="'+str(OUT/f'sem_slot_{PENDING[1]}.png')+'" -nosuffix');PENDING=None
    if SI==5 and not PENDING and t>10:
        BODY.show_material_section(HIDDEN,HIDDEN,True,0);unreal.unregister_slate_post_tick_callback(SH)
SH=unreal.register_slate_post_tick_callback(slot_tick)
props={}
for path in ['/Game/HypeWorld/Personagens/Indra/M_Indra','/Game/Ninja/VFX/Materials/M_ChakraFeetShell','/Game/Maps/Tutorial/Tutorial_Assets/Materiais/M_PP_Personagem']:
    m=unreal.load_asset(path);data={}
    for n in ['blend_mode','blendable_location','enable_responsive_aa','output_translucent_velocity','translucency_pass','shading_model']:
        try:data[n]=str(m.get_editor_property(n))
        except Exception:pass
    props[path]=data
(OUT/'materiais_propriedades.json').write_text(json.dumps(props,indent=2))
