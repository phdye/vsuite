#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vsuite/varchar.h"
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
    CHECK("vd_copy len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

/*
 * When the C string is too large for the destination the macro should
 * clear the destination length to zero so callers can detect the error.
 */
static void test_vd_copy_overflow(void) {
    VARCHAR(dst, 4);
    const char *src = "abcd";
    vd_copy(dst, src);
    CHECK("vd_copy overflow", dst.len == 0); /* no data copied */
}

/* Copying an empty string should yield an empty VARCHAR. */
static void test_vd_copy_empty(void) {
    VARCHAR(dst, 4);
    const char *src = "";
    vd_copy(dst, src);
    CHECK("vd_copy empty", dst.len == 0);
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
    CHECK("dv_copy len", strcmp(dst, "abc") == 0);
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
    CHECK("dv_copy overflow", dst[0] == '\0'); /* buffer cleared */
}

/* Copying from an empty VARCHAR results in an empty destination C string. */
static void test_dv_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0;
    src.arr[0] = '\0';
    dv_copy(dst, sizeof(dst), src);
    CHECK("dv_copy empty", dst[0] == '\0');
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
    CHECK("dv_copy zero cap", dst[0] == 'x');
}

/*
 * dv_dup() allocates a new C string from a VARCHAR.  The returned pointer
 * should contain an exact copy and must be non-NULL.
 */
static void test_dv_dup(void) {
    VARCHAR(src, 6);
    strcpy(src.arr, "abc");
    src.len = 3;
    char *d = dv_dup(src);
    CHECK("dv_dup", d && strcmp(d, "abc") == 0);
    free(d);
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    test_vd_copy();
    test_vd_copy_overflow();
    test_vd_copy_empty();

    test_dv_copy();
    test_dv_copy_overflow();
    test_dv_copy_empty();
    test_dv_copy_zero_cap();
    test_dv_dup();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
