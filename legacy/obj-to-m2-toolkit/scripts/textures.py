from pathlib import Path
import io, struct
import numpy as np
from PIL import Image
lut=np.arange(256)/255.;lut=np.where(lut<=.04045,lut/12.92,((lut+.055)/1.055)**2.4)

def blp2(image):
    image=image.convert('RGB')
    w,h=image.size
    if any(n<=0 or n & (n-1) for n in (w,h)):
        raise ValueError('Power-of-two dimensions required')
    payloads=[]; dims=[]
    while True:
        dims.append(image.size)
        # BC1 always stores complete 4x4 blocks, including the final tiny mips.
        padded=image
        if min(image.size)<4:
            padded=Image.new('RGB',(max(4,image.width),max(4,image.height)))
            for y in range(padded.height):
                for x in range(padded.width):
                    padded.putpixel((x,y),image.getpixel((min(x,image.width-1),min(y,image.height-1))))
        stream=io.BytesIO();padded.save(stream,format='DDS',pixel_format='DXT1')
        dds=stream.getvalue();assert dds[:4]==b'DDS ' and dds[84:88]==b'DXT1'
        payload=dds[128:]
        expected=((image.width+3)//4)*((image.height+3)//4)*8
        assert len(payload)==expected
        payloads.append(payload)
        if image.size==(1,1):break
        image=image.resize((max(1,image.width//2),max(1,image.height//2)),Image.Resampling.LANCZOS)
    assert len(payloads)<=16
    offsets=[0]*16;sizes=[0]*16;offset=148+1024
    for i,payload in enumerate(payloads):
        offsets[i]=offset;sizes[i]=len(payload);offset+=len(payload)
    header=struct.pack('<4sI4BII16I16I',b'BLP2',1,2,0,0,1,w,h,*offsets,*sizes)
    return header+bytes(1024)+b''.join(payloads),dims

def rgb(c):
 r=(c>>11)&31;g=(c>>5)&63;b=c&31
 return np.array([(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)],dtype=np.int32)

def palette(a,b):
 x,y=rgb(a),rgb(b);return np.array([x,y,(2*x+y)//3,(x+2*y)//3])

def lift(c):return (min(31,((c>>11)&31)+1)<<11)|(min(63,((c>>5)&63)+2)<<5)|min(31,(c&31)+1)

def texture(src,dst,size=2048,dark_guard=False):
 original=Image.open(src).convert('RGBA')
 if original.getchannel('A').getextrema() != (255,255):raise ValueError('This pipeline supports opaque textures only; do not discard source alpha')
 if size<=0 or size>4096 or size & (size-1):raise ValueError('Texture size must be a power of two up to 4096')
 im=original.convert('RGB').resize((size,size),Image.Resampling.LANCZOS)
 data,dims=blp2(im);b=bytearray(data);changed=0;minsum=3.
 for off,n in zip(struct.unpack_from('<16I',b,20),struct.unpack_from('<16I',b,84)):
  for j in range(off,off+n,8):
   a,c,bits=struct.unpack_from('<HHI',b,j);old=(a,c,bits)
   if a<=c:
    x,y=rgb(a),rgb(c);palold=np.array([x,y,(x+y)//2,[0,0,0]])
    na,nc=(c,a) if a<c else ((a+1,a) if a<65535 else (a,a-1))
    mapping=((palold[:,None,:]-palette(na,nc)[None,:,:])**2).sum(2).argmin(1)
    nb=0
    for k in range(16):
     idx=(bits>>(2*k))&3;assert idx!=3
     nb|=int(mapping[idx])<<(2*k)
    a,c,bits=na,nc,nb
   used=set((bits>>(2*k))&3 for k in range(16))
   for attempt in range(64):
    sums=lut[palette(a,c)].sum(1)
    if not dark_guard or min(sums[k] for k in used)>=.065:break
    if sums[0]<.08:a=lift(a)
    if sums[1]<.08:c=lift(c)
    if a==c:
     if a<65535:a+=1
     else:c-=1
    if a<c:a,c=c,a;bits^=0x55555555;used=set((bits>>(2*k))&3 for k in range(16))
   else:raise ValueError('Black mask guard failed')
   assert a>c
   minsum=min(minsum,float(min(sums[k] for k in used)));changed+=old!=(a,c,bits)
   struct.pack_into('<HHI',b,j,a,c,bits)
 dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(b)
 decoded=Image.open(io.BytesIO(b)).convert('RGBA');assert decoded.getchannel('A').getextrema()==(255,255)
 # Keep the source image untouched.
 return dict(dimensions=[size,size],mips=len(dims),opaque=True,minimum_linear_rgb_sum=minsum,adjusted_blocks=changed)