def native_curves(pose,obj=None):
    obj=obj or mesh
    for side in ('r','l'):
        for d in ('thumb','index','middle','ring','pinky'):
            ratio=0
            for j in (1,2,3):
                n=f'{d}_{j:02d}_{side}'
                direction=ref[f'{d}_{j+1:02d}_{side}'][0]-ref[n][0] if j<3 else ref[n][0]-ref[f'{d}_02_{side}'][0]
                axis=ref[n][1].inverted()@direction.normalized().cross(Vector((0,0,-1))).normalized()
                delta=bind[n][1].inverted()@pose[n][1]
                if delta.w<0:delta=-delta
                angle=math.degrees(2*math.atan2(Vector((delta.x,delta.y,delta.z)).dot(axis),delta.w))
                full=([15,45,65] if d=='thumb' else [65,90,60])[j-1]
                ratio+=min(1,max(0,angle/full))/3
            obj.data.shape_keys.key_blocks[f'Corr_{d}_{side}'].value=min(1,max(0,(ratio-.45)/.55))
