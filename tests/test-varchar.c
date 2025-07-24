#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "vsuite/varchar.h"

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
 * Test the basic initialization helpers. v_init() and v_clear() should both
 * reset the length to zero regardless of the prior contents of the buffer.
 */
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

/*
 * v_valid() only succeeds when the length is within the buffer size. This
 * function checks both a valid and an overflow case.
 */
static void test_valid(void) {
    VARCHAR(v, 5);
    v.len = 5;
    CHECK("v_valid ok", v_valid(v));           /* exact boundary */
    v.len = 6;
    CHECK("v_valid overflow", !v_valid(v));    /* length exceeds capacity */
}

/* v_has_capacity() checks the declared size of the VARCHAR. */
static void test_has_capacity(void) {
    VARCHAR(v, 5);
    v.len = 3;                      /* length does not affect test */
    CHECK("v_has_capacity ok", v_has_capacity(v, 4));      /* within size */
    CHECK("v_has_capacity full", v_has_capacity(v, 3));    /* also fits */
    CHECK("v_has_capacity big", !v_has_capacity(v, 6));    /* beyond size */
    CHECK("v_has_capacity max", v_has_capacity(v, 5));     /* exactly size */
}

/*
 * v_unused_capacity() reports the remaining space in the buffer. It should
 * clamp to zero when the length exceeds the declared size.
 */
static void test_unused_capacity(void) {
    VARCHAR(v, 5);
    v.len = 2;
    CHECK("v_unused_capacity", v_unused_capacity(v) == 3);
    v.len = 5;
    CHECK("v_unused_capacity zero", v_unused_capacity(v) == 0);
    v.len = 6;
    CHECK("v_unused_capacity overflow", v_unused_capacity(v) == 0);
}

/*
 * v_has_unused_capacity() checks whether a VARCHAR can accept additional
 * characters beyond its current length.  It must fail when full or when the
 * length already overflows.
 */
static void test_has_unused_capacity(void) {
    VARCHAR(v, 5);
    v.len = 2;
    CHECK("v_has_unused ok", v_has_unused_capacity(v, 3));
    CHECK("v_has_unused none", !v_has_unused_capacity(v, 4));
    v.len = 5;
    CHECK("v_has_unused full", !v_has_unused_capacity(v, 1));
    v.len = 7;
    CHECK("v_has_unused overflow", !v_has_unused_capacity(v, 1));
}

/*
 * Basic copy from one VARCHAR to another.  First copy succeeds when the
 * destination is large enough, then a second copy to a too-small buffer
 * should fail and leave the destination empty.
 */
static void test_copy(void) {
    VARCHAR(src, 6); VARCHAR(dst, 6); VARCHAR(small, 2);
    strcpy(src.arr, "abc");
    src.len = 3;
    int n = v_copy(dst, src);
    CHECK("v_copy len", n == 3 && dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);

    n = v_copy(small, src);
    CHECK("v_copy fail", n == 0 && small.len == 0);
}

/* Copy that exactly fills the destination buffer. */
static void test_copy_exact(void) {
    VARCHAR(src, 3); VARCHAR(dst, 3);
    strcpy(src.arr, "abc");
    src.len = 3;
    int n = v_copy(dst, src);
    CHECK("v_copy exact", n == 3 && dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

/* Copying an empty source should leave the destination empty as well. */
static void test_copy_empty(void) {
    VARCHAR(src, 4); VARCHAR(dst, 4);
    src.len = 0;
    int n = v_copy(dst, src);
    CHECK("v_copy empty", n == 0 && dst.len == 0);
}

/* Ensure copying onto itself behaves as a no-op. */
static void test_copy_self(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "abc");
    v.len = 3;
    int n = v_copy(v, v);
    CHECK("v_copy self", n == 3 && v.len == 3 && memcmp(v.arr, "abc", 3) == 0);
}

/* Copying into a destination of size 1 must fail since no character fits. */
static void test_copy_dest_size_one(void) {
    VARCHAR(src, 3); VARCHAR(dst, 1);
    strcpy(src.arr, "ab");
    src.len = 2;
    int n = v_copy(dst, src);
    CHECK("v_copy tiny", n == 0 && dst.len == 0);
}

/* Copy a very large string to ensure no size assumptions break. */
static void test_large_copy(void) {
    enum { N = 4096 };
    VARCHAR(src, N); VARCHAR(dst, N);
    memset(src.arr, 'a', N);
    src.len = N;
    int n = v_copy(dst, src);
    CHECK("v_copy large", n == N && dst.len == N && memcmp(dst.arr, src.arr, N) == 0);
}

/*
 * Exercise v_copy with an extremely large VARCHAR to stress internal loops
 * and boundary conditions at around 32k characters.
 */
static void test_extreme_copy(void) {
    enum { N = 32768 };
    VARCHAR(src, N); VARCHAR(dst, N);
    memset(src.arr, 'x', N);
    src.len = N;
    int n = v_copy(dst, src);
    CHECK("v_copy extreme", n == N && dst.len == N && memcmp(dst.arr, src.arr, N) == 0);
}

/* Verify left, right and full trimming of whitespace. */
static void test_trim(void) {
    VARCHAR(v1, 10); VARCHAR(v2, 10); VARCHAR(v3, 10);
    strcpy(v1.arr, "  hi");
    v1.len = 4;
    v_ltrim(v1);
    CHECK("v_ltrim", v1.len == 2 && memcmp(v1.arr, "hi", 2) == 0);

    strcpy(v2.arr, "hi  ");
    v2.len = 4;
    v_rtrim(v2);
    CHECK("v_rtrim", v2.len == 2 && memcmp(v2.arr, "hi", 2) == 0);

    strcpy(v3.arr, "  hi  ");
    v3.len = 6;
    v_trim(v3);
    CHECK("v_trim", v3.len == 2 && memcmp(v3.arr, "hi", 2) == 0);
}

/* Trimming should not modify an already trimmed string. */
static void test_trim_noop(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "hi");
    v.len = 2;
    v_ltrim(v);
    CHECK("v_ltrim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
    v_rtrim(v);
    CHECK("v_rtrim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
    v_trim(v);
    CHECK("v_trim no-op", v.len == 2 && memcmp(v.arr, "hi", 2) == 0);
}

/* Trimming a string of only spaces should result in an empty VARCHAR. */
static void test_trim_all_spaces(void) {
    VARCHAR(v, 6);
    strcpy(v.arr, "   ");
    v.len = 3;
    v_trim(v);
    CHECK("v_trim all", v.len == 0);
}

/* Ensure trimming functions handle an already empty string. */
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

/* Trimming also removes other whitespace characters such as tabs/newlines. */
static void test_trim_tabs_newlines(void) {
    VARCHAR(v1, 10); VARCHAR(v2, 10);
    strcpy(v1.arr, "\thi\n");
    v1.len = 4;
    v_ltrim(v1);
    CHECK("v_ltrim misc", v1.len == 3 && memcmp(v1.arr, "hi\n", 3) == 0);

    strcpy(v2.arr, "hi\t\n");
    v2.len = 4;
    v_rtrim(v2);
    CHECK("v_rtrim misc", v2.len == 2 && memcmp(v2.arr, "hi", 2) == 0);
}

/* Case conversion should ignore non alphabetic characters. */
static void test_upper_lower_nonalpha(void) {
    VARCHAR(v, 5);
    strcpy(v.arr, "a1!B");
    v.len = 4;
    v_upper(v);
    CHECK("v_upper nonalpha", strcmp(v.arr, "A1!B") == 0 && v.len == 4);
    v_lower(v);
    CHECK("v_lower nonalpha", strcmp(v.arr, "a1!b") == 0 && v.len == 4);
}

/* v_upper/v_lower should be no-ops on an empty string. */
static void test_case_empty(void) {
    VARCHAR(v, 3);
    v.len = 0;
    v_upper(v);
    CHECK("v_upper empty", v.len == 0);
    v_lower(v);
    CHECK("v_lower empty", v.len == 0);
}

/* Verify normal case conversion on alphabetic characters. */
static void test_case(void) {
    VARCHAR(v, 4);
    strcpy(v.arr, "aB3");
    v.len = 3;
    v_upper(v);
    CHECK("v_upper", strcmp(v.arr, "AB3") == 0 && v.len == 3);
    v_lower(v);
    CHECK("v_lower", strcmp(v.arr, "ab3") == 0 && v.len == 3);
}

/*
 * Toggle the case of a massive buffer to ensure v_upper and v_lower correctly
 * handle long strings.
 */
static void test_mass_case(void) {
    enum { N = 32768 };
    VARCHAR(v, N);
    memset(v.arr, 'a', N);
    v.len = N;
    v_upper(v);
    for (size_t i = 0; i < N; i++)
        if (v.arr[i] != 'A') { CHECK("v_mass_upper", 0); break; }
    v_lower(v);
    for (size_t i = 0; i < N; i++)
        if (v.arr[i] != 'a') { CHECK("v_mass_lower", 0); break; }
}

/* v_sprintf should format data into the destination when it fits. */
static void test_v_sprintf_basic(void) {
    VARCHAR(v, 16);
    int n = v_sprintf(v, "hi %d", 42);
    int ok = (n == 5 && v.len == 5 && memcmp(v.arr, "hi 42", 5) == 0);
    CHECK("v_sprintf basic", ok);
}

/* Overflow during formatting clears the destination length. */
static void test_v_sprintf_overflow(void) {
    VARCHAR(v, 4);
    int n = v_sprintf(v, "value %d", 100); /* longer than 4 */
    CHECK("v_sprintf overflow", n == 0 && v.len == 0);
}

/* Exactly filling the buffer should succeed and update the length. */
static void test_v_sprintf_exact(void) {
    VARCHAR(v, 4);
    int n = v_sprintf(v, "abcd");
    CHECK("v_sprintf exact", n == 4 && v.len == 4 && memcmp(v.arr, "abcd", 4) == 0);
}

/* Large formatting operations should also work correctly. */
static void test_v_sprintf_large(void) {
    enum { N = 4096 };
    char src[N + 1];
    memset(src, 'a', N);
    src[N] = '\0';
    VARCHAR(v, N);
    int n = v_sprintf(v, "%s", src);
    int ok = (n == N && v.len == N && memcmp(v.arr, src, N) == 0);
    CHECK("v_sprintf large", ok);
}

/*
 * Test helper macros V_SIZE(), V_BUF() and varchar_buf_t.  V_SIZE should
 * report the declared capacity while V_BUF must point at the underlying
 * array.  The varchar_buf_t typedef resolves to a single character array.
 */
static void test_helper_macros(void) {
    VARCHAR(v, 4);
    strcpy(v.arr, "abc");
    v.len = 3;
    CHECK("V_SIZE", V_SIZE(v) == 4);
    char *p = V_BUF(v);
    p[0] = 'x';
    CHECK("V_BUF", v.arr[0] == 'x');
    CHECK("varchar_buf_t", sizeof(varchar_buf_t) == 1);
}

/* v_strncpy copies up to n characters from src */
static void test_v_strncpy(void) {
    VARCHAR(src,6); VARCHAR(dst,6);
    strcpy(src.arr, "abcd");
    src.len = 4;
    int n = v_strncpy(dst, src, 2);
    CHECK("v_strncpy", n == 2 && dst.len == 2 && memcmp(dst.arr, "ab", 2) == 0);
}

/* Overflow during v_strncpy clears the destination */
static void test_v_strncpy_overflow(void) {
    VARCHAR(src,4); VARCHAR(dst,3);
    memcpy(src.arr, "abcd", 4);
    src.len = 4;
    int n = v_strncpy(dst, src, 4);
    CHECK("v_strncpy overflow", n == 0 && dst.len == 0);
}

/* v_strcat appends one VARCHAR to another */
static void test_v_strcat(void) {
    VARCHAR(a,6); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cd"); b.len = 2;
    int n = v_strcat(a, b);
    CHECK("v_strcat", n == 2 && a.len == 4 && memcmp(a.arr, "abcd", 4) == 0);
}

/* Overflow while concatenating clears the destination */
static void test_v_strcat_overflow(void) {
    VARCHAR(a,4); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cde"); b.len = 3;
    int n = v_strcat(a, b);
    CHECK("v_strcat overflow", n == 0 && a.len == 0);
}

/* v_strncat appends up to n characters */
static void test_v_strncat(void) {
    VARCHAR(a,6); VARCHAR(b,4);
    memcpy(a.arr, "ab", 2); a.len = 2;
    memcpy(b.arr, "cdef", 4); b.len = 4;
    int n = v_strncat(a, b, 2);
    CHECK("v_strncat", n == 2 && a.len == 4 && memcmp(a.arr, "abcd", 4) == 0);
}

/* v_strncat overflow clears the destination */
static void test_v_strncat_overflow(void) {
    VARCHAR(a,3); VARCHAR(b,3);
    strcpy(a.arr, "ab"); a.len = 2;
    strcpy(b.arr, "cd"); b.len = 2;
    int n = v_strncat(a, b, 2);
    CHECK("v_strncat overflow", n == 0 && a.len == 0);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++) if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) verbose = 1;

    test_init_clear();
    test_valid();
    test_has_capacity();
    test_unused_capacity();
    test_has_unused_capacity();

    test_copy();
    test_copy_exact();
    test_copy_empty();
    test_large_copy();
    test_copy_dest_size_one();
    test_copy_self();
    test_extreme_copy();

    test_trim();
    test_trim_noop();
    test_trim_all_spaces();
    test_trim_empty();
    test_trim_tabs_newlines();

    test_upper_lower_nonalpha();
    test_case_empty();
    test_case();
    test_mass_case();
    test_v_sprintf_basic();
    test_v_sprintf_overflow();
    test_v_sprintf_exact();
    test_v_sprintf_large();

    test_v_strncpy();
    test_v_strncpy_overflow();
    test_v_strcat();
    test_v_strcat_overflow();
    test_v_strncat();
    test_v_strncat_overflow();

    test_helper_macros();

    if (failures == 0) {
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    } else {
        printf("\n%d test(s) failed.\n", failures);
    }
    return failures;
}
