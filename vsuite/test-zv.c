#include <stdio.h>
#include <string.h>
#include "zv.h"

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
    DECL_VARCHAR(v, 5);
    strcpy(v.arr, "abc"); v.len = 3;
    zv_init(v);
    CHECK("zv_init", v.len == 0 && v.arr[0] == '\0');
    strcpy(v.arr, "abc"); v.len = 3;
    zv_clear(v);
    CHECK("zv_clear", v.len == 0 && v.arr[0] == '\0');
}

static void test_valid(void) {
    DECL_VARCHAR(v, 4);
    strcpy(v.arr, "abc"); v.len = 3; v.arr[3] = '\0';
    CHECK("zv_valid ok", zv_valid(v));
    v.len = 4;
    CHECK("zv_valid len", !zv_valid(v));
    v.len = 3; v.arr[3] = 'x';
    CHECK("zv_valid term", !zv_valid(v));
}

static void test_zero_term(void) {
    DECL_VARCHAR(v1, 4); memcpy(v1.arr, "abcx", 4); v1.len = 3; zv_zero_term(v1);
    CHECK("zv_zero_term keep", v1.len == 3 && v1.arr[3] == '\0');

    DECL_VARCHAR(v2, 4); memcpy(v2.arr, "abcd", 4); v2.len = 4; zv_zero_term(v2);
    CHECK("zv_zero_term cut", v2.len == 3 && v2.arr[3] == '\0');
}

static void test_copy(void) {
    DECL_VARCHAR(src,6); DECL_VARCHAR(dst,6); DECL_VARCHAR(small,3);
    strcpy(src.arr, "abc"); src.len = 3;
    int n = zv_copy(dst, src);
    CHECK("zv_copy len", n == 3 && dst.len == 3 && strcmp(dst.arr, "abc") == 0);
    n = zv_copy(small, src);
    CHECK("zv_copy fail", n == 0 && small.len == 2 && small.arr[2] == '\0');
}

static void test_trim(void) {
    DECL_VARCHAR(v1,10); DECL_VARCHAR(v2,10); DECL_VARCHAR(v3,10);
    strcpy(v1.arr, "  hi"); v1.len = 4; zv_ltrim(v1);
    CHECK("zv_ltrim", v1.len == 2 && strcmp(v1.arr, "hi") == 0);

    strcpy(v2.arr, "hi  "); v2.len = 4; zv_rtrim(v2);
    CHECK("zv_rtrim", v2.len == 2 && strcmp(v2.arr, "hi") == 0);

    strcpy(v3.arr, "  hi  "); v3.len = 6; zv_trim(v3);
    CHECK("zv_trim", v3.len == 2 && strcmp(v3.arr, "hi") == 0);
}

static void test_case(void) {
    DECL_VARCHAR(v,4);
    strcpy(v.arr, "aB3"); v.len = 3; zv_upper(v);
    CHECK("zv_upper", strcmp(v.arr, "AB3") == 0 && v.arr[3] == '\0');
    zv_lower(v);
    CHECK("zv_lower", strcmp(v.arr, "ab3") == 0 && v.arr[3] == '\0');
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose = 1;
    test_init_clear();
    test_valid();
    test_zero_term();
    test_copy();
    test_trim();
    test_case();
    if (failures == 0) {
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
