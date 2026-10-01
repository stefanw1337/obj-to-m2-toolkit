"""Static opaque OBJ -> Wrath M2/SKIN/BLP -> MPQ. No client installation."""
import argparse, hashlib, json, math, os, shutil, struct as S, subprocess, sys
from pathlib import Path, PurePosixPath
from collections import Counter
import numpy as np
from PIL import Image
import mpyq
from textures import texture
from new_tree_collision import fitted_collision
HERE=Path(__file__).resolve().parent.parent

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

def measure_m2(path):
 b=Path(path).read_bytes();require(len(b)>=304 and b[:4]==b'MD20' and S.unpack_from('<I',b,4)[0]==264,'Expected Wrath M2 version 264')
 n,o=array(b,60,48);require(n>0,'No vertices');v=np.array([S.unpack_from('<3f',b,o+i*48) for i in range(n)]);require(np.isfinite(v).all(),'Nonfinite geometry')
 return dict(vertices=n,min=v.min(0).tolist(),max=v.max(0).tolist(),dimensions=np.ptp(v,axis=0).tolist(),note='Raw model-space vertex bounds; world placement scale is not applied')

def inspect_obj(path):
 v=[];f=[];uv=0;norm=0
 for line in Path(path).read_text(encoding='utf-8-sig').splitlines():
  w=line.split()
  if not w:continue
  if w[0]=='v':v.append(list(map(float,w[1:4])))
  elif w[0]=='vt':uv+=1
  elif w[0]=='vn':norm+=1
  elif w[0]=='f':f.append(w[1:])
 require(v and f,'OBJ needs vertices and faces');a=np.array(v);require(a.shape[1]==3 and np.isfinite(a).all(),'Invalid coordinates')
 return dict(vertices=len(v),faces=len(f),triangles=sum(len(x)-2 for x in f),face_sizes=sorted(set(map(len,f))),uv_entries=uv,normal_entries=norm,min=a.min(0).tolist(),max=a.max(0).tolist())

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

def verify_mpq(mpq,stage):
 report=validate_stage(stage)
 with Path(mpq).open('rb') as handle:
  a=mpyq.MPQArchive(handle);expected={f['path'].replace('/','\\').lower() for f in report['files']}
  listed={x.decode().replace('/','\\').lower() for x in a.files};actual={x for x in listed if not x.startswith('(')}
  require(actual==expected,'MPQ member set differs from staged files')
  for f in report['files']:require(a.read_file(f['path'].replace('/','\\'))==(Path(stage)/f['path']).read_bytes(),'MPQ data mismatch: '+f['path'])
 report.update(mpq_sha256=digest(mpq),archive_members_verified=len(expected),client_test='Not established by file validation')
 return report

def pack(stage,mpq):
 stage=Path(stage).resolve();mpq=Path(mpq).resolve();require(not mpq.exists(),'Output MPQ already exists');report=validate_stage(stage)
 require(stage!=mpq.parent and stage not in mpq.parents,'Keep output MPQ outside archive staging directory')
 exe=HERE/'tools/MPQEditor.exe';require(exe.exists(),'Missing tools/MPQEditor.exe');mpq.parent.mkdir(parents=True,exist_ok=True)
 script=mpq.with_suffix('.build.txt');require(not script.exists(),'Build script already exists')
 for p in [str(stage),str(mpq)]:require(not any(c in p for c in '"\r\n'),'Unsupported filesystem path')
 capacity=max(32,2**math.ceil(math.log2(len(report['files'])*2)))
 script.write_text('\n'.join([f'new "{mpq}" {capacity}']+[f'add "{mpq}" "{stage/f["path"]}" "{f["path"].replace(chr(47),chr(92))}" /c' for f in report['files']]+[f'close "{mpq}"','exit'])+'\n')
 r=subprocess.run([str(exe),'/console',str(script)],capture_output=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),timeout=120)
 require(r.returncode==0,'MPQEditor failed: '+r.stderr.decode(errors='replace'))
 result=verify_mpq(mpq,stage);mpq.with_suffix('.validation.json').write_text(json.dumps(result,indent=2));return result

def transform_model(cfg,root,out):
 src=(root/cfg['obj']).resolve();source_texture=(root/cfg['texture']).resolve();summary=inspect_obj(src);require(summary['face_sizes']==[3],'Triangulate the OBJ before conversion')
 require(summary['triangles']<=21845,'Converter index-count limit exceeded');require(summary['uv_entries']>0 and summary['normal_entries']>0,'Export UVs and normals')
 target=member(cfg['model_path']);texpath=member(cfg['texture_path']);require(target.lower().endswith('.m2') and texpath.lower().endswith('.blp'),'Use .m2 and .blp paths')
 prepared=out/'prepared'/Path(target).stem;prepared.mkdir(parents=True,exist_ok=False);stage=out/'archive';model=stage/target;tex=stage/texpath
 require(not model.exists() and not tex.exists(),'Model or texture path already present; choose unique new paths')
 up=cfg.get('up_axis','Y').upper();require(up in ['Y','Z'],'up_axis must be Y or Z');origin=np.array(cfg.get('origin',[0,0,0]),dtype=float);scale=np.array(cfg.get('scale',[1,1,1]),dtype=float)
 require(origin.shape==(3,) and scale.shape==(3,) and np.isfinite(origin).all() and np.isfinite(scale).all() and (scale>0).all(),'Invalid origin or positive scale')
 def rotate(w):
  x,y,z=map(float,w[1:4]);return np.array([x,-z,y] if up=='Y' else [x,y,z])
 lines=src.read_text(encoding='utf-8-sig').splitlines();v=[];norm=[];f=[]
 for line in lines:
  w=line.split()
  if not w:continue
  if w[0]=='v':v.append(rotate(w))
  elif w[0]=='vn':norm.append(rotate(w))
  elif w[0]=='f':
   ids=[int(x.split('/')[0]) for x in w[1:]];f.append([x-1 if x>0 else len(v)+x for x in ids])
 v=(np.array(v)-origin)*scale;norm=np.array(norm)/scale;length=np.linalg.norm(norm,axis=1);require((length>0).all(),'Zero normals');norm/=length[:,None];f=np.array(f)
 require(f.min()>=0 and f.max()<len(v),'Invalid OBJ face indices');height=float(np.ptp(v[:,2]));require(height>0,'No vertical extent')
 extension=float(cfg.get('extend_foot',0));require(extension>=0,'extend_foot must be nonnegative');v[:,2]-=extension*np.clip(1-v[:,2]/(height*.003),0,1)
 col=cfg['collision'];kind=col['mode']
 if kind=='tree':
  require(v[:,2].max()>0,'Tree must extend above local ground z=0')
  end=float(col['top_fraction']);floor=float(col.get('sample_floor_fraction',.0005));require(0<floor<end<=1,'Invalid collision fractions')
  cv,cf=fitted_collision(v,f,height,end,sample_floor=floor);cv=np.array(cv)
 elif kind=='prepared_obj':
  # Supplied collision is already in output Z-up coordinates BEFORE lower_z.
  cp=(root/col['path']).resolve();cv=[];cf=[]
  for l in cp.read_text().splitlines():
   w=l.split()
   if not w:continue
   if w[0]=='v':cv.append(list(map(float,w[1:4])))
   elif w[0]=='f':require(len(w)==4,'Collision must be triangulated');cf.append([int(t.split('/')[0])-1 for t in w[1:]])
  cv=np.array(cv,dtype=float)
 else:raise ValueError('collision.mode must be tree or prepared_obj')
 lower=float(cfg.get('lower_z',0));require(math.isfinite(lower),'Invalid lower_z');v[:,2]-=lower;cv[:,2]-=lower
 vi=ni=0;new=[]
 for line in lines:
  w=line.split()
  if w and w[0]=='v':new.append('v '+' '.join(f'{x:.9g}' for x in v[vi]));vi+=1
  elif w and w[0]=='vn':new.append('vn '+' '.join(f'{x:.9g}' for x in norm[ni]));ni+=1
  elif not w or w[0]!='mtllib':new.append(line)
 obj=prepared/'visible.obj';obj.write_text('\n'.join(new)+'\n');collision=prepared/'collision.obj';collision.write_text('\n'.join(['v '+' '.join(f'{x:.9g}' for x in p) for p in cv]+['f '+' '.join(str(i+1) for i in face) for face in cf])+'\n')
 treport=texture(source_texture,tex,size=int(cfg.get('texture_size',2048)),dark_guard=bool(cfg.get('ue_dark_pixel_guard',False)))
 exe=HERE/'tools/OBJtoM2.exe';require(exe.exists(),'Missing tools/OBJtoM2.exe')
 r=subprocess.run([str(exe),str(obj),'model','--texture',texpath.replace('/','\\'),'--collision',str(collision)],cwd=prepared,capture_output=True,text=True,timeout=60)
 (prepared/'converter.log').write_text(r.stdout+'\n'+r.stderr);require(r.returncode==0,'Converter failed: '+r.stdout+r.stderr)
 model.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(prepared/'model.m2',model);shutil.copy2(prepared/'model00.skin',model.with_name(model.stem+'00.skin'))
 return dict(model_path=target,source_obj_sha256=digest(src),source_texture_sha256=digest(source_texture),source=summary,texture=treport,transform=dict(up_axis=up,origin=origin.tolist(),scale=scale.tolist(),lower_z=lower,extend_foot=extension))

def main():
 p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
 x=sub.add_parser('inspect');x.add_argument('obj')
 x=sub.add_parser('measure');x.add_argument('m2')
 x=sub.add_parser('validate');x.add_argument('stage')
 x=sub.add_parser('verify');x.add_argument('mpq');x.add_argument('stage')
 x=sub.add_parser('pack');x.add_argument('stage');x.add_argument('mpq')
 x=sub.add_parser('blp');x.add_argument('image');x.add_argument('output');x.add_argument('--size',type=int,default=2048);x.add_argument('--ue-dark-pixel-guard',action='store_true')
 x=sub.add_parser('build');x.add_argument('config');x.add_argument('--out',required=True)
 a=p.parse_args()
 if a.command=='inspect':r=inspect_obj(a.obj)
 elif a.command=='measure':r=measure_m2(a.m2)
 elif a.command=='validate':r=validate_stage(a.stage)
 elif a.command=='verify':r=verify_mpq(a.mpq,a.stage)
 elif a.command=='pack':r=pack(a.stage,a.mpq)
 elif a.command=='blp':require(not Path(a.output).exists(),'Output already exists');r=texture(Path(a.image),Path(a.output),a.size,a.ue_dark_pixel_guard)
 else:
  config=Path(a.config).resolve();cfg=json.loads(config.read_text(encoding='utf-8-sig'));out=Path(a.out).resolve();require(not out.exists(),'Use a fresh output directory');require(cfg.get('models'),'No models configured')
  out.mkdir(parents=True);(out/'archive').mkdir();shutil.copy2(config,out/'build-config.json');results=[]
  for m in cfg['models']:results.append(transform_model(m,config.parent,out))
  (out/'model-report.json').write_text(json.dumps(results,indent=2));name=member(cfg.get('patch_name','patch-enGB-T.MPQ'));require('/' not in name and name.lower().endswith('.mpq'),'Invalid patch filename');r=pack(out/'archive',out/name)
 print(json.dumps(r,indent=2))
if __name__=='__main__':
 try:main()
 except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
