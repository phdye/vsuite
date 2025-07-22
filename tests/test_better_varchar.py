import unittest
import importlib.util
import os

SPEC = importlib.util.spec_from_file_location(
    "better_varchar", os.path.join(os.path.dirname(__file__), "..", "better-varchar.py")
)
better_varchar = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(better_varchar)

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

if __name__ == '__main__':
    unittest.main()
