"""Validate previously exported static assets for unchanged MPQ examples."""
import hashlib,math,struct as S
from pathlib import Path,PurePosixPath
from collections import Counter
import numpy as np
from PIL import Image

def require(ok,message):
 if not ok:raise ValueError(message)

def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def member(name):
 name=str(name).replace('\\','/');p=PurePosixPath(name)
 require(name and not p.is_absolute() and all(x not in ['.','..',''] for x in name.split('/')) and not any(c in name for c in ':"\r\n'), 'Unsafe archive path: '+name)
 return p.as_posix()

def array(b,head,stride):
 require(head+8<=len(b),'Truncated array header');n,o=S.unpack_from('<II',b,head)
 require(n==0 or o+n*stride<=len(b),'Array outside file')
 return n,o

def validate_stage(stage):
 stage=Path(stage);files={};models=[];textures=[]
 for p in stage.rglob('*'):
  if not p.is_file():continue
  rel=member(p.relative_to(stage));key=rel.lower();require(key not in files,'Case-insensitive path collision');files[key]=p
  require(p.suffix.lower() in ['.m2','.skin','.blp'],'Unexpected staged file: '+rel)
 require(files,'Empty archive')
 for key,p in files.items():
  b=p.read_bytes()
  if key.endswith('.blp'):
   require(len(b)>=148 and b[:4]==b'BLP2','Expected BLP2');kind=S.unpack_from('<I',b,4)[0];enc,alpha,ae,mips=S.unpack_from('<4B',b,8);w,h=S.unpack_from('<II',b,12)
   require(kind==1 and enc==2 and alpha==0 and ae==0 and mips==1,'Expected opaque BC1 with mipmaps')
   require(w>0 and h>0 and w&(w-1)==0 and h&(h-1)==0,'Non-power-of-two BLP')
   offsets=S.unpack_from('<16I',b,20);sizes=S.unpack_from('<16I',b,84);count=int(math.log2(max(w,h)))+1;require(count<=16,'Too many mipmaps');end=148
   for i in range(count):
    wi,hi=max(1,w>>i),max(1,h>>i);n=((wi+3)//4)*((hi+3)//4)*8;off=offsets[i]
    require(sizes[i]==n and off>=end and off+n<=len(b),'Invalid BLP mip range');end=off+n
    require(all(S.unpack_from('<H',b,j)[0]>S.unpack_from('<H',b,j+2)[0] for j in range(off,off+n,8)),'Non-four-color BC1 block')
   require(not any(offsets[count:]) and not any(sizes[count:]),'Unexpected mip entries')
   image=Image.open(p).convert('RGBA');require(image.getchannel('A').getextrema()==(255,255),'Unexpected alpha')
   textures.append(dict(path=key,width=w,height=h,mips=count))
  if not key.endswith('.m2'):continue
  require(b[:4]==b'MD20' and len(b)>=304 and S.unpack_from('<I',b,4)[0]==264,'Expected Wrath MD20 v264')
  nv,ov=array(b,60,48);require(nv>0,'No visible geometry');v=np.array([S.unpack_from('<3f',b,ov+i*48) for i in range(nv)]);norm=np.array([S.unpack_from('<3f',b,ov+i*48+20) for i in range(nv)])
  require(np.isfinite(v).all() and np.isfinite(norm).all() and np.allclose(np.linalg.norm(norm,axis=1),1,atol=1e-4),'Invalid vertices/normals')
  require(np.allclose(S.unpack_from('<3f',b,160),v.min(0),atol=2e-5) and np.allclose(S.unpack_from('<3f',b,172),v.max(0),atol=2e-5),'Visible bounds mismatch')
  require(S.unpack_from('<f',b,184)[0]+1e-4>=np.linalg.norm(v,axis=1).max(),'Visible radius too small')
  nt,ot=array(b,80,16);require(nt>0,'No textures')
  for i in range(nt):
   typ,flags,n,o=S.unpack_from('<4I',b,ot+i*16);require(typ==0 and o+n<=len(b),'Unsupported texture reference')
   ref=member(b[o:o+n].rstrip(b'\0').decode()).lower();require(ref in files,'Missing texture '+ref)
  ns,os_=array(b,28,64);require(ns>0,'No static sequence')
  for i in range(ns):
   require(np.allclose(S.unpack_from('<3f',b,os_+i*64+32),v.min(0),atol=2e-5) and np.allclose(S.unpack_from('<3f',b,os_+i*64+44),v.max(0),atol=2e-5),'Sequence bounds mismatch')
  skey=key[:-3]+'00.skin';require(skey in files,'Missing SKIN');sk=files[skey].read_bytes();require(sk[:4]==b'SKIN','Bad SKIN magic')
  ni,oi=array(sk,4,2);nf,of=array(sk,12,2);require(nf>0 and nf%3==0,'Invalid triangle count');lookup=S.unpack_from('<'+str(ni)+'H',sk,oi);idx=S.unpack_from('<'+str(nf)+'H',sk,of)
  require(ni>0 and max(lookup)<nv and max(idx)<ni,'SKIN index outside geometry')
  nsec,osec=array(sk,28,48)
  for i in range(nsec):
   sec=S.unpack_from('<10H',sk,osec+i*48);require(sec[2]+sec[3]<=ni and sec[4]+sec[5]<=nf,'Submesh range invalid')
  nb,ob=array(sk,36,24);require(nb>0,'No render batches')
  ci,co=array(b,216,2);cv,cvo=array(b,224,12);cn,cno=array(b,232,12);require(ci>0 and ci%3==0 and cn==ci//3 and cv>0,'Invalid collision arrays')
  cp=np.array([S.unpack_from('<3f',b,cvo+i*12) for i in range(cv)]);cf=list(S.iter_unpack('<3H',b[co:co+ci*2]));require(max(max(t) for t in cf)<cv,'Collision index invalid')
  require(np.allclose(S.unpack_from('<3f',b,188),cp.min(0),atol=2e-5) and np.allclose(S.unpack_from('<3f',b,200),cp.max(0),atol=2e-5),'Collision bounds mismatch')
  cnormal=np.array([S.unpack_from('<3f',b,cno+i*12) for i in range(cn)]);require(np.isfinite(cnormal).all() and np.allclose(np.linalg.norm(cnormal,axis=1),1,atol=1e-4),'Invalid collision normals')
  edges=Counter(tuple(sorted((f[i],f[(i+1)%3]))) for f in cf for i in range(3));require(set(edges.values())=={2},'Collision is not closed')
  models.append(dict(path=key,vertices=nv,triangles=nf//3,collision_triangles=ci//3,bounds_min=v.min(0).tolist(),bounds_max=v.max(0).tolist()))
 for key in files:
  if key.endswith('.skin'):require(key.endswith('00.skin') and key[:-7]+'.m2' in files,'Orphan/unsupported SKIN '+key)
 return dict(models=models,textures=textures,files=[dict(path=member(p.relative_to(stage)),bytes=p.stat().st_size,sha256=digest(p)) for p in files.values()])
