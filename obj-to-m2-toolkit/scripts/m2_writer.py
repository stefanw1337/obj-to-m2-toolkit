"""Write self-contained Wrath MD20 v264 models and a single SKIN profile."""
import math
import struct as S
from pathlib import Path


def require(ok, message):
    if not ok: raise ValueError(message)


class Buffer:
    def __init__(self,size): self.data=bytearray(size)
    def add(self,payload):
        self.data.extend(b'\0'*((-len(self.data))%16))
        offset=len(self.data);self.data.extend(payload);return offset
    def put(self,offset,fmt,*args):S.pack_into('<'+fmt,self.data,offset,*args)
    def array(self,header,fmt,rows):
        rows=list(rows)
        payload=b''.join(S.pack('<'+fmt,*r) for r in rows)
        self.put(header,'II',len(rows),self.add(payload) if rows else 0)


def encode_quaternion(values):
    # On-disk signed M2CompQuat encoding, not a conventional signed-normalized int16.
    result=[]
    for value in values:
        value=max(-1.,min(1.,value))
        result.append(int(round(value*32767 + (32767 if value<0 else -32768))))
    return result


def decode_quaternion(values):
    return [(v+32768)/32767 if v<0 else (v-32767)/32767 for v in values]


def track(buffer,offset,timelines,values,fmt):
    require(len(timelines)==len(values),'Track sequence count mismatch')
    time_pairs=[];value_pairs=[]
    for times,keys in zip(timelines,values):
        require(len(times)==len(keys),'Track key count mismatch')
        if keys and all(k==keys[0] for k in keys):times=[0];keys=keys[:1]
        time_pairs.append((len(times),buffer.add(S.pack('<'+'I'*len(times),*times))))
        value_pairs.append((len(keys),buffer.add(b''.join(S.pack('<'+fmt,*k) for k in keys))))
    t=buffer.add(b''.join(S.pack('<II',*p) for p in time_pairs))
    v=buffer.add(b''.join(S.pack('<II',*p) for p in value_pairs))
    buffer.put(offset,'HhIIII',1,-1,len(time_pairs),t,len(value_pairs),v)


def write(scene,path,texture_path,material=None):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    require(not path.exists(),'Refusing to overwrite model')
    material=material or {}
    vertices=scene['vertices'];triangles=scene['triangles'];bones=scene['bones'];seqs=sorted(scene['sequences'],key=lambda x:x['id'])
    require(vertices and triangles and bones and seqs,'Incomplete scene')
    # A single hardware bone palette is deliberately bounded for the first release.
    require(len(bones)<=64,'This exporter supports at most 64 bones per section')
    require(len(vertices)<=65535 and len(triangles)*3<=65535,'SKIN section limits exceeded')
    b=Buffer(304);b.put(0,'4sI',b'MD20',264)
    name=path.stem.encode()+b'\0';b.put(8,'II',len(name),b.add(name))
    minimum=[min(s['minimum'][i] for s in seqs) for i in range(3)]
    maximum=[max(s['maximum'][i] for s in seqs) for i in range(3)]
    radius=max(s['radius'] for s in seqs)
    b.put(160,'7f',*minimum,*maximum,radius)
    # Nonzero selection bounds are retained even when collision triangle arrays are empty.
    b.put(188,'7f',*minimum,*maximum,radius)
    seqrows=[]
    for s in seqs:
        seqrows.append((s['id'],0,s['duration'],s['movespeed'],32 if s['loop'] else 33,32767,0,0,0,150,*s['minimum'],*s['maximum'],s['radius'],-1,0))
    b.array(28,'HHIfIhHIII7fhH',seqrows)
    lookup=[-1]*max(506,max(s['id'] for s in seqs)+1)
    for i,s in enumerate(seqs):lookup[s['id']]=i
    b.array(36,'h',[(x,) for x in lookup])
    bone_offset=b.add(bytes(88*len(bones)));b.put(44,'II',len(bones),bone_offset)
    times=[s['timestamps'] for s in seqs]
    for i,bone in enumerate(bones):
        o=bone_offset+i*88
        b.put(o,'iIhHI',-1,512,bone['parent'],0,0)
        b.put(o+76,'3f',*bone['pivot'])
        for field,delta,fmt in [('translation',16,'3f'),('rotation',36,'4h'),('scale',56,'3f')]:
            tracks=[]
            for s in seqs:
                keys=[t[field] for t in s['tracks'][i]]
                if field=='rotation':
                    continuous=[]
                    for q in keys:
                        if continuous and sum(a*c for a,c in zip(q,continuous[-1]))<0:q=[-v for v in q]
                        continuous.append(q)
                    keys=[encode_quaternion(q) for q in continuous]
                tracks.append(keys)
            track(b,o+delta,times,tracks,fmt)
    b.array(52,'h',[(-1,)]*27)
    b.array(60,'3f4B4B3f4f',[( *v['position'],*v['weights'],*v['bones'],*v['normal'],*v['uv'],0.,0.) for v in vertices])
    b.put(68,'I',1)
    texname=texture_path.replace('/','\\').encode()+b'\0'
    to=b.add(texname);b.array(80,'4I',[(0,0,len(texname),to)])
    weight=b.add(bytes(20));b.put(88,'II',1,weight)
    track(b,weight,[[0] for _ in seqs],[[(32767,)] for _ in seqs],'h')
    blend={'opaque':0,'cutout':1,'alpha':2,'additive':3}.get(material.get('blend','opaque'))
    require(blend is not None,'Unknown material blend mode')
    flags=(4 if material.get('two_sided',False) else 0)|(1 if material.get('unlit',False) else 0)
    b.array(112,'HH',[(flags,blend)])
    b.array(120,'H',[(i,) for i in range(len(bones))])
    b.array(128,'H',[(0,)])
    b.array(136,'H',[(0,)])
    b.array(144,'H',[(0,)])
    b.array(152,'h',[(-1,)])
    collision=scene.get('collision')
    if collision:
        cv=collision['vertices'];cf=collision['triangles'];normals=[]
        for face in cf:
            a,c,d=[cv[i] for i in face];u=[c[i]-a[i] for i in range(3)];v=[d[i]-a[i] for i in range(3)]
            n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]];length=math.sqrt(sum(x*x for x in n))
            require(length>0,'Degenerate collision triangle');normals.append(tuple(x/length for x in n))
        b.array(216,'H',[(x,) for f in cf for x in f]);b.array(224,'3f',cv);b.array(232,'3f',normals)
        b.put(188,'7f',*[min(v[i] for v in cv) for i in range(3)],*[max(v[i] for v in cv) for i in range(3)],max(math.sqrt(sum(x*x for x in v)) for v in cv))
    skin=Buffer(48);skin.put(0,'4s',b'SKIN');skin.put(44,'I',64)
    skin.array(4,'H',[(i,) for i in range(len(vertices))])
    skin.array(12,'H',[(v,) for tri in triangles for v in tri])
    skin.array(20,'4B',[tuple(v['bones']) for v in vertices])
    center=[(a+c)/2 for a,c in zip(minimum,maximum)]
    skin.array(28,'I8H7f',[(0,0,len(vertices),0,len(triangles)*3,len(bones),0,max(sum(w>0 for w in v['weights']) for v in vertices),0,*center,*center,radius)])
    skin.array(36,'HHHHh7H',[(16,0,0,0,-1,0,0,1,0,0,0,0)])
    path.write_bytes(b.data)
    path.with_name(path.stem+'00.skin').write_bytes(skin.data)
    return dict(vertices=len(vertices),triangles=len(triangles),bones=len(bones),animations=[dict(id=s['id'],name=s['name'],duration_ms=s['duration']) for s in seqs],minimum=minimum,maximum=maximum,radius=radius)
