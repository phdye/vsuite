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

static void test_vf_copy(void) {
    VARCHAR(dst, 6);
    vf_copy(dst, "abc");
    CHECK("vf_copy len", dst.len == 3 && memcmp(dst.arr, "abc", 3) == 0);
}

static void test_vf_copy_overflow(void) {
    VARCHAR(dst, 3);
    vf_copy(dst, "abcd");
    CHECK("vf_copy overflow", dst.len == 0);
}

static void test_vf_copy_empty(void) {
    VARCHAR(dst, 4);
    vf_copy(dst, "");
    CHECK("vf_copy empty", dst.len == 0);
}

static void test_fv_copy(void) {
    char dst[6];
    VARCHAR(src, 6);
    strcpy(src.arr, "abc"); src.len = 3;
    fv_copy(dst, src);
    CHECK("fv_copy len", strcmp(dst, "abc") == 0);
}

static void test_fv_copy_overflow(void) {
    char dst[4];
    VARCHAR(src, 6);
    strcpy(src.arr, "abcd"); src.len = 4;
    fv_copy(dst, src);
    CHECK("fv_copy overflow", dst[0] == '\0');
}

static void test_fv_copy_empty(void) {
    char dst[4];
    VARCHAR(src, 4);
    src.len = 0; src.arr[0] = '\0';
    fv_copy(dst, src);
    CHECK("fv_copy empty", dst[0] == '\0');
}

int main(int argc, char **argv) {
    for (int i=1;i<argc;i++)
        verbose |= (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"));

    test_vf_copy();
    test_vf_copy_overflow();
    test_vf_copy_empty();

    test_fv_copy();
    test_fv_copy_overflow();
    test_fv_copy_empty();

    if (failures == 0)
        printf(verbose ? "\nAll tests passed.\n" : "\n");
    else
        printf("\n%d test(s) failed.\n", failures);

    return failures;
}
