from pathlib import Path
import json,math
import numpy as np
from PIL import Image,ImageFilter,ImageDraw
P=Path(__file__).resolve().parent;ROOT=P.parents[2];FR=ROOT/'Saved/VisualLab/Alto60Frames_v2'
clips=['frente_viradas','strafe_re','camera_grama','parada'];metrics={};candidates=[]
for clip in clips:
    rows=[];prev={}
    for i in range(360):
        pair={}
        for profile in ['base','alto60']:
            f=FR/profile/clip/f'{i:04d}.png'
            if not f.exists():raise SystemExit('frames incomplete')
            im=Image.open(f).convert('RGB');a=np.asarray(im.crop((680,180,1240,900)),dtype=np.float32)
            pair[profile]=a
        base=pair['base'];agg=pair['alto60']
        # Candidate detection only: extra bright pixels around white clothing edges.
        mask=((base.min(2)>150)&((base.max(2)-base.min(2))<65))
        mi=Image.fromarray((mask*255).astype('uint8'))
        outer=np.asarray(mi.filter(ImageFilter.MaxFilter(17)))>0
        band=outer & ~mask
        excess=np.maximum(agg.mean(2)-base.mean(2)-18,0)
        score=float(excess[band].mean()) if band.any() else 0.
        row={'frame':i,'halo_candidate_score':score}
        for profile,a in pair.items():
            if profile in prev:row[profile+'_temporal_delta']=float(np.abs(a-prev[profile]).mean())
            prev[profile]=a
        rows.append(row)
    metrics[clip]=rows
    if clip!='camera_grama':
        ordered=sorted(rows,key=lambda r:r['halo_candidate_score'],reverse=True)
        picked=[]
        for row in ordered:
            if 30<row['frame']<340 and all(abs(row['frame']-x)>24 for x in picked):picked.append(row['frame'])
            if len(picked)==3:break
        for frame in picked:candidates.append({'clip':clip,'frame':frame,'score':rows[frame]['halo_candidate_score']})
(P/'analise_2880_quadros.json').write_text(json.dumps({'frames_reviewed_algorithmically':2880,'metric_is_not_proof_of_ghosting':True,'metrics':metrics,'candidates':candidates},indent=2))
# Consecutive-frame review around each candidate, retaining native crop pixels.
for clip in ['frente_viradas','strafe_re','parada']:
    frame=max((c for c in candidates if c['clip']==clip),key=lambda x:x['score'])['frame']
    indices=[frame-2,frame-1,frame,frame+1]
    sheet=Image.new('RGB',(1120,4*520),(16,20,24));draw=ImageDraw.Draw(sheet)
    for r,i in enumerate(indices):
        for col,name in enumerate(['base','alto60']):
            crop=Image.open(FR/name/clip/f'{i:04d}.png').crop((680,260,1240,756))
            sheet.paste(crop,(col*560,r*520+24));draw.text((col*560+8,r*520+6),f'{name} / {clip} / {i}',fill='white')
    sheet.save(P/(clip+'_candidatos.png'))
print('reviewed 2880 frames; candidate sheets saved')

