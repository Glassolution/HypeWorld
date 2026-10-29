from pathlib import Path
script=Path(r'C:/Users/lucas/Downloads/Hype World/HypeWorld/Docs/ItachiAnbu/validate.py').read_text(encoding='utf-8')
script=script.replace("CASES=[('parado',False,False,True),('guarda',False,False,True),('trote',True,False,False),('corrida_ninja',True,True,False),('tigre',False,False,True)]","CASES=[('rosto',False,False,True)]")
script=script.replace("'validation.json'","'validation_rosto.json'")
script=script.replace('target=p.get_actor_location()+unreal.Vector(0,0,0);eye=target+unreal.Vector(370 if front else -390,100,90 if front else 140)','target=p.get_actor_location()+unreal.Vector(0,0,72);eye=target+unreal.Vector(165,0,10)')
script=script.replace("'fov_angle',40.","'fov_angle',25.")
exec(compile(script,'itachi_face_check','exec'),{'__name__':'itachi_face_check'})
