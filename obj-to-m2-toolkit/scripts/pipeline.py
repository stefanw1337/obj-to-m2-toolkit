"""General static OBJ to Wrath M2/SKIN/BLP2 and additive MPQ toolkit."""
import argparse
import json
import math
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import assets
from assets import require,member,digest
from m2_writer import write
from validate import validate,read

ROOT=Path(__file__).resolve().parents[1]


def load_obj(path):
    positions=[];uvs=[];normals=[];faces=[]
    def index(text,items):
        value=int(text);require(value!=0,'OBJ indices cannot be zero')
        result=value-1 if value>0 else len(items)+value
        require(0<=result<len(items),'OBJ index out of range');return result
    for line in Path(path).read_text(encoding='utf-8-sig').splitlines():
        parts=line.partition('#')[0].split()
        if not parts:continue
        if parts[0] in ('v','vt','vn'):
            count=2 if parts[0]=='vt' else 3
            value=list(map(float,parts[1:count+1]));require(len(value)==count and np.isfinite(value).all(),'Invalid OBJ coordinate')
            {'v':positions,'vt':uvs,'vn':normals}[parts[0]].append(value)
        elif parts[0]=='f':
            require(len(parts)==4,'Triangulate the OBJ before conversion')
            face=[]
            for corner in parts[1:]:
                fields=corner.split('/');require(len(fields)<=3,'Invalid OBJ face')
                face.append((index(fields[0],positions),index(fields[1],uvs) if len(fields)>1 and fields[1] else None,index(fields[2],normals) if len(fields)>2 and fields[2] else None))
            faces.append(face)
    require(positions and faces,'OBJ needs vertices and triangles')
    return positions,uvs,normals,faces


def inspect_obj(path):
    positions,uvs,normals,faces=load_obj(path);v=np.array(positions)
    return dict(vertices=len(v),triangles=len(faces),uv_entries=len(uvs),normal_entries=len(normals),minimum=v.min(0).tolist(),maximum=v.max(0).tolist())


def transform(cfg):
    up=cfg.get('up_axis','Y').upper();require(up in ('Y','Z'),'up_axis must be Y or Z')
    axes=np.array([[1,0,0],[0,0,-1],[0,1,0]]) if up=='Y' else np.eye(3)
    scale=np.array(cfg.get('scale',[1,1,1]),dtype=float)
    if scale.ndim==0:scale=np.repeat(scale,3)
    origin=np.array(cfg.get('origin',[0,0,0]),dtype=float);offset=np.array(cfg.get('offset',[0,0,0]),dtype=float)
    require(scale.shape==origin.shape==offset.shape==(3,) and np.isfinite([scale,origin,offset]).all() and (scale>0).all(),'Invalid transform')
    angle=math.radians(cfg.get('rotation_z_degrees',0));c,s=math.cos(angle),math.sin(angle);rotation=np.array([[c,-s,0],[s,c,0],[0,0,1]])
    matrix=rotation@np.diag(scale)@axes
    shift=offset-rotation@np.diag(scale)@origin
    return matrix,shift


def scene_from_obj(cfg,root):
    require(not any(k in cfg for k in ('extend_foot','lower_z')),'Use an explicit offset; geometry-specific legacy options are unsupported')
    require(not any(k in cfg for k in ('texture_size','ue_dark_pixel_guard')),'Prepare the source image at the desired size and colors; legacy texture overrides are unsupported')
    positions,uvs,normals,faces=load_obj(root/cfg['obj']);matrix,shift=transform(cfg);normal_matrix=np.linalg.inv(matrix).T
    points=np.array(positions)@matrix.T+shift
    vertices=[];triangles=[];cache={}
    require(len(faces)<=21845,'Maximum 21,845 triangles per model')
    for face in faces:
        a,b,c=[points[v[0]] for v in face];normal=np.cross(b-a,c-a);length=np.linalg.norm(normal)
        require(length>1e-12,'Degenerate triangle');normal/=length
        ids=[]
        for vi,ui,ni in face:
            require(ui is not None,'Every visible face corner needs UV coordinates')
            n=normal if ni is None else normal_matrix@normals[ni];length=np.linalg.norm(n);require(length>0,'Zero normal');n=n/length
            key=(vi,ui,tuple(np.round(n,8)))
            if key not in cache:
                cache[key]=len(vertices)
                vertices.append(dict(position=points[vi].tolist(),normal=n.tolist(),uv=uvs[ui],weights=[255,0,0,0],bones=[0,0,0,0]))
            ids.append(cache[key])
        triangles.append(ids)
    # Convert OBJ's bottom-left UV origin to M2's texture convention by default.
    if cfg.get('flip_v',True):
        for v in vertices:v['uv']=[v['uv'][0],1-v['uv'][1]]
    used=np.array([v['position'] for v in vertices]);minimum=used.min(0);maximum=used.max(0);radius=float(np.linalg.norm(used,axis=1).max())
    identity=dict(translation=[0,0,0],rotation=[0,0,0,1],scale=[1,1,1])
    scene=dict(vertices=vertices,triangles=triangles,bones=[dict(name='Root',parent=-1,pivot=[0,0,0])],sequences=[dict(name='Stand',id=0,duration=1000,movespeed=0,loop=True,timestamps=[0],tracks=[[identity]],minimum=minimum.tolist(),maximum=maximum.tolist(),radius=radius)],collision=None)
    collision=cfg.get('collision',{'mode':'none'});mode=collision['mode']
    if mode=='box':
        require(np.all(maximum>minimum),'A box collision needs nonzero dimensions on every axis')
        cv=[[x,y,z] for z in (minimum[2],maximum[2]) for y in (minimum[1],maximum[1]) for x in (minimum[0],maximum[0])]
        cf=[[0,2,3],[0,3,1],[4,5,7],[4,7,6],[0,1,5],[0,5,4],[2,6,7],[2,7,3],[0,4,6],[0,6,2],[1,3,7],[1,7,5]]
        scene['collision']=dict(vertices=cv,triangles=cf)
    elif mode in ('mesh','prepared_obj'):
        cv,_,_,cf=load_obj(root/collision['path']);cv=np.array(cv)
        if mode=='mesh':cv=cv@matrix.T+shift
        scene['collision']=dict(vertices=cv.tolist(),triangles=[[v[0] for v in f] for f in cf])
    else:require(mode=='none','collision.mode must be none, box, mesh, or prepared_obj')
    return scene


def validate_stage(stage):
    stage=Path(stage);models=[];files=[];keys=set()
    for p in sorted(stage.rglob('*')):
        if not p.is_file():continue
        name=member(p.relative_to(stage));require(name.lower() not in keys,'Case-insensitive path conflict');keys.add(name.lower())
        require(p.suffix.lower() in ('.m2','.skin','.blp'),'Unexpected staged file: '+name)
        files.append(dict(path=name,bytes=p.stat().st_size,sha256=digest(p)))
        if p.suffix.lower()=='.m2':
            try:report=validate(p)
            except ValueError:
                from legacy_validation import validate_stage as validate_legacy
                return validate_legacy(stage)
            report['path']=name;models.append(report)
        elif p.suffix.lower()=='.blp':
            with Image.open(p) as im:im.load()
    require(models,'No staged models')
    for m in models:require(m['texture'].lower() in keys,'Missing texture '+m['texture'])
    for name in keys:
        if name.endswith('.skin'):require(name.endswith('00.skin') and name[:-7]+'.m2' in keys,'Orphan skin')
    return dict(models=models,files=files)


def verify_mpq(mpq,stage):
    import mpyq
    report=validate_stage(stage)
    with Path(mpq).open('rb') as f:
        a=mpyq.MPQArchive(f);expected={x['path'].lower() for x in report['files']}
        require({x.decode().replace('\\','/').lower() for x in a.files if not x.startswith(b'(')}==expected,'MPQ member mismatch')
        for x in report['files']:require(a.read_file(x['path'].replace('/','\\'))==(Path(stage)/x['path']).read_bytes(),'MPQ bytes mismatch')
    return dict(**report,archive_members_verified=len(report['files']),mpq_sha256=digest(mpq))


def build(config,out):
    config=Path(config).resolve();root=config.parent;cfg=json.loads(config.read_text(encoding='utf-8-sig'));out=Path(out).resolve()
    require(not out.exists(),'Use a fresh output directory');require(cfg.get('models'),'No models configured')
    out.mkdir(parents=True);stage=out/'archive';stage.mkdir();reports=[]
    (out/'build-config.json').write_text(json.dumps(cfg,indent=2))
    for model in cfg['models']:
        scene=scene_from_obj(model,root);path=member(model['model_path']);texpath=member(model['texture_path'])
        require(path.lower().endswith('.m2') and texpath.lower().endswith('.blp'),'Expected M2 and BLP paths')
        blend=model.get('material',{}).get('blend','opaque')
        texture=assets.texture(root/model['texture'],stage/texpath,blend)
        result=write(scene,stage/path,texpath,model.get('material'));result['validation']=validate(stage/path,scene);result['texture']=texture;result['path']=path;reports.append(result)
    report=validate_stage(stage);name=member(cfg.get('patch_name','patch-S.MPQ'));require('/' not in name and name.lower().endswith('.mpq'),'Invalid patch filename')
    report.update(models=reports,package=assets.pack(stage,out/name,ROOT/'tools/MPQEditor.exe'),in_game_verified=False)
    (out/'validation.json').write_text(json.dumps(report,indent=2));return report


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    a=sub.add_parser('build');a.add_argument('config');a.add_argument('--out',required=True)
    a=sub.add_parser('inspect');a.add_argument('obj')
    a=sub.add_parser('measure');a.add_argument('m2')
    a=sub.add_parser('validate');a.add_argument('stage')
    a=sub.add_parser('verify');a.add_argument('mpq');a.add_argument('stage')
    a=sub.add_parser('pack');a.add_argument('stage');a.add_argument('mpq')
    a=sub.add_parser('blp');a.add_argument('image');a.add_argument('output');a.add_argument('--blend',choices=['opaque','cutout','alpha','additive'],default='opaque')
    a=p.parse_args()
    if a.command=='build':r=build(a.config,a.out)
    elif a.command=='inspect':r=inspect_obj(a.obj)
    elif a.command=='measure':
        v=read(a.m2)['vertices'][:,:3];r=dict(vertices=len(v),minimum=v.min(0).tolist(),maximum=v.max(0).tolist(),dimensions=np.ptp(v,axis=0).tolist())
    elif a.command=='validate':r=validate_stage(a.stage)
    elif a.command=='verify':r=verify_mpq(a.mpq,a.stage)
    elif a.command=='pack':validate_stage(a.stage);r=assets.pack(a.stage,a.mpq,ROOT/'tools/MPQEditor.exe')
    else:r=assets.texture(a.image,a.output,a.blend)
    print(json.dumps(r,indent=2))


if __name__=='__main__':
    try:main()
    except Exception as e:print('ERROR:',e,file=sys.stderr);sys.exit(1)
