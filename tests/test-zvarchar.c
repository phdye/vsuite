#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "vsuite/zvarchar.h"

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

/*
 * Initialization helpers for zero-terminated VARCHARS should both clear the
 * length and write a terminator byte at position zero.
 */
static void test_init_clear(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "abc");
    v.len = 3;
    zv_init(v);
    CHECK("zv_init", v.len == 0 && v.arr[0] == '\0');
    strcpy(v.arr, "abc");
    v.len = 3;
    zv_clear(v);
    CHECK("zv_clear", v.len == 0 && v.arr[0] == '\0');
}

/*
 * zv_valid() additionally checks that the byte after the last character is a
 * NUL terminator.  This test covers the success case, an overflow case and a
 * missing terminator case.
 */
static void test_valid(void) {
    VARCHAR(v, 4);
    strcpy(v.arr, "abc");
    v.len = 3;
    v.arr[3] = '\0';
    CHECK("zv_valid ok", zv_valid(v));
    v.len = 4;
    CHECK("zv_valid len", !zv_valid(v));      /* length beyond buffer */
    v.len = 3;
    v.arr[3] = 'x';
    CHECK("zv_valid term", !zv_valid(v));     /* no terminator */
}

/* Validate that zv_valid succeeds when len is zero but the array is
 * NUL-terminated as expected.
 */
static void test_valid_zero_len_good_term(void) {
    VARCHAR(v, 2);
    v.len = 0;
    v.arr[0] = '\0';
    CHECK("zv_valid zero good", zv_valid(v));
}

/* Even length zero strings must have a terminator byte. */
static void test_valid_zero_len_bad_term(void) {
    VARCHAR(v, 2);
    v.arr[0] = 'x';
    v.len = 0;
    CHECK("zv_valid zero bad", !zv_valid(v));
}

/*
 * zv_has_capacity() reports whether a buffer can hold N characters in
 * addition to the terminating NUL byte. This verifies normal and edge cases
 * including size-one buffers that offer zero capacity.
 */
static void test_has_capacity(void) {
    VARCHAR(v, 5);                 /* 4 bytes usable */
    CHECK("zv_has_capacity ok", zv_has_capacity(v, 4));
    CHECK("zv_has_capacity max", !zv_has_capacity(v, 5));
    CHECK("zv_has_capacity zero", zv_has_capacity(v, 0));

    VARCHAR(tiny, 1);              /* no space for data */
    CHECK("zv_has_capacity none", !zv_has_capacity(tiny, 1) &&
                                  zv_has_capacity(tiny, 0));
}

/* ZV_CAPACITY reports the usable size excluding the terminator. */
static void test_zv_capacity(void) {
    VARCHAR(v, 5);
    CHECK("ZV_CAPACITY", ZV_CAPACITY(v) == 4);
    VARCHAR(tiny, 1);
    CHECK("ZV_CAPACITY zero", ZV_CAPACITY(tiny) == 0);
}

/*
 * zv_zero_term() should ensure the string is terminated.  If the length is
 * within bounds it simply writes a terminator, otherwise it truncates the
 * string.
 */
static void test_zero_term(void) {
    VARCHAR(v1, 4); memcpy(v1.arr, "abcx", 4); v1.len = 3; zv_zero_terminate(v1);
    CHECK("zv_zero_term keep", v1.len == 3 && v1.arr[3] == '\0');

    VARCHAR(v2, 4); memcpy(v2.arr, "abcd", 4); v2.len = 4; zv_zero_terminate(v2);
    CHECK("zv_zero_term cut", v2.len == 3 && v2.arr[3] == '\0');
}

/* Calling zv_zero_term() multiple times should not change data. */
static void test_zero_term_idempotent(void) {
    VARCHAR(v,4);
    strcpy(v.arr, "abc");
    v.len = 3;
    zv_zero_terminate(v);
    CHECK("zv_zero_term idempotent", v.len == 3 && strcmp(v.arr, "abc") == 0);
}

/* zv_zero_term() should handle empty strings and write the terminator. */
static void test_zero_term_empty(void) {
    VARCHAR(v,4);
    v.len = 0;
    v.arr[0] = 'x';
    zv_zero_terminate(v);
    CHECK("zv_zero_term empty", v.len == 0 && v.arr[0] == '\0');
}

/* Ensure zv_zero_term works on a size-one buffer by resetting the single
 * character to the terminator and clearing the length.
 */
static void test_zero_term_size_one(void) {
    VARCHAR(v,1);
    v.arr[0] = 'a';
    v.len = 1;
    zv_zero_terminate(v);
    CHECK("zv_zero_term size1", v.len == 0 && v.arr[0] == '\0');
}

/*
 * Copying into a sufficiently large destination should succeed and preserve
 * termination.  Copying to a smaller buffer should truncate and report zero
 * bytes copied.
 */
static void test_copy(void) {
    VARCHAR(src,6); VARCHAR(dst,6); VARCHAR(small,3);
    strcpy(src.arr, "abc");
    src.len = 3;
    int n = zv_copy(dst, src);
    CHECK("zv_copy len", n == 3 && dst.len == 3 && strcmp(dst.arr, "abc") == 0);
    n = zv_copy(small, src);
    CHECK("zv_copy fail", n == 0 && small.len == 2 && small.arr[2] == '\0');
}

/* Copy that exactly fills the destination including the terminator. */
static void test_copy_exact(void) {
    VARCHAR(src,4); VARCHAR(dst,4);
    strcpy(src.arr, "abc");
    src.len = 3;
    int n = zv_copy(dst, src);
    CHECK("zv_copy exact", n == 3 && dst.len == 3 && strcmp(dst.arr, "abc") == 0);
}

/* Empty source results in empty destination with terminator preserved. */
static void test_copy_empty(void) {
    VARCHAR(src,4); VARCHAR(dst,4);
    src.arr[0] = '\0';
    src.len = 0;
    int n = zv_copy(dst, src);
    CHECK("zv_copy empty", n == 0 && dst.len == 0 && dst.arr[0] == '\0');
}

/* Copying onto itself should leave the contents unchanged. */
static void test_copy_self(void) {
    VARCHAR(v,5);
    strcpy(v.arr, "abc");
    v.len = 3;
    int n = zv_copy(v, v);
    CHECK("zv_copy self", n == 3 && v.len == 3 && strcmp(v.arr, "abc") == 0);
}

/* Ensure very large strings are copied correctly and remain terminated. */
static void test_large_copy(void) {
    enum { N = 4096 };
    VARCHAR(src, N); VARCHAR(dst, N);
    memset(src.arr, 'a', N-1); src.arr[N-1] = '\0';
    src.len = N-1;
    int n = zv_copy(dst, src);
    CHECK("zv_copy large", n == N-1 && dst.len == N-1 && memcmp(dst.arr, src.arr, N-1) == 0 && dst.arr[N-1] == '\0');
}

/* Destination size of one cannot hold any characters. */
static void test_copy_dest_size_one(void) {
    VARCHAR(src,2); VARCHAR(dst,1);
    strcpy(src.arr,"a");
    src.len=1;
    int n = zv_copy(dst, src);
    CHECK("zv_copy tiny", n == 0 && dst.len == 0);
}

/*
 * Similar to test_extreme_copy in varchar tests but ensures zv_copy properly
 * copies very large, NUL-terminated strings.
 */
static void test_extreme_copy(void) {
    enum { N = 32768 };
    VARCHAR(src, N); VARCHAR(dst, N);
    memset(src.arr, 'x', N-1);
    src.arr[N-1] = '\0';
    src.len = N-1;
    int n = zv_copy(dst, src);
    CHECK("zv_copy extreme", n == N-1 && dst.len == N-1 && memcmp(dst.arr, src.arr, N-1) == 0 && dst.arr[N-1] == '\0');
}

/* Verify trimming functions while maintaining termination. */
static void test_trim(void) {
    VARCHAR(v1,10); VARCHAR(v2,10); VARCHAR(v3,10);
    strcpy(v1.arr, "  hi");
    v1.len = 4;
    zv_ltrim(v1);
    CHECK("zv_ltrim", v1.len == 2 && strcmp(v1.arr, "hi") == 0);

    strcpy(v2.arr, "hi  ");
    v2.len = 4;
    zv_rtrim(v2);
    CHECK("zv_rtrim", v2.len == 2 && strcmp(v2.arr, "hi") == 0);

    strcpy(v3.arr, "  hi  ");
    v3.len = 6;
    zv_trim(v3);
    CHECK("zv_trim", v3.len == 2 && strcmp(v3.arr, "hi") == 0);
}

/* Trimming an already trimmed string should do nothing. */
static void test_trim_noop(void) {
    VARCHAR(v,5);
    strcpy(v.arr, "hi");
    v.len = 2;
    zv_trim(v);
    CHECK("zv_trim no-op", v.len == 2 && strcmp(v.arr, "hi") == 0);
}

/* When the string only contains spaces trimming should yield empty string. */
static void test_trim_all_spaces(void) {
    VARCHAR(v,5);
    strcpy(v.arr, "   ");
    v.len = 3;
    zv_trim(v);
    CHECK("zv_trim all", v.len == 0 && v.arr[0] == '\0');
}

/* All trim functions should accept an already empty string. */
static void test_trim_empty(void) {
    VARCHAR(v,5);
    v.arr[0] = '\0';
    v.len = 0;
    zv_ltrim(v);
    CHECK("zv_ltrim empty", v.len == 0 && v.arr[0] == '\0');
    zv_rtrim(v);
    CHECK("zv_rtrim empty", v.len == 0 && v.arr[0] == '\0');
    zv_trim(v);
    CHECK("zv_trim empty", v.len == 0 && v.arr[0] == '\0');
}

/* Trimming should also strip tabs and newlines from the edges. */
static void test_trim_tabs_newlines(void) {
    VARCHAR(v1, 10); VARCHAR(v2, 10);
    strcpy(v1.arr, "\tfoo\n");
    v1.len = 5;
    zv_ltrim(v1);
    CHECK("zv_ltrim misc", v1.len == 4 && strcmp(v1.arr, "foo\n") == 0);
    strcpy(v2.arr, "foo\t\n");
    v2.len = 5;
    zv_rtrim(v2);
    CHECK("zv_rtrim misc", v2.len == 3 && strcmp(v2.arr, "foo") == 0);
}

/* Normal case conversion with preservation of terminator. */
static void test_case(void) {
    VARCHAR(v,4);
    strcpy(v.arr, "aB3");
    v.len = 3;
    zv_upper(v);
    CHECK("zv_upper", strcmp(v.arr, "AB3") == 0 && v.arr[3] == '\0');
    zv_lower(v);
    CHECK("zv_lower", strcmp(v.arr, "ab3") == 0 && v.arr[3] == '\0');
}

/* Upper/lower should not modify an empty string. */
static void test_case_empty(void) {
    VARCHAR(v,1);
    v.arr[0] = '\0';
    v.len = 0;
    zv_upper(v);
    CHECK("zv_upper empty", v.len == 0 && v.arr[0] == '\0');
    zv_lower(v);
    CHECK("zv_lower empty", v.len == 0 && v.arr[0] == '\0');
}

/* Non alphabetic characters should remain unaffected by case conversion. */
static void test_upper_lower_nonalpha(void) {
    VARCHAR(v,5);
    strcpy(v.arr, "a1!B");
    v.len=4;
    zv_upper(v);
    CHECK("zv_upper nonalpha", strcmp(v.arr,"A1!B") == 0);
    zv_lower(v);
    CHECK("zv_lower nonalpha", strcmp(v.arr,"a1!b") == 0);
}

/*
 * Mass upper/lower-case conversion on a zvarchar to validate handling of large
 * buffers and preservation of the trailing terminator.
 */
static void test_mass_case(void) {
    enum { N = 32768 };
    VARCHAR(v, N);
    memset(v.arr, 'a', N-1);
    v.arr[N-1] = '\0';
    v.len = N-1;
    zv_upper(v);
    for (size_t i = 0; i < N-1; i++)
        if (v.arr[i] != 'A') { CHECK("zv_mass_upper", 0); break; }
    zv_lower(v);
    for (size_t i = 0; i < N-1; i++)
        if (v.arr[i] != 'a') { CHECK("zv_mass_lower", 0); break; }
    CHECK("zv_mass_case term", v.arr[N-1] == '\0');
}

/* zv_strncpy copies with termination */
static void test_zv_strncpy(void) {
    VARCHAR(src,6); VARCHAR(dst,6);
    strcpy(src.arr, "abcd");
    src.len = 4;
    int n = zv_strncpy(dst, src, 2);
    CHECK("zv_strncpy", n == 2 && dst.len == 2 && strcmp(dst.arr, "ab") == 0);
}

static void test_zv_strncpy_overflow(void) {
    VARCHAR(src,4); VARCHAR(dst,3);
    memcpy(src.arr, "abcd", 4);
    src.len = 4;
    int n = zv_strncpy(dst, src, 4);
    CHECK("zv_strncpy overflow", n == 0 && dst.len == 0 && dst.arr[0] == '\0');
}

/* Concatenate zvarchars */
static void test_zv_strcat(void) {
    VARCHAR(a,6); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cd"); b.len = 2;
    int n = zv_strcat(a, b);
    CHECK("zv_strcat", n == 2 && a.len == 4 && strcmp(a.arr, "abcd") == 0);
}

static void test_zv_strcat_overflow(void) {
    VARCHAR(a,4); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cde"); b.len = 3;
    int n = zv_strcat(a, b);
    CHECK("zv_strcat overflow", n == 0 && a.len == 0 && a.arr[0] == '\0');
}

/* zv_strncat appends up to n chars */
static void test_zv_strncat(void) {
    VARCHAR(a,6); VARCHAR(b,4);
    strcpy(a.arr, "ab"); a.len = 2;
    memcpy(b.arr, "cdef", 4); b.len = 4;
    int n = zv_strncat(a, b, 2);
    CHECK("zv_strncat", n == 2 && a.len == 4 && strcmp(a.arr, "abcd") == 0);
}

static void test_zv_strncat_overflow(void) {
    VARCHAR(a,3); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cd"); b.len = 2;
    int n = zv_strncat(a, b, 2);
    CHECK("zv_strncat overflow", n == 0 && a.len == 0 && a.arr[0] == '\0');
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose = 1;

    test_init_clear();

    test_valid();
    test_valid_zero_len_good_term();
    test_valid_zero_len_bad_term();
    test_has_capacity();
    test_zv_capacity();

    test_zero_term();
    test_zero_term_idempotent();
    test_zero_term_empty();
    test_zero_term_size_one();

    test_copy();
    test_copy_exact();
    test_copy_empty();
    test_copy_self();
    test_large_copy();
    test_copy_dest_size_one();    
    test_extreme_copy();

    test_trim();
    test_trim_noop();
    test_trim_all_spaces();
    test_trim_empty();
    test_trim_tabs_newlines();

    test_case();
    test_case_empty();
    test_upper_lower_nonalpha();
    test_mass_case();

    test_zv_strncpy();
    test_zv_strncpy_overflow();
    test_zv_strcat();
    test_zv_strcat_overflow();
    test_zv_strncat();
    test_zv_strncat_overflow();

    if (failures == 0) {
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
