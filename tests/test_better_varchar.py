import unittest
import os
import types

module_path = os.path.join(os.path.dirname(__file__), "..", "better-varchar.py")
better_varchar = types.ModuleType("better_varchar")
better_varchar.__file__ = module_path
with open(module_path, "r") as fh:
    code = compile(fh.read(), module_path, "exec")
exec(code, better_varchar.__dict__)

class TestBetterVarchar(unittest.TestCase):
    """Tests for the better-varchar source transformation helpers."""

    def test_setlenz(self):
        # Simple termination assignment should convert to macro
        src = "FOO.arr[FOO.len] = '\0';"
        self.assertIn('VARCHAR_SETLENZ(FOO);', better_varchar.transform(src))

    def test_v_copy(self):
        # strcpy + terminator should collapse to v_copy
        src = "strcpy(foo.arr, bar.arr);\nfoo.arr[bar.len] = '\0';"
        out = better_varchar.transform(src)
        self.assertIn('v_copy(foo, bar);', out)

    def test_vp_copy_literal(self):
        # copying a literal string should yield vp_copy
        src = 'strcpy(foo.arr, "hi");'
        out = better_varchar.transform(src)
        self.assertIn('vp_copy(foo, "hi");', out)

    def test_only_filter(self):
        src = (
            "strcpy(foo.arr, bar.arr);\nfoo.arr[bar.len] = '\0';\n"
            "foo.arr[foo.len] = '\0';"
        )
        out = better_varchar.transform(src, only=['setlenz'])
        self.assertIn('VARCHAR_SETLENZ(foo);', out)
        self.assertNotIn('v_copy(foo, bar);', out)

    def test_parse_only(self):
        args = better_varchar.parse_args(['--only:vp_copy', 'in.pc', 'out.pc'])
        self.assertEqual(args.only, ['vp_copy'])

    def test_parse_show(self):
        args = better_varchar.parse_args(['--show', '--only:setlenz', 'in.pc', 'out.pc'])
        self.assertTrue(args.show)
        self.assertEqual(args.only, ['setlenz'])

    def test_show_no_write(self):
        import tempfile
        tmpdir = tempfile.mkdtemp()
        try:
            in_path = os.path.join(tmpdir, 'in.pc')
            out_path = os.path.join(tmpdir, 'out.pc')
            with open(in_path, 'w') as fh:
                fh.write("FOO.arr[FOO.len] = '\0';\n")
            better_varchar.main(['--show', in_path, out_path])
            self.assertFalse(os.path.exists(out_path))
        finally:
            if os.path.exists(in_path):
                os.remove(in_path)
            if os.path.exists(out_path):
                os.remove(out_path)
            os.rmdir(tmpdir)

if __name__ == '__main__':
    unittest.main()
