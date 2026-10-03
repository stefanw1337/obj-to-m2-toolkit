from pathlib import Path
import numpy as np,json

def section(v, f, z):
    segments=[]
    for face in f:
        p=v[face];hits=[]
        for i,j in [(0,1),(1,2),(2,0)]:
            if (p[i,2]<=z<p[j,2]) or (p[j,2]<=z<p[i,2]):
                t=(z-p[i,2])/(p[j,2]-p[i,2]);hits.append(p[i,:2]+t*(p[j,:2]-p[i,:2]))
        if len(hits)==2:segments.append(hits)
    points={};adj={}
    for a,b in segments:
        ka=tuple(np.round(a,6));kb=tuple(np.round(b,6));points[ka]=a;points[kb]=b
        adj.setdefault(ka,set()).add(kb);adj.setdefault(kb,set()).add(ka)
    groups=[];seen=set()
    for k in adj:
        if k in seen:continue
        stack=[k];seen.add(k);keys=[]
        while stack:
            q=stack.pop();keys.append(q)
            for n in adj[q]:
                if n not in seen:seen.add(n);stack.append(n)
        groups.append(np.array([points[q] for q in keys]))
    return groups

