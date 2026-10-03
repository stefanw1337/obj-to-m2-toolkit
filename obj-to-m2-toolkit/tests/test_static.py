"""Static conversion integration tests, including planar geometry and alpha."""
import json
import sys
import tempfile
import unittest
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from pipeline import build,verify_mpq,scene_from_obj,load_obj,member
from validate import read


class StaticTests(unittest.TestCase):
    def test_static_collision_modes(self):
        for mode in ('none','box','mesh','prepared_obj'):
            with self.subTest(mode=mode),tempfile.TemporaryDirectory() as d:
                root=Path(d);config=json.loads((ROOT/'examples/demo.json').read_text());m=config['models'][0]
                m.update(obj=str(ROOT/'examples/demo.obj'),texture=str(ROOT/'examples/demo.ppm'),scale=[2,3,4],offset=[1,2,3],collision={'mode':mode,'path':str(ROOT/'examples/demo.obj')})
                path=root/'config.json';path.write_text(json.dumps(config));report=build(path,root/'output')
                result=report['models'][0];self.assertEqual(result['validation']['collision_triangles'],0 if mode=='none' else 12)
                self.assertTrue(np.allclose(result['minimum'],[0,.5,3]));self.assertTrue(np.allclose(result['maximum'],[2,3.5,7]))
                verified=verify_mpq(root/'output/patch-S.MPQ',root/'output/archive');self.assertEqual(verified['archive_members_verified'],3)
                with self.assertRaises(ValueError):build(path,root/'output')
    def test_flat_model_with_alpha(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);(p/'plane.obj').write_text('v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf -3/-3 -2/-2 -1/-1\n')
            im=Image.new('RGBA',(8,8),(150,200,100,0));im.putpixel((0,0),(255,255,255,255));im.save(p/'alpha.png')
            model=dict(obj='plane.obj',texture='alpha.png',up_axis='Z',model_path='World/Custom/Plane.m2',texture_path='World/Custom/Plane.blp',material={'blend':'cutout'})
            config=p/'config.json';config.write_text(json.dumps({'models':[model]}));build(config,p/'build')
            m=read(p/'build/archive/World/Custom/Plane.m2');self.assertEqual(m['collision_triangles'],0)
            self.assertTrue(np.allclose(m['vertices'][0,14:16],[0,1]))
            with Image.open(p/'build/archive/World/Custom/Plane.blp') as image:self.assertEqual(image.convert('RGBA').getchannel('A').getextrema(),(0,255))
            model['collision']={'mode':'box'}
            with self.assertRaises(ValueError):scene_from_obj(model,p)
    def test_bad_input(self):
        for path in ('../x','a/../x','C:/x','a//x'):
            with self.assertRaises(ValueError):member(path)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'bad.obj';p.write_text('v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 99\n')
            with self.assertRaises(ValueError):load_obj(p)


if __name__=='__main__':unittest.main()
