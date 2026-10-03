"""Strict validation of the toolkit's Wrath subset and sampled skinning round-trip."""
import struct as S
from pathlib import Path
import numpy as np
from m2_writer import decode_quaternion


def require(ok,message):
    if not ok:raise ValueError(message)


def array(data,header,stride):
    require(header+8<=len(data),'Truncated array header')
    n,o=S.unpack_from('<II',data,header)
    require(not n or o>=48 and o+n*stride<=len(data),'Array extends outside file')
    return n,o


def rotation(q):
    x,y,z,w=q/np.linalg.norm(q)
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)], [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)], [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])


def read(path):
    path=Path(path);b=path.read_bytes()
    require(len(b)>=304 and b[:4]==b'MD20' and S.unpack_from('<I',b,4)[0]==264,'Expected MD20 version 264')
    nv,ov=array(b,60,48);nb,ob=array(b,44,88);ns,os=array(b,28,64)
    require(nv>0 and 0<nb<=64 and ns>0,'Invalid geometry, bone or sequence count')
    vertices=[S.unpack_from('<3f4B4B3f4f',b,ov+48*i) for i in range(nv)]
    for v in vertices:
        require(sum(v[3:7])==255,'Bone weights must sum to 255')
        require(all(0<=i<nb for i,w in zip(v[7:11],v[3:7]) if w),'Invalid bone reference')
        require(np.isfinite(v).all() and abs(np.linalg.norm(v[11:14])-1)<0.0001,'Invalid vertex normal/position')
    sequences=[]
    for i in range(ns):
        row=S.unpack_from('<HHIfIhHIII7fhH',b,os+64*i)
        require(row[2]>0 and row[4]&32 and not row[4]&64,'Expected embedded non-alias sequences')
        sequences.append(dict(id=row[0],duration=row[2],minimum=row[10:13],maximum=row[13:16],radius=row[16]))
    lookup_n,lookup_o=array(b,36,2)
    for i,s in enumerate(sequences):
        require(s['id']<lookup_n and S.unpack_from('<h',b,lookup_o+2*s['id'])[0]==i,'Invalid animation lookup')
    bones=[]
    for i in range(nb):
        o=ob+88*i;parent=S.unpack_from('<h',b,o+8)[0]
        require(-1<=parent<i,'Invalid bone hierarchy')
        bone=dict(parent=parent,pivot=np.array(S.unpack_from('<3f',b,o+76)),tracks=[])
        for offset,fmt in [(16,'3f'),(36,'4h'),(56,'3f')]:
            interp,glob=S.unpack_from('<Hh',b,o+offset)
            require(interp==1 and glob==-1,'Unexpected track interpolation')
            nt,ot=array(b,o+offset+4,8);nk,ok=array(b,o+offset+12,8)
            require(nt==nk==ns,'Track sequence arrays mismatch')
            tracks=[]
            for j in range(ns):
                count,times=array(b,ot+j*8,4);count2,keys=array(b,ok+j*8,S.calcsize('<'+fmt))
                require(count==count2 and count>0,'Track key counts mismatch')
                ts=np.array(S.unpack_from('<'+'I'*count,b,times))
                values=np.array([S.unpack_from('<'+fmt,b,keys+k*S.calcsize('<'+fmt)) for k in range(count)],dtype=float)
                require(ts[0]==0 and np.all(np.diff(ts.astype(float))>0) and ts[-1]<=sequences[j]['duration'],'Invalid timestamps')
                if fmt=='4h':
                    values=np.array([decode_quaternion(q) for q in values])
                    require(np.max(np.abs(np.linalg.norm(values,axis=1)-1))<0.0001,'Invalid quaternion')
                require(np.isfinite(values).all(),'Nonfinite track values')
                tracks.append((ts,values))
            bone['tracks'].append(tracks)
        bones.append(bone)
    sk=path.with_name(path.stem+'00.skin').read_bytes();require(sk[:4]==b'SKIN','Missing SKIN signature')
    ni,oi=array(sk,4,2);nf,of=array(sk,12,2);np_,op=array(sk,20,4);nsec,osec=array(sk,28,48);nmat,omat=array(sk,36,24)
    indices=np.array(S.unpack_from('<'+'H'*ni,sk,oi));faces=np.array(S.unpack_from('<'+'H'*nf,sk,of))
    require(ni==nv and np_==ni and nf%3==0 and indices.max()<nv and faces.max()<ni,'Invalid SKIN indices')
    require(nsec==nmat==1,'Expected one SKIN section and batch')
    section=S.unpack_from('<I8H7f',sk,osec)
    require(section[1]==0 and section[2]==nv and section[3]==0 and section[4]==nf and section[5]==nb,'Invalid SKIN section ranges')
    require(section[7]<=4,'Too many bone influences')
    for i,v in enumerate(vertices):require(tuple(sk[op+i*4:op+i*4+4])==v[7:11],'SKIN palette mismatch')
    for head,stride in [(80,16),(88,20),(112,4),(120,2),(128,2),(136,2),(144,2),(152,2),(216,2),(224,12),(232,12)]:array(b,head,stride)
    ci,co=array(b,216,2);cv,cvo=array(b,224,12);cn,cno=array(b,232,12)
    if ci or cv or cn:
        require(ci>0 and ci%3==0 and cv>0 and cn==ci//3,'Invalid collision array counts')
        collision_indices=S.unpack_from('<'+'H'*ci,b,co);require(max(collision_indices)<cv,'Invalid collision index')
        points=np.array([S.unpack_from('<3f',b,cvo+12*i) for i in range(cv)])
        normals=np.array([S.unpack_from('<3f',b,cno+12*i) for i in range(cn)])
        require(np.isfinite(points).all() and np.isfinite(normals).all(),'Nonfinite collision geometry')
        require(np.allclose(np.linalg.norm(normals,axis=1),1,atol=1e-4),'Invalid collision normals')
        require(np.allclose(S.unpack_from('<3f',b,188),points.min(0),atol=1e-5) and np.allclose(S.unpack_from('<3f',b,200),points.max(0),atol=1e-5),'Invalid collision bounds')
    nt,ot=array(b,80,16);require(nt==1,'Expected one texture')
    typ,flags,n,o=S.unpack_from('<4I',b,ot);require(typ==0 and o+n<=len(b),'Invalid texture entry')
    texture=b[o:o+n].rstrip(b'\0').decode().replace('\\','/')
    return dict(vertices=np.array(vertices),bones=bones,sequences=sequences,triangles=indices[faces].reshape(-1,3),texture=texture,collision_triangles=S.unpack_from('<I',b,216)[0]//3)


def pose(model,sequence,time):
    matrices=[]
    for bone in model['bones']:
        values=[]
        for ti,tracks in enumerate(bone['tracks']):
            times,keys=tracks[sequence]
            index=max(0,min(len(times)-1,int(np.searchsorted(times,time,side='right'))-1))
            value=keys[index]
            if index+1<len(times):
                t=(time-times[index])/(times[index+1]-times[index]);other=keys[index+1]
                if ti==1 and np.dot(value,other)<0:other=-other
                value=value*(1-t)+other*t
            values.append(value)
        translation,q,scale=values
        matrix=np.eye(4);matrix[:3,:3]=rotation(q) @ np.diag(scale)
        matrix[:3,3]=translation+bone['pivot']-matrix[:3,:3] @ bone['pivot']
        if bone['parent']>=0:matrix=matrices[bone['parent']] @ matrix
        matrices.append(matrix)
    vertices=model['vertices'];points=np.c_[vertices[:,:3],np.ones(len(vertices))];result=np.zeros((len(vertices),3))
    for i in range(4):
        ids=vertices[:,7+i].astype(int);weights=vertices[:,3+i]/255
        selected=np.array(matrices)[ids]
        result+=np.einsum('nij,nj->ni',selected,points)[:,:3]*weights[:,None]
    return result


def validate(path,scene=None):
    model=read(path);errors=[]
    if scene:
        v=model['vertices'];points=np.c_[v[:,:3],np.ones(len(v))]
        for si,sequence in enumerate(model['sequences']):
            original=next(s for s in scene['sequences'] if s['id']==sequence['id'])
            for ki,time in enumerate(original['timestamps']):
                matrices=[]
                for bi,bone in enumerate(scene['bones']):
                    t=original['tracks'][bi][ki];p=np.array(bone['pivot']);m=np.eye(4)
                    m[:3,:3]=rotation(np.array(t['rotation'])) @ np.diag(t['scale'])
                    m[:3,3]=t['translation']+p-m[:3,:3] @ p
                    if bone['parent']>=0:m=matrices[bone['parent']] @ m
                    matrices.append(m)
                expected=np.zeros((len(v),3))
                for i in range(4):expected+=np.einsum('nij,nj->ni',np.array(matrices)[v[:,7+i].astype(int)],points)[:,:3]*(v[:,3+i]/255)[:,None]
                actual=pose(model,si,time)
                errors.append(float(np.linalg.norm(actual-expected,axis=1).max()))
                require(np.all(actual.min(0)>=np.array(sequence['minimum'])-0.01) and np.all(actual.max(0)<=np.array(sequence['maximum'])+0.01),'Animated vertex outside sequence bounds')
        require(max(errors)<0.005,'Serialized skinning error exceeds tolerance')
    return dict(vertices=len(model['vertices']),triangles=len(model['triangles']),bones=len(model['bones']),sequences=len(model['sequences']),texture=model['texture'],collision_triangles=model['collision_triangles'],sampled_poses=len(errors),max_serialization_error=max(errors,default=0),in_game_verified=False)
