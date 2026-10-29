assert state()['r.AntiAliasingMethod']=='2'
pose(0)
cmd('Shot filename="'+str(OUT/'taa.png')+'" -nosuffix')
