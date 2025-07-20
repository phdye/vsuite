#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "vsuite/v.h"

static int failures = 0;
static int verbose = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { \
        printf("\nFAIL: %s\n", name); \
        failures++; \
    } else if (verbose) { \
        printf("PASS: %s\n", name); \
    } else { \
        fputc('.', stdout); fflush(stdout); \
    } \
} while (0)

static void test_init_clear(void) {
    VARCHAR(v, 5);
    memset(v.arr, 'x', sizeof(v.arr));
    v.len = 3;
    v_init(v);
    CHECK("v_init", v.len == 0);

    v.len = 4;
    v_clear(v);
    CHECK("v_clear", v.len == 0);
}

static void test_valid(void) {
    VARCHAR(v, 5);
    v.len = 5;
    CHECK("v_valid ok", v_valid(v));
    v.len = 6;
    CHECK("v_valid overflow", !v_valid(v));
}

static void test_copy(void) {
    VARCHAR(src, 6); VARCHAR(dst, 6); VARCHAR(small, 2);
    strcpy(src.arr, "abc"); src.len = 3;
    int n = v_copy(dst, src);
    CHECK("v_copy len", n == 3 && dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);

    n = v_copy(small, src);
    CHECK("v_copy fail", n == 0 && small.len == 0);
}

static void test_copy_exact(void) {
    VARCHAR(src, 3); VARCHAR(dst, 3);
    strcpy(src.arr, "abc"); src.len = 3;
    int n = v_copy(dst, src);
    CHECK("v_copy exact", n == 3 && dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

static void test_copy_empty(void) {
    VARCHAR(src, 4); VARCHAR(dst, 4);
    src.len = 0;
    int n = v_copy(dst, src);
    CHECK("v_copy empty", n == 0 && dst.len == 0);
}

static void test_trim(void) {
    VARCHAR(v1, 10); VARCHAR(v2, 10); VARCHAR(v3, 10);
    strcpy(v1.arr, "  hi"); v1.len = 4; v_ltrim(v1);
    CHECK("v_ltrim", v1.len == 2 && memcmp(v1.arr, "hi", 2) == 0);

    strcpy(v2.arr, "hi  "); v2.len = 4; v_rtrim(v2);
    CHECK("v_rtrim", v2.len == 2 && memcmp(v2.arr, "hi", 2) == 0);

    strcpy(v3.arr, "  hi  "); v3.len = 6; v_trim(v3);
    CHECK("v_trim", v3.len == 2 && memcmp(v3.arr, "hi", 2) == 0);
}

static void test_trim_noop(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "hi"); v.len = 2;
    v_ltrim(v);
    CHECK("v_ltrim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
    v_rtrim(v);
    CHECK("v_rtrim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
    v_trim(v);
    CHECK("v_trim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
}

static void test_trim_all_spaces(void) {
    VARCHAR(v, 6);
    strcpy(v.arr, "   "); v.len = 3;
    v_trim(v);
    CHECK("v_trim all", v.len == 0);
}

static void test_trim_empty(void) {
    VARCHAR(v, 5);
    v.len = 0;
    v_ltrim(v);
    CHECK("v_ltrim empty", v.len == 0);
    v_rtrim(v);
    CHECK("v_rtrim empty", v.len == 0);
    v_trim(v);
    CHECK("v_trim empty", v.len == 0);
}

static void test_case_empty(void) {
    VARCHAR(v, 3);
    v.len = 0;
    v_upper(v);
    CHECK("v_upper empty", v.len == 0);
    v_lower(v);
    CHECK("v_lower empty", v.len == 0);
}

static void test_copy_self(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "abc");
    v.len = 3;
    int n = v_copy(v, v);
    CHECK("v_copy self", n == 3 && v.len == 3 && memcmp(v.arr, "abc", 3) == 0);
}

static void test_large_copy(void) {
    enum { N = 4096 };
    VARCHAR(src, N); VARCHAR(dst, N);
    memset(src.arr, 'a', N); src.len = N;
    int n = v_copy(dst, src);
    CHECK("v_copy large", n == N && dst.len == N && memcmp(dst.arr, src.arr, N) == 0);
}

static void test_case(void) {
    VARCHAR(v, 4);
    strcpy(v.arr, "aB3"); v.len = 3;
    v_upper(v);
    CHECK("v_upper", strcmp(v.arr, "AB3") == 0 && v.len == 3);
    v_lower(v);
    CHECK("v_lower", strcmp(v.arr, "ab3") == 0 && v.len == 3);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose = 1;
    test_init_clear();
    test_valid();
    test_copy();
    test_copy_exact();
    test_copy_empty();
    test_trim();
    test_trim_noop();
    test_trim_all_spaces();
    test_trim_empty();
    test_large_copy();
    test_case_empty();
    test_copy_self();
    test_case();
    if (failures == 0) {
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
