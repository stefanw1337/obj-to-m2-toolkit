"""Shared texture conversion and additive MPQ packaging."""
import hashlib

import io

import json

import math

import re

import struct as S

import subprocess

from pathlib import Path, PurePosixPath

import mpyq

from PIL import Image

def require(ok,message):
    if not ok: raise ValueError(message)

def member(path):
    path=str(path).replace('\\','/')
    require(path and not PurePosixPath(path).is_absolute() and all(x not in ('','.','..') for x in path.split('/')) and not any(c in path for c in ':"\r\n'),'Unsafe archive path')
    return path

def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def texture(source,output,blend):
    with Image.open(source) as original:image=original.convert('RGBA')
    w,h=image.size
    require(w>0 and h>0 and not w&(w-1) and not h&(h-1),'Texture dimensions must be powers of two')
    require(max(w,h)<=4096,'Texture exceeds 4096 pixels')
    if blend=='opaque':image.putalpha(255)
    # Encode BC1 for opaque surfaces and BC3 for alpha surfaces, including small mips.
    offsets=[0]*16;sizes=[0]*16;payload=[];offset=1172;i=0
    while True:
        padded=image
        if min(image.size)<4:
            padded=Image.new('RGBA',(max(4,image.width),max(4,image.height)))
            for y in range(padded.height):
                for x in range(padded.width):padded.putpixel((x,y),image.getpixel((min(x,image.width-1),min(y,image.height-1))))
        stream=io.BytesIO();padded.save(stream,format='DDS',pixel_format='DXT1' if blend=='opaque' else 'DXT5')
        raw=stream.getvalue()[128:];offsets[i]=offset;sizes[i]=len(raw);payload.append(raw);offset+=len(raw);i+=1
        if image.size==(1,1):break
        image=image.resize((max(1,image.width//2),max(1,image.height//2)),Image.Resampling.LANCZOS)
    header=S.pack('<4sI4BII16I16I',b'BLP2',1,2,0 if blend=='opaque' else 8,0 if blend=='opaque' else 7,1,w,h,*offsets,*sizes)
    output=Path(output);output.parent.mkdir(parents=True,exist_ok=True);require(not output.exists(),'Texture output already exists')
    output.write_bytes(header+bytes(1024)+b''.join(payload))
    with Image.open(output) as decoded:
        decoded.load();require(decoded.size==(w,h),'BLP decode failed')
    return dict(width=w,height=h,mips=i,encoding='BC1' if blend=='opaque' else 'BC3',alpha_depth=0 if blend=='opaque' else 8)

def pack(stage,output,exe):
    stage=Path(stage).resolve();output=Path(output).resolve();exe=Path(exe).resolve()
    require(not output.exists(),'MPQ output already exists')
    require(output.parent!=stage and stage not in output.parents,'MPQ must be outside staging')
    files=sorted(p for p in stage.rglob('*') if p.is_file())
    require(files,'No staged files')
    output.parent.mkdir(parents=True,exist_ok=True)
    entries=[member(p.relative_to(stage)) for p in files]
    require(len(set(x.lower() for x in entries))==len(entries),'Case-insensitive archive path conflict')
    for p in [str(stage),str(output),str(exe)]:require(not any(c in p for c in '"\r\n'),'Unsupported filesystem path')
    script=output.with_suffix('.build.txt')
    require(not script.exists(),'MPQ build script already exists')
    capacity=max(32,2**math.ceil(math.log2(len(files)*2)))
    lines=[f'new "{output}" {capacity}']+[f'add "{output}" "{p}" "{entry.replace(chr(47),chr(92))}" /c' for p,entry in zip(files,entries)]+[f'close "{output}"','exit']
    script.write_text('\n'.join(lines)+'\n')
    result=subprocess.run([str(exe),'/console',str(script)],capture_output=True,timeout=120,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    require(result.returncode==0,'MPQEditor failed: '+result.stderr.decode(errors='replace'))
    with output.open('rb') as f:
        archive=mpyq.MPQArchive(f)
        listed={n.decode().replace('\\','/').lower() for n in archive.files if not n.startswith(b'(')}
        require(listed=={e.lower() for e in entries},'MPQ member set mismatch')
        for p,e in zip(files,entries):require(archive.read_file(e.replace('/','\\'))==p.read_bytes(),'MPQ round-trip failed: '+e)
    return dict(mpq=str(output),sha256=digest(output),files=[dict(path=e,bytes=p.stat().st_size,sha256=digest(p)) for p,e in zip(files,entries)],archive_members_verified=len(entries))
