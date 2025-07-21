#include <stdio.h>
#include <string.h>
#include "vsuite/varchar.h"
#include "vsuite/fixed.h"

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
 * Test copying from a fixed C string literal into a VARCHAR.  The source
 * fits entirely so the resulting length should equal the literal length and
 * the bytes should match exactly.
 */
static void test_vf_copy(void) {
    VARCHAR(dst, 6);
    vf_copy(dst, "abc");
    CHECK("vf_copy len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

/*
 * When the literal does not fit the macro clears the destination length to
 * signal failure.
 */
static void test_vf_copy_overflow(void) {
    VARCHAR(dst, 3);
    vf_copy(dst, "abcd");
    CHECK("vf_copy overflow", dst.len == 0);
}

/* A zero-length literal should yield an empty VARCHAR. */
static void test_vf_copy_empty(void) {
    VARCHAR(dst, 4);
    vf_copy(dst, "");
    CHECK("vf_copy empty", dst.len == 0);
}

/* zvf_copy ensures the destination is always NUL terminated. */
static void test_zvf_copy(void) {
    VARCHAR(dst, 6);
    zvf_copy(dst, "abc");
    CHECK("zvf_copy len", dst.len == 3 && strcmp(dst.arr, "abc") == 0);
}

/* Overflow using zvf_copy clears the result and leaves a terminator. */
static void test_zvf_copy_overflow(void) {
    VARCHAR(dst, 3);
    zvf_copy(dst, "abcd");
    CHECK("zvf_copy overflow", dst.len == 0 && dst.arr[0] == '\0');
}

/* Empty strings copy cleanly with zvf_copy. */
static void test_zvf_copy_empty(void) {
    VARCHAR(dst, 2);
    zvf_copy(dst, "");
    CHECK("zvf_copy empty", dst.len == 0 && dst.arr[0] == '\0');
}

/* Large constant data should copy and remain terminated. */
static void test_zvf_copy_large(void) {
    enum { N = 8192 };
    static char src[N + 1];
    for (size_t i = 0; i < N; i++) src[i] = 'e';
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    zvf_copy(dst, src);
    int ok = (dst.len == N && dst.arr[N] == '\0' && memcmp(dst.arr, src, N) == 0);
    CHECK("zvf_copy large", ok);
}

/*
 * Verify vf_copy can handle copying a large constant string into a VARCHAR
 * without truncation or corruption.
 */
static void test_vf_copy_large(void) {
    enum { N = 8192 };
    static char src[N + 1];
    for (size_t i = 0; i < N; i++) src[i] = 'a';
    src[N] = '\0';
    VARCHAR(dst, N + 1);
    vf_copy(dst, src);
    int ok = (dst.len == N);
    ok = ok && memcmp(dst.arr, src, N) == 0;
    CHECK("vf_copy large", ok);
}

/*
 * Copying from a VARCHAR into a fixed C array should produce an identical
 * NUL terminated string when the buffer is large enough.
 */
static void test_fv_copy(void) {
    char dst[6];
    VARCHAR(src, 6);
    strcpy(src.arr, "abc");
    src.len = 3;
    fv_copy(dst, src);
    CHECK("fv_copy len", strcmp(dst, "abc") == 0);
}

/*
 * If the destination array is too small fv_copy() must write an empty
 * string so callers know truncation occurred.
 */
static void test_fv_copy_overflow(void) {
    char dst[4];
    VARCHAR(src, 6);
    strcpy(src.arr, "abcd");
    src.len = 4;
    fv_copy(dst, src);
    CHECK("fv_copy overflow", dst[0] == '\0');
}

/* Copying an empty VARCHAR should yield an empty C string. */
static void test_fv_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0; src.arr[0] = '\0';
    fv_copy(dst, src);
    CHECK("fv_copy empty", dst[0] == '\0');
}

/*
 * Ensure fv_copy correctly copies a large VARCHAR into a fixed C buffer and
 * preserves the NUL terminator.
 */
static void test_fv_copy_large(void) {
    enum { N = 8192 };
    char dst[N + 1];
    VARCHAR(src, N + 1);
    memset(src.arr, 'b', N);
    src.arr[N] = '\0';
    src.len = N;
    fv_copy(dst, src);
    int ok = (strcmp(dst, src.arr) == 0);
    CHECK("fv_copy large", ok);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    test_vf_copy();
    test_vf_copy_overflow();
    test_vf_copy_empty();
    test_vf_copy_large();

    test_zvf_copy();
    test_zvf_copy_overflow();
    test_zvf_copy_empty();
    test_zvf_copy_large();

    test_fv_copy();
    test_fv_copy_overflow();
    test_fv_copy_empty();
    test_fv_copy_large();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
