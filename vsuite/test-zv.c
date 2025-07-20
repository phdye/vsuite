#include <stdio.h>
#include <string.h>
#include "zv.h"
#include <ctype.h>

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

static void test_copy_exact(void) {
    DECL_VARCHAR(src,4); DECL_VARCHAR(dst,4);
    strcpy(src.arr, "abc"); src.len = 3;
    int n = zv_copy(dst, src);
    CHECK("zv_copy exact", n == 3 && dst.len == 3 && strcmp(dst.arr, "abc") == 0);
}

static void test_copy_empty(void) {
    DECL_VARCHAR(src,4); DECL_VARCHAR(dst,4);
    src.arr[0] = '\0'; src.len = 0;
    int n = zv_copy(dst, src);
    CHECK("zv_copy empty", n == 0 && dst.len == 0 && dst.arr[0] == '\0');
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

static void test_trim_noop(void) {
    DECL_VARCHAR(v,5);
    strcpy(v.arr, "hi"); v.len = 2; zv_trim(v);
    CHECK("zv_trim no-op", v.len == 2 && strcmp(v.arr, "hi") == 0);
}

static void test_trim_all_spaces(void) {
    DECL_VARCHAR(v,5);
    strcpy(v.arr, "   "); v.len = 3; zv_trim(v);
    CHECK("zv_trim all", v.len == 0 && v.arr[0] == '\0');
}

static void test_zero_term_empty(void) {
    DECL_VARCHAR(v,4); v.len = 0; v.arr[0] = 'x';
    zv_zero_term(v);
    CHECK("zv_zero_term empty", v.len == 0 && v.arr[0] == '\0');
}

static void test_trim_empty(void) {
    DECL_VARCHAR(v,5);
    v.arr[0] = '\0';
    v.len = 0;
    zv_ltrim(v);
    CHECK("zv_ltrim empty", v.len == 0 && v.arr[0] == '\0');
    zv_rtrim(v);
    CHECK("zv_rtrim empty", v.len == 0 && v.arr[0] == '\0');
    zv_trim(v);
    CHECK("zv_trim empty", v.len == 0 && v.arr[0] == '\0');
}

static void test_case_empty(void) {
    DECL_VARCHAR(v,1);
    v.arr[0] = '\0';
    v.len = 0;
    zv_upper(v);
    CHECK("zv_upper empty", v.len == 0 && v.arr[0] == '\0');
    zv_lower(v);
    CHECK("zv_lower empty", v.len == 0 && v.arr[0] == '\0');
}

static void test_copy_self(void) {
    DECL_VARCHAR(v,5);
    strcpy(v.arr, "abc");
    v.len = 3;
    int n = zv_copy(v, v);
    CHECK("zv_copy self", n == 3 && v.len == 3 && strcmp(v.arr, "abc") == 0);
}

static void test_zero_term_idempotent(void) {
    DECL_VARCHAR(v,4);
    strcpy(v.arr, "abc");
    v.len = 3;
    zv_zero_term(v);
    CHECK("zv_zero_term idempotent", v.len == 3 && strcmp(v.arr, "abc") == 0);
}

static void test_large_copy(void) {
    enum { N = 4096 };
    DECL_VARCHAR(src, N); DECL_VARCHAR(dst, N);
    memset(src.arr, 'a', N-1); src.arr[N-1] = '\0'; src.len = N-1;
    int n = zv_copy(dst, src);
    CHECK("zv_copy large", n == N-1 && dst.len == N-1 && memcmp(dst.arr, src.arr, N-1) == 0 && dst.arr[N-1] == '\0');
}

static void test_valid_zero_len_bad_term(void) {
    DECL_VARCHAR(v, 2);
    v.arr[0] = 'x';
    v.len = 0;
    CHECK("zv_valid zero bad", !zv_valid(v));
}

static void test_trim_tabs_newlines(void) {
    DECL_VARCHAR(v1, 10); DECL_VARCHAR(v2, 10);
    strcpy(v1.arr, "\tfoo\n"); v1.len = 5; zv_ltrim(v1);
    CHECK("zv_ltrim misc", v1.len == 4 && strcmp(v1.arr, "foo\n") == 0);
    strcpy(v2.arr, "foo\t\n"); v2.len = 5; zv_rtrim(v2);
    CHECK("zv_rtrim misc", v2.len == 3 && strcmp(v2.arr, "foo") == 0);
}

static void test_copy_dest_size_one(void) {
    DECL_VARCHAR(src,2); DECL_VARCHAR(dst,1);
    strcpy(src.arr,"a"); src.len=1;
    int n = zv_copy(dst, src);
    CHECK("zv_copy tiny", n == 0 && dst.len == 0);
}

static void test_upper_lower_nonalpha(void) {
    DECL_VARCHAR(v,5);
    strcpy(v.arr, "a1!B"); v.len=4;
    zv_upper(v);
    CHECK("zv_upper nonalpha", strcmp(v.arr,"A1!B") == 0);
    zv_lower(v);
    CHECK("zv_lower nonalpha", strcmp(v.arr,"a1!b") == 0);
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
    test_copy_exact();
    test_copy_empty();
    test_trim();
    test_trim_noop();
    test_trim_all_spaces();
    test_trim_empty();
    test_valid_zero_len_bad_term();
    test_zero_term_empty();
    test_zero_term_idempotent();
    test_case_empty();
    test_copy_self();
    test_large_copy();
    test_trim_tabs_newlines();
    test_copy_dest_size_one();
    test_upper_lower_nonalpha();
    test_case();
    if (failures == 0) {
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
