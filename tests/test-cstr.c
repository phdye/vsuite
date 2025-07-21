#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsuite/varchar.h"
#include "vsuite/zvarchar.h" /* for zvp_copy helpers */
#include "vsuite/pstr.h"

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
 * Verify vp_copy() copies from a NUL terminated C string into a VARCHAR
 * when the source fits in the destination.  The length field should be
 * updated and the bytes copied verbatim.
 */
static void test_vp_copy(void) {
    VARCHAR(dst, 6);
    const char *src = "abc";
    vp_copy(dst, src);
    CHECK("vp_copy  len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

/*
 * When the C string is too large for the destination the macro should
 * clear the destination length to zero so callers can detect the error.
 */
static void test_vp_copy_overflow(void) {
    VARCHAR(dst, 4);
    const char *src = "abcd";
    vp_copy(dst, src);
    CHECK("vp_copy  overflow", dst.len == 0); /* no data copied */
}

/* Copying an empty string should yield an empty VARCHAR. */
static void test_vp_copy_empty(void) {
    VARCHAR(dst, 4);
    const char *src = "";
    vp_copy(dst, src);
    CHECK("vp_copy  empty", dst.len == 0);
}

/* zvp_copy behaves like vp_copy but always NUL terminates the target. */
static void test_zvp_copy(void) {
    VARCHAR(dst, 6);
    const char *src = "abc";
    zvp_copy(dst, src);
    CHECK("zvp_copy len", dst.len == 3 && strcmp(dst.arr, "abc") == 0);
}

/* Overflow when using zvp_copy clears length and leaves a terminator. */
static void test_zvp_copy_overflow(void) {
    VARCHAR(dst, 4);
    const char *src = "abcd";
    zvp_copy(dst, src);
    CHECK("zvp_copy overflow", dst.len == 0 && dst.arr[0] == '\0');
}

/* Empty string through zvp_copy should yield an empty, terminated VARCHAR. */
static void test_zvp_copy_empty(void) {
    VARCHAR(dst, 2);
    const char *src = "";
    zvp_copy(dst, src);
    CHECK("zvp_copy empty", dst.len == 0 && dst.arr[0] == '\0');
}

/* Large buffer stress test for zvp_copy. */
static void test_zvp_copy_large(void) {
    enum { N = 8192 };
    char src[N + 1];
    memset(src, 'd', N);
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    zvp_copy(dst, src);
    int ok = (dst.len == N && dst.arr[N] == '\0' && memcmp(dst.arr, src, N) == 0);
    CHECK("zvp_copy large", ok);
}

/*
 * Stress-test vp_copy with a very large source string. This ensures copying
 * from a C string into a VARCHAR works correctly for buffers well beyond
 * typical small sizes.
 */
static void test_vp_copy_large(void) {
    enum { N = 8192 };
    char src[N + 1];
    memset(src, 'a', N);
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    vp_copy(dst, src);
    int ok = (dst.len == N && memcmp(dst.arr, src, N) == 0);
    CHECK("vp_copy  large", ok);
}

/*
 * pv_copy() transfers a VARCHAR into a preallocated C buffer.  When the
 * buffer is large enough the result should be a NUL terminated duplicate of
 * the source.
 */
static void test_pv_copy(void) {
    char dst[6];
    VARCHAR(src, 6);
    strcpy(src.arr, "abc");
    src.len = 3;
    pv_copy(dst, sizeof(dst), src);
    CHECK("pv_copy  len", strcmp(dst, "abc") == 0);
}

/*
 * If the destination buffer is too small pv_copy() should write an empty
 * string so that callers know truncation occurred.
 */
static void test_pv_copy_overflow(void) {
    char dst[4];
    VARCHAR(src, 6);
    strcpy(src.arr, "abcd");
    src.len = 4;
    pv_copy(dst, sizeof(dst), src);
    CHECK("pv_copy  overflow", dst[0] == '\0'); /* buffer cleared */
}

/* Copying from an empty VARCHAR results in an empty destination C string. */
static void test_pv_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0;
    src.arr[0] = '\0';
    pv_copy(dst, sizeof(dst), src);
    CHECK("pv_copy  empty", dst[0] == '\0');
}

/*
 * When the destination capacity is reported as zero pv_copy() must not
 * touch the buffer at all.
 */
static void test_pv_copy_zero_cap(void) {
    char dst[1];
    VARCHAR(src, 2);
    strcpy(src.arr, "a");
    src.len = 1;
    dst[0] = 'x';
    pv_copy(dst, 0, src);
    CHECK("pv_copy  zero cap", dst[0] == 'x');
}

/* pv_copy with a size-one buffer should yield an empty string. */
static void test_pv_copy_dest_size_one(void) {
    char dst[1];
    VARCHAR(src, 2);
    strcpy(src.arr, "a");
    src.len = 1;
    pv_copy(dst, sizeof(dst), src);
    CHECK("pv_copy  tiny", dst[0] == '\0');
}

/*
 * Stress-test pv_copy with a very large VARCHAR source. This validates
 * copying into a fixed C buffer and that the destination is properly
 * NUL-terminated.
 */
static void test_pv_copy_large(void) {
    enum { N = 8192 };
    char dst[N + 1];
    VARCHAR(src, N + 1);
    memset(src.arr, 'b', N);
    src.arr[N] = '\0';
    src.len = N;
    pv_copy(dst, sizeof(dst), src);
    int ok = (memcmp(dst, src.arr, N) == 0 && dst[N] == '\0');
    CHECK("pv_copy  large", ok);
}

/*
 * dv_dup() allocates a new C string from a VARCHAR.  The returned pointer
 * should contain an exact copy and must be non-NULL.
 */
static void test_dv_dup_basic(void) {
    VARCHAR(src, 6);
    strcpy(src.arr, "abc");
    src.len = 3;
    char *d = dv_dup(src);
    CHECK("dv_dup   basic", d && strcmp(d, "abc") == 0);
    free(d);
}

/* Duplicate an empty VARCHAR and verify the returned C string is a
 * valid empty string.
 */
static void test_dv_dup_empty(void) {
    VARCHAR(src, 1);
    src.len = 0;
    src.arr[0] = '\0';
    char *d = dv_dup(src);
    int ok = (d && *d == '\0');
    CHECK("dv_dup   empty", ok);
    free(d);
}

/*
 * Duplicate a very large VARCHAR to ensure dv_dup handles big allocations
 * and produces an exact copy.
 */
static void test_dv_dup_large(void) {
    enum { N = 8192 };
    VARCHAR(src, N + 1);
    memset(src.arr, 'c', N);
    src.arr[N] = '\0';
    src.len = N;
    char *d = dv_dup(src);
    int ok = (d && strlen(d) == N && memcmp(d, src.arr, N) == 0);
    CHECK("dv_dup   large", ok);
    free(d);
}

/* Directly exercise dv_dup_fcn with a small string. */
static void test_dv_dup_fcn_basic(void) {
    const char src[] = "xyz";
    char *d = dv_dup_fcn(src, 3);
    CHECK("dv_dup_fcn basic", d && strcmp(d, "xyz") == 0);
    free(d);
}

/* dv_dup_fcn should handle empty strings. */
static void test_dv_dup_fcn_empty(void) {
    const char src[] = "";
    char *d = dv_dup_fcn(src, 0);
    CHECK("dv_dup_fcn empty", d && *d == '\0');
    free(d);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    test_vp_copy();
    test_vp_copy_overflow();
    test_vp_copy_empty();
    test_vp_copy_large();

    test_zvp_copy();
    test_zvp_copy_overflow();
    test_zvp_copy_empty();
    test_zvp_copy_large();

    test_pv_copy();
    test_pv_copy_overflow();
    test_pv_copy_empty();
    test_pv_copy_dest_size_one();
    test_pv_copy_zero_cap();
    test_pv_copy_large();

    test_dv_dup_basic();
    test_dv_dup_empty();
    test_dv_dup_large();
    test_dv_dup_fcn_basic();
    test_dv_dup_fcn_empty();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
