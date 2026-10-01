import numpy as np
from trunk_sections import section

def fitted_collision(v, f, height, end, sample_floor=.0005):
    n = 64
    levels = sorted(set([float(v[:, 2].min()), 0.0005*height] + [t*height for t in [.003,.01,.02,.03,.045,.06,.08,.10,.12,.15,.18,.20,.22,.25,.28] if t < end] + [end*height]))
    rings = []
    for z in levels:
        sample_z = max(z, height*sample_floor)
        groups = section(v, f, sample_z)
        selected = min(groups, key=lambda g: np.linalg.norm((g.min(0)+g.max(0))/2))
        allowed = {tuple(p.round(6)) for p in selected}
        segments = []
        for face in f:
            p = v[face]; hits = []
            for i,j in [(0,1),(1,2),(2,0)]:
                if (p[i,2]<=sample_z<p[j,2]) or (p[j,2]<=sample_z<p[i,2]):
                    t=(sample_z-p[i,2])/(p[j,2]-p[i,2])
                    hits.append(p[i,:2]+t*(p[j,:2]-p[i,:2]))
            if len(hits)==2 and (z < height*.07 or tuple(hits[0].round(6)) in allowed):
                segments.append(hits)
        radii = []
        for k in range(n):
            a=2*np.pi*k/n; d=np.array([np.cos(a),np.sin(a)]); distances=[]
            for p,q in segments:
                e=q-p; det=d[0]*e[1]-d[1]*e[0]
                if abs(det)<1e-12: continue
                r=(p[0]*e[1]-p[1]*e[0])/det
                t=(p[0]*d[1]-p[1]*d[0])/det
                if r>0 and -.000001<=t<=1.000001: distances.append(r)
            radii.append(max(distances) if distances else np.nan)
        good=np.flatnonzero(np.isfinite(radii)); assert len(good)>n//4
        radii=np.array(radii); missing=np.flatnonzero(~np.isfinite(radii))
        radii[missing]=np.interp(missing,np.r_[good-n,good,good+n],np.tile(radii[good],3))
        rings.append([(float(r*np.cos(2*np.pi*k/n)),float(r*np.sin(2*np.pi*k/n)),z) for k,r in enumerate(radii)])
    cv=[p for ring in rings for p in ring]; cf=[]
    for ring in range(len(rings)-1):
        for k in range(n):
            a=ring*n+k;b=ring*n+(k+1)%n;c=(ring+1)*n+k;d=(ring+1)*n+(k+1)%n
            cf.extend([(a,b,d),(a,d,c)])
    bottom=len(cv);cv.append((0,0,levels[0]));top=len(cv);cv.append((0,0,levels[-1]))
    for k in range(n):
        cf.extend([(bottom,(k+1)%n,k),(top,(len(rings)-1)*n+k,(len(rings)-1)*n+(k+1)%n)])
    return cv,cf

