#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsuite/varchar.h"
#include "vsuite/zvarchar.h" /* for zvd_copy helpers */
#include "vsuite/cstr.h"

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
 * Verify vd_copy() copies from a NUL terminated C string into a VARCHAR
 * when the source fits in the destination.  The length field should be
 * updated and the bytes copied verbatim.
 */
static void test_vd_copy(void) {
    VARCHAR(dst, 6);
    const char *src = "abc";
    vd_copy(dst, src);
    CHECK("vd_copy  len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

/*
 * When the C string is too large for the destination the macro should
 * clear the destination length to zero so callers can detect the error.
 */
static void test_vd_copy_overflow(void) {
    VARCHAR(dst, 4);
    const char *src = "abcd";
    vd_copy(dst, src);
    CHECK("vd_copy  overflow", dst.len == 0); /* no data copied */
}

/* Copying an empty string should yield an empty VARCHAR. */
static void test_vd_copy_empty(void) {
    VARCHAR(dst, 4);
    const char *src = "";
    vd_copy(dst, src);
    CHECK("vd_copy  empty", dst.len == 0);
}

/* zvd_copy behaves like vd_copy but always NUL terminates the target. */
static void test_zvd_copy(void) {
    VARCHAR(dst, 6);
    const char *src = "abc";
    zvd_copy(dst, src);
    CHECK("zvd_copy len", dst.len == 3 && strcmp(dst.arr, "abc") == 0);
}

/* Overflow when using zvd_copy clears length and leaves a terminator. */
static void test_zvd_copy_overflow(void) {
    VARCHAR(dst, 4);
    const char *src = "abcd";
    zvd_copy(dst, src);
    CHECK("zvd_copy overflow", dst.len == 0 && dst.arr[0] == '\0');
}

/* Empty string through zvd_copy should yield an empty, terminated VARCHAR. */
static void test_zvd_copy_empty(void) {
    VARCHAR(dst, 2);
    const char *src = "";
    zvd_copy(dst, src);
    CHECK("zvd_copy empty", dst.len == 0 && dst.arr[0] == '\0');
}

/* Large buffer stress test for zvd_copy. */
static void test_zvd_copy_large(void) {
    enum { N = 8192 };
    char src[N + 1];
    memset(src, 'd', N);
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    zvd_copy(dst, src);
    int ok = (dst.len == N && dst.arr[N] == '\0' && memcmp(dst.arr, src, N) == 0);
    CHECK("zvd_copy large", ok);
}

/*
 * Stress-test vd_copy with a very large source string. This ensures copying
 * from a C string into a VARCHAR works correctly for buffers well beyond
 * typical small sizes.
 */
static void test_vd_copy_large(void) {
    enum { N = 8192 };
    char src[N + 1];
    memset(src, 'a', N);
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    vd_copy(dst, src);
    int ok = (dst.len == N && memcmp(dst.arr, src, N) == 0);
    CHECK("vd_copy  large", ok);
}

/*
 * dv_copy() transfers a VARCHAR into a preallocated C buffer.  When the
 * buffer is large enough the result should be a NUL terminated duplicate of
 * the source.
 */
static void test_dv_copy(void) {
    char dst[6];
    VARCHAR(src, 6);
    strcpy(src.arr, "abc");
    src.len = 3;
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy  len", strcmp(dst, "abc") == 0);
}

/*
 * If the destination buffer is too small dv_copy() should write an empty
 * string so that callers know truncation occurred.
 */
static void test_dv_copy_overflow(void) {
    char dst[4];
    VARCHAR(src, 6);
    strcpy(src.arr, "abcd");
    src.len = 4;
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy  overflow", dst[0] == '\0'); /* buffer cleared */
}

/* Copying from an empty VARCHAR results in an empty destination C string. */
static void test_dv_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0;
    src.arr[0] = '\0';
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy  empty", dst[0] == '\0');
}

/*
 * When the destination capacity is reported as zero dv_copy() must not
 * touch the buffer at all.
 */
static void test_dv_copy_zero_cap(void) {
    char dst[1];
    VARCHAR(src, 2);
    strcpy(src.arr, "a");
    src.len = 1;
    dst[0] = 'x';
    dv_copy(dst, 0, src);
    CHECK("dv_copy  zero cap", dst[0] == 'x');
}

/* dv_copy with a size-one buffer should yield an empty string. */
static void test_dv_copy_dest_size_one(void) {
    char dst[1];
    VARCHAR(src, 2);
    strcpy(src.arr, "a");
    src.len = 1;
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy  tiny", dst[0] == '\0');
}

/*
 * Stress-test dv_copy with a very large VARCHAR source. This validates
 * copying into a fixed C buffer and that the destination is properly
 * NUL-terminated.
 */
static void test_dv_copy_large(void) {
    enum { N = 8192 };
    char dst[N + 1];
    VARCHAR(src, N + 1);
    memset(src.arr, 'b', N);
    src.arr[N] = '\0';
    src.len = N;
    dv_copy(dst, sizeof(dst), src);
    int ok = (memcmp(dst, src.arr, N) == 0 && dst[N] == '\0');
    CHECK("dv_copy  large", ok);
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

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    test_vd_copy();
    test_vd_copy_overflow();
    test_vd_copy_empty();
    test_vd_copy_large();

    test_zvd_copy();
    test_zvd_copy_overflow();
    test_zvd_copy_empty();
    test_zvd_copy_large();

    test_dv_copy();
    test_dv_copy_overflow();
    test_dv_copy_empty();
    test_dv_copy_dest_size_one();
    test_dv_copy_zero_cap();
    test_dv_copy_large();

    test_dv_dup_basic();
    test_dv_dup_empty();
    test_dv_dup_large();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
