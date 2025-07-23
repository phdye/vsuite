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
        args = better_varchar.parse_args(['--show', '--only:setlenz', 'in.pc'])
        self.assertTrue(args.show)
        self.assertEqual(args.only, ['setlenz'])

    def test_parse_show_with_output_error(self):
        with self.assertRaises(SystemExit):
            better_varchar.parse_args(['--show', 'in.pc', 'out.pc'])

    def test_parse_show_with_lines(self):
        args = better_varchar.parse_args([
            '--show', '--lines', '1:5', 'in.pc'
        ])
        self.assertTrue(args.show)
        self.assertEqual(args.lines, '1:5')

    def test_parse_show_with_fraction(self):
        args = better_varchar.parse_args([
            '--show', '--fraction', '0.1:0.2', 'in.pc'
        ])
        self.assertTrue(args.show)
        self.assertEqual(args.fraction, '0.1:0.2')

    def test_parse_show_with_function(self):
        args = better_varchar.parse_args([
            '--show', '--function', 'foo', 'in.pc'
        ])
        self.assertTrue(args.show)
        self.assertEqual(args.function, ['foo'])

    def test_show_no_write(self):
        import tempfile
        tmpdir = tempfile.mkdtemp()
        try:
            in_path = os.path.join(tmpdir, 'in.pc')
            out_path = os.path.join(tmpdir, 'out.pc')
            with open(in_path, 'w') as fh:
                fh.write("FOO.arr[FOO.len] = '\0';\n")
            better_varchar.main(['--show', in_path])
            self.assertFalse(os.path.exists(out_path))
        finally:
            if os.path.exists(in_path):
                os.remove(in_path)
            if os.path.exists(out_path):
                os.remove(out_path)
            os.rmdir(tmpdir)

    def test_show_with_scopes(self):
        import tempfile
        import io
        import contextlib

        tmpdir = tempfile.mkdtemp()
        try:
            in_path = os.path.join(tmpdir, 'in.pc')
            with open(in_path, 'w') as fh:
                fh.write(
                    "void foo() \n"
                    "{\n"
                    "    FOO.arr[FOO.len] = '\0';\n"
                    "}\n"
                    "void bar() \n"
                    "{\n"
                    "    BAR.arr[BAR.len] = '\0';\n"
                    "}\n"
                    "void fizz() \n"
                    "{\n"
                    "    FOO.arr[FOO.len] = '\0';\n"
                    "}\n"
                    "void buzz() \n"
                    "{\n"
                    "    BAR.arr[BAR.len] = '\0';\n"
                    "}\n"
                )

            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', in_path])
            all_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(all_output), 4)

            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--lines', '1:2', in_path])
            line_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(line_output), 0)

            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--lines', '1:4', in_path])
            line_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(line_output), 1)

            # Outout is matched later with fraction 50% / 0.50 and function foo+bar output
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--lines', '1:8', in_path])
            line_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(line_output), 2)

            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--fraction', '0:0.25', in_path])
            frac_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(frac_output), 1)

            # Outout is matched later with final lines:... output
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--fraction', '0:0.5', in_path])
            frac_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(frac_output), 2)

            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--function', 'foo', in_path])
            func_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(func_output), 1)

            # Outout is matched later with final lines:... output
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                better_varchar.main(['--show', '--function', 'foo', '--function', 'bar', in_path])
            func_output = buf.getvalue().strip().splitlines()
            self.assertEqual(len(func_output), 2)

            self.assertGreater(len(all_output), len(line_output))
            self.assertEqual(line_output, frac_output)
            self.assertEqual(line_output, func_output)
        finally:
            if os.path.exists(in_path):
                os.remove(in_path)
            os.rmdir(tmpdir)

if __name__ == '__main__':
    unittest.main()
