"""End-to-end tests use synthetic geometry; no WoW installation is needed."""
import json, subprocess, sys, tempfile, unittest
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from pipeline import inspect_obj,validate_stage,verify_mpq,pack,member,measure_m2
from textures import texture
class PipelineTests(unittest.TestCase):
 def test_reject_archive_traversal(self):
  for value in ['../x','World/../x','/absolute','C:/x','World//x','bad\nname']:
   with self.assertRaises(ValueError):member(value)
 def test_opaque_only(self):
  with tempfile.TemporaryDirectory(prefix='objm2-test-') as td:
   root=Path(td);p=root/'alpha.png';Image.new('RGBA',(8,8),(100,120,140,0)).save(p)
   with self.assertRaises(ValueError):texture(p,root/'bad.blp',8)
 def test_end_to_end_and_repack(self):
  with tempfile.TemporaryDirectory(prefix='objm2-test-') as td:
   out=Path(td)/'result'
   r=subprocess.run([sys.executable,str(ROOT/'scripts/pipeline.py'),'build',str(ROOT/'examples/demo.json'),'--out',str(out)],capture_output=True,text=True)
   self.assertEqual(r.returncode,0,r.stdout+r.stderr)
   report=verify_mpq(out/'patch-enGB-T.MPQ',out/'archive');self.assertEqual(report['archive_members_verified'],3)
   m=report['models'][0];self.assertEqual(m['triangles'],12);self.assertAlmostEqual(m['bounds_min'][2],-.28,places=5);self.assertAlmostEqual(m['bounds_max'][2],11.75,places=5)
   im=np.array(Image.open(out/'archive/World/CustomTrees/DemoTrunk.blp').convert('RGB'))/255.;linear=np.where(im<=.04045,im/12.92,((im+.055)/1.055)**2.4);self.assertGreaterEqual(linear.sum(2).min(),.065)
   measured=measure_m2(out/'archive/World/CustomTrees/DemoTrunk.m2');self.assertAlmostEqual(measured['dimensions'][2],12.03,places=5)
   second=pack(out/'archive',Path(td)/'repacked.MPQ');self.assertEqual(second['files'],report['files'])
   with self.assertRaises(ValueError):pack(out/'archive',Path(td)/'repacked.MPQ')
   skin=out/'archive/World/CustomTrees/DemoTrunk00.skin';skin.write_bytes(skin.read_bytes()[:30])
   with self.assertRaises(ValueError):validate_stage(out/'archive')
 def test_inspection(self):
  r=inspect_obj(ROOT/'examples/assets/demo-trunk.obj');self.assertEqual(r['triangles'],12);self.assertEqual(r['face_sizes'],[3])
if __name__=='__main__':unittest.main()
