from PIL import Image, ImageDraw
import numpy as np, colorsys, json, shutil
from pathlib import Path
out=Path(__file__).parent
names=['b6fa5f6e-82d8-42fe-9249-24711fb8d177','899e9824-1523-43cb-82c3-64290b534e69','838b64dd-e11c-447c-bad1-df0516625daf','447524d0-d43c-4c15-b434-1e98762f074a']
imgs=[]
for i,n in enumerate(names,1):
    p=Path('C:/Users/lucas/AppData/Local/Temp')/('codex-clipboard-'+n+'.png')
    shutil.copy2(p,out/f'imagem_{i}.png'); imgs.append(Image.open(p).convert('RGB'))
# Rectangles in source pixels; medians avoid a handful of highlights. No inferred engine parameters.
regions=[(1,'grama_sol',(870,565,1250,760)),(1,'arbusto_menta',(350,550,440,595)),(1,'tunica',(715,550,765,675)),(2,'grama_sol',(320,450,575,650)),(2,'grama_sombra',(1160,380,1370,440)),(2,'montanhas',(620,78,900,125)),(3,'pele_braco',(612,340,650,363)),(3,'roupa',(642,373,685,440)),(3,'montanhas',(950,173,1170,265)),(4,'pele_rosto',(690,220,731,251)),(4,'roupa_azul',(605,350,775,475)),(4,'calca',(613,630,770,720))]
rows=[]
for i,label,box in regions:
    a=np.asarray(imgs[i-1].crop(box))/255.; flat=a.reshape(-1,3)
    hsv=np.array([colorsys.rgb_to_hsv(*x) for x in flat]); lin=np.where(flat<=.04045,flat/12.92,((flat+.055)/1.055)**2.4)
    rows.append(dict(image=i,region=label,xyxy=box,rgb_median=np.round(np.median(flat,axis=0)*255).astype(int).tolist(),h_deg=round(float(np.median(hsv[:,0])*360),1),s_pct=round(float(np.median(hsv[:,1])*100),1),v_pct=round(float(np.median(hsv[:,2])*100),1),linear_luminance_pct=round(float(np.median(lin@np.array([.2126,.7152,.0722]))*100),1)))
(out/'medicoes.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
board=Image.new('RGB',(1600,980),'#181c20'); d=ImageDraw.Draw(board)
for idx,im in enumerate(imgs):
    x=(idx%2)*800; y=(idx//2)*490; im=im.copy(); im.thumbnail((790,450)); board.paste(im,(x,y+30)); d.text((x+12,y+8),f'Imagem {idx+1} - '+('atual fornecida' if idx==0 else 'referencia Zenkai'),fill='white')
board.save(out/'comparacao_fontes.jpg',quality=95)
print(json.dumps(rows,ensure_ascii=False))
