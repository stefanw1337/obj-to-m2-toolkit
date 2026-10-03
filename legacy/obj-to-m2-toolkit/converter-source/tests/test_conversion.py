"""Read the produced binary files independently; no Blender or game required."""
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = str(Path(sys.argv.pop(1)).resolve())
BASE = """v 0 0 0
v 1 0 0
v 0 1 0
v 1 1 0
vt 0 0
vt 1 0
vt 0 1
vt 1 1
vt 0.25 0.75
vn 0 0 1
vn 0 1 0
"""


def read_output(stem):
    data = Path(str(stem) + '.m2').read_bytes()
    assert data[:4] == b'MD20'
    assert struct.unpack_from('<I', data, 4)[0] == 264
    count, offset = struct.unpack_from('<II', data, 60)
    assert offset + count * 48 <= len(data)
    vertices = []
    textures, texture_offset = struct.unpack_from('<II', data, 80)
    assert textures == 1
    _, _, name_length, name_offset = struct.unpack_from('<4I', data, texture_offset)
    assert name_offset + name_length <= len(data)
    assert data[name_offset:name_offset+name_length].endswith(b'.blp')
    for i in range(count):
        start = offset + i * 48
        position = struct.unpack_from('<3f', data, start)
        normal = struct.unpack_from('<3f', data, start + 20)
        uv = struct.unpack_from('<2f', data, start + 32)
        assert all(math.isfinite(v) for v in position + normal + uv)
        vertices.append((position, normal, uv))
    skin = Path(str(stem) + '00.skin').read_bytes()
    assert skin[:4] == b'SKIN'
    fields = struct.unpack_from('<11I', skin, 4)
    ni, oi, nt, ot, np, op, ns, os, nb, ob, _ = fields
    assert ni == np == count
    assert nb == ns
    for size, n, o in [(2, ni, oi), (2, nt, ot), (4, np, op), (48, ns, os), (24, nb, ob)]:
        assert o + n * size <= len(skin)
    lookup = struct.unpack_from(f'<{ni}H', skin, oi)
    indices = struct.unpack_from(f'<{nt}H', skin, ot)
    assert all(v < count for v in lookup)
    assert all(i < ni for i in indices)
    sections = []
    for n in range(ns):
        _, sv, nv, st, nc = struct.unpack_from('<I4H', skin, os + n * 48)
        assert sv + nv <= ni and st + nc <= nt
        assert all(sv <= i < sv + nv for i in indices[st:st+nc])
        sections.append((sv, nv, st, nc))
    # Resolve each rendered corner through SKIN -> M2 rather than assuming identity.
    corners = [vertices[lookup[i]] for i in indices]
    return vertices, corners, sections


class ConversionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)

    def convert(self, content, success=True, interactive=False):
        source = self.folder / 'input.obj'
        source.write_text(content)
        stem = self.folder / 'model'
        args = [EXE, str(source), str(stem)]
        if not interactive:
            args += ['--texture', r'World\Custom\oak.blp']
        result = subprocess.run(args, input='', text=True, capture_output=True, timeout=15)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            return read_output(stem)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(stem.with_suffix('.m2').exists())
        return result.stderr

    def test_static_sequence_bounds_and_selection(self):
        self.convert(BASE + 'f 1/1/1 2/2/1 3/3/1\n')
        data = (self.folder / 'model.m2').read_bytes()
        count, offset = struct.unpack_from('<II', data, 28)
        self.assertEqual(count, 1)
        self.assertEqual(struct.unpack_from('<h', data, offset + 16)[0], 32767)
        self.assertEqual(struct.unpack_from('<7f', data, offset + 32),
                         struct.unpack_from('<7f', data, 160))
        self.assertGreater(struct.unpack_from('<f', data, offset + 56)[0], 0)

    def test_uv_seam(self):
        vertices, corners, _ = self.convert(BASE + 'f 1/1/1 2/2/1 3/3/1\nf 1/5/1 3/3/1 4/4/1\n')
        self.assertEqual(len(vertices), 5)
        self.assertEqual(corners[0][0], corners[3][0])
        self.assertEqual(corners[0][2], (0, 1))
        self.assertEqual(corners[3][2], (0.25, 0.25))

    def test_hard_normal(self):
        vertices, corners, _ = self.convert(BASE + 'f 1/1/1 2/2/1 3/3/1\nf 1/1/2 3/3/1 4/4/1\n')
        self.assertEqual(len(vertices), 5)
        self.assertEqual(corners[0][1], (0, 0, 1))
        self.assertEqual(corners[3][1], (0, 1, 0))

    def test_groups_shared_vertices_duplicate_faces_and_unused_positions(self):
        face = 'f 1/1/1 2/2/1 3/3/1\n'
        vertices, corners, sections = self.convert(BASE + face + 'g second\n' + face * 2)
        self.assertEqual(len(vertices), 6)
        self.assertEqual(len(corners), 9)
        self.assertEqual(sections, [(0, 3, 0, 3), (3, 3, 3, 6)])

    def test_whitespace_negative_indices(self):
        _, corners, _ = self.convert(BASE + '  f\t-4/-5/-2  -3/-4/-2 -2/-3/-2 # triangle\n')
        self.assertEqual([v[0] for v in corners], [(0, 0, 0), (1, 0, 0), (0, 1, 0)])

    def test_material_change_creates_contiguous_section(self):
        _, _, sections = self.convert(BASE + 'usemtl a\nf 1/1/1 2/2/1 3/3/1\nusemtl b\nf 1/1/1 3/3/1 4/4/1\n')
        self.assertEqual(sections, [(0, 3, 0, 3), (3, 3, 3, 3)])

    def test_invalid_input_rejected(self):
        for face in ['f 0/1/1 2/2/1 3/3/1', 'f 99/1/1 2/2/1 3/3/1',
                     'f 1//1 2/2/1 3/3/1', 'f 1/1/1 2/2/1',
                     'f 1/1/1 2/2/1 3/3/1 4/4/1']:
            with self.subTest(face=face):
                self.assertIn('OBJ line', self.convert(BASE + face, success=False))

    def test_empty_input_rejected(self):
        self.assertIn('no faces', self.convert(BASE, success=False))

    def test_index_limit_rejected(self):
        self.assertIn('Too many triangle indices', self.convert(
            BASE + 'f 1/1/1 2/2/1 3/3/1\n' * 21846, success=False))

    def test_interactive_eof_does_not_hang(self):
        self.assertIn('Input ended', self.convert(
            BASE + 'f 1/1/1 2/2/1 3/3/1\n', success=False, interactive=True))

    def test_missing_output_directory_reports_failure(self):
        source = self.folder / 'input.obj'
        source.write_text(BASE + 'f 1/1/1 2/2/1 3/3/1\n')
        result = subprocess.run([EXE, str(source), str(self.folder / 'missing' / 'oak'),
                                 '--texture', r'World\Custom\oak.blp'],
                                capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)

    def test_collision_is_separate_from_visible_geometry(self):
        source = self.folder / 'input.obj'
        source.write_text(BASE + 'f 1/1/1 2/2/1 3/3/1\n')
        collision = self.folder / 'trunk.obj'
        collision.write_text('v 0 0 0\nv 0.1 0 0\nv 0 0.1 0\nv 0 0 0.5\n'
                             'f 1 3 2\nf 1 2 4\nf 2 3 4\nf 3 1 4\n')
        stem = self.folder / 'tree'
        result = subprocess.run([EXE, str(source), str(stem), '--texture', 'tree.blp',
                                 '--collision', str(collision)], capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        visible, _, _ = read_output(stem)
        self.assertEqual(len(visible), 3)
        data = stem.with_suffix('.m2').read_bytes()
        nt, ot, nv, ov, nn, on = struct.unpack_from('<6I', data, 216)
        self.assertEqual((nt, nv, nn), (12, 4, 4))
        vertices = [struct.unpack_from('<3f', data, ov + i*12) for i in range(nv)]
        self.assertAlmostEqual(max(v[0] for v in vertices), 0.1)
        self.assertAlmostEqual(max(v[2] for v in vertices), 0.5)
        for i in range(nn):
            n = struct.unpack_from('<3f', data, on + i*12)
            self.assertAlmostEqual(sum(x*x for x in n), 1.0, places=5)
        collision_box = struct.unpack_from('<6f', data, 188)
        self.assertEqual(collision_box[:3], (0, 0, 0))
        self.assertAlmostEqual(collision_box[3], 0.1)
        self.assertAlmostEqual(collision_box[5], 0.5)
        skin = Path(str(stem) + '00.skin').read_bytes()
        section_offset = struct.unpack_from('<I', skin, 32)[0]
        center = struct.unpack_from('<3f', skin, section_offset + 32)
        radius = struct.unpack_from('<f', skin, section_offset + 44)[0]
        self.assertEqual(center, (0.5, 0.5, 0))
        self.assertAlmostEqual(radius, math.sqrt(0.5), places=5)

    def test_degenerate_collision_rejected_before_writing(self):
        source = self.folder / 'input.obj'
        source.write_text(BASE + 'f 1/1/1 2/2/1 3/3/1\n')
        collision = self.folder / 'bad.obj'
        collision.write_text('v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n')
        stem = self.folder / 'bad'
        result = subprocess.run([EXE, str(source), str(stem), '--texture', 'tree.blp',
                                 '--collision', str(collision)], capture_output=True, text=True, timeout=15)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Degenerate collision', result.stderr)
        self.assertFalse(stem.with_suffix('.m2').exists())


if __name__ == '__main__':
    unittest.main()
